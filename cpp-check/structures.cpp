#include "structures.hpp"

#include <iostream>
#include <fstream>
#include <unordered_set>

//------------- Problem --------------------

CGAL::Polygon_set_2< K > Problem::get_silhouette() const {
	CGAL::Polygon_set_2< K > ret;
	for (auto const &poly : silhouette) {
		CGAL::Polygon_2< K > polygon(poly.begin(), poly.end());
		if (polygon.orientation() == CGAL::COUNTERCLOCKWISE) {
			//it's an outer boundary
			ret.join(polygon);
		} else {
			//it's a hole
			//reverse and difference (not sure if reversing is actually needed):
			polygon.reverse_orientation();
			ret.difference(polygon);
		}
	}
	return ret;
}


std::unique_ptr< Problem > Problem::read(std::string const &filename) {
	#define ERROR( X ) \
		do { \
			std::cerr << "ERROR reading problem file '" << filename << "': " << X << std::endl; \
			return std::unique_ptr< Problem >(); \
		} while(0)

	std::unique_ptr< Problem > ret(new Problem);
	std::ifstream file(filename);
	uint_fast32_t poly_count = 0;
	if (!(file >> poly_count)) {
		ERROR("failed to read polygon count.");
	}
	ret->silhouette.reserve(poly_count);
	for (uint_fast32_t p = 0; p < poly_count; ++p) {
		uint_fast32_t vert_count = 0;
		if (!(file >> vert_count)) {
			ERROR("failed to read vertex count for polygon " << p << ".");
		}
		ret->silhouette.emplace_back();
		ret->silhouette.back().reserve(vert_count);
		for (uint_fast32_t v = 0; v < vert_count; ++v) {
			CGAL::Gmpq x,y;
			char comma;
			if (!(file >> x >> comma >> y)) {
				ERROR("failed to read vertex " << v << " for polygon " << p << ".");
			}
			if (comma != ',') {
				ERROR("expected ',' got '" << comma << "' reading vertex " << v << " for polygon " << p << ".");
			}
			ret->silhouette.back().emplace_back(x, y);
		}
	}

	uint_fast32_t skel_count = 0;
	if (!(file >> skel_count)) {
		ERROR("failed to read skeleton count.");
	}
	ret->skeleton.reserve(skel_count);
	for (uint_fast32_t l = 0; l < skel_count; ++l) {
		CGAL::Gmpq x1,y1,x2,y2;
		char comma1, comma2;
		if (!(file >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2)) {
			ERROR("failed to read line " << l << " for skeleton.");
		}
		if (comma1 != ',' || comma2 != ',') {
			ERROR("expected ',' and ',' got '" << comma1 << "' and '" << comma2 << "' while reading point pair " << l << " in skeleton.");
		}
		ret->skeleton.emplace_back(K::Point_2(x1, y1), K::Point_2(x2, y2));
	}
	return ret;
	#undef ERROR
}

K::FT Problem::get_score1(const CGAL::Polygon_2<K>& a) const {
	return get_score(CGAL::Polygon_set_2<K>(a));
}

K::FT Problem::get_score(const CGAL::Polygon_set_2<K>& a) const {
	CGAL::Polygon_set_2< K > and_set = get_silhouette();
	and_set.intersection(a);
	CGAL::Polygon_set_2< K > or_set = get_silhouette();
	or_set.join(a);

	auto polygon_with_holes_area = [](CGAL::Polygon_with_holes_2< K > const &pwh) -> CGAL::Gmpq {
		assert(!pwh.is_unbounded());
		auto ret = pwh.outer_boundary().area();
		assert(ret >= 0);
		for (auto hi = pwh.holes_begin(); hi != pwh.holes_end(); ++hi) {
			auto ha = hi->area();
			assert(ha <= 0);
			ret += ha;
		}
		assert(ret >= 0);
		return ret;
	};

	auto polygon_set_area = [&polygon_with_holes_area](CGAL::Polygon_set_2< K > const &ps) -> CGAL::Gmpq {
		std::list< CGAL::Polygon_with_holes_2< K > > res;
		ps.polygons_with_holes( std::back_inserter( res ) );
		CGAL::Gmpq ret = 0;
		for (auto const &poly : res) {
			ret += polygon_with_holes_area(poly);
		}
		assert(ret >= 0);
		return ret;
	};

	auto and_area = polygon_set_area(and_set);
	auto or_area = polygon_set_area(or_set);

	return and_area / or_area;
}

//------------- Solution --------------------

CGAL::Polygon_set_2< K > Solution::get_silhouette() const {
	CGAL::Polygon_set_2< K > ret;
	for (auto const &facet : facets) {
		CGAL::Polygon_2< K > polygon;
		for (auto index : facet) {
			polygon.push_back(destination[index]);
		}
		if (polygon.orientation() == CGAL::CLOCKWISE) {
			//negative orientation, might as well flip it.
			polygon.reverse_orientation();
		}
		ret.join(polygon); //union it in.
	}
	return ret;
}

bool Solution::is_valid() const {

	for (auto const &s : source) {
		//"All the source positions of the vertices are within the initial square spanned by the four vertices (0,0), (1,0), (1,1), (0,1)".:
		if (s.x() < 0 || s.x() > 1 || s.y() < 0 || s.y() > 1) {
			std::cerr << "ERROR: Source vertex " << &s - &source[0] << " is outside the unit square." << std::endl;
			return false;
		}
		//"No coordinate appears more than once in the source positions part.":
		for (auto const &s2 : source) {
			if (&s2 >= &s) break;
			if (s == s2) {
				std::cerr << "ERROR: Source vertex " << (&s2 - &source[0]) << " and " << (&s - &source[0]) << " are the same." << std::endl;
				return false;
			}
		}
	}

	CGAL::Gmpq source_area(0);

	typedef CGAL::Arr_segment_traits_2< K > T2;
	typedef CGAL::Arrangement_2< T2 > Arrangement_2;

	std::vector< T2::X_monotone_curve_2 > source_edges;
	std::unordered_set< uint_fast32_t > used_verts;

	for (auto const &facet : facets) {
		for (uint_fast32_t i = 0; i < facet.size(); ++i) {
			used_verts.insert(facet[i]);

			auto const &a = source[facet[i]];
			auto const &b = source[facet[(i+1) % facet.size()]];

			K::Segment_2 seg(a,b);
			//"Any edge of any facet has length greater than zero."
			if (a == b) {
				std::cerr << "ERROR: facet " << (&facet - &facets[0]) << " has zero-length edge." << std::endl;
				return false;
			}
			source_edges.emplace_back(a,b);

			//"All facet polygons are simple; a facet polygon’s perimeter must not intersect itself."
			for (uint_fast32_t i2 = 0; i2 < facet.size(); ++i2) {
				if (i2 == i) continue;

				auto const &a2 = source[facet[i2]];
				auto const &b2 = source[facet[(i2+1) % facet.size()]];

				K::Segment_2 seg2(a2,b2);

				auto res = intersection(seg, seg2);
				if (i2 == (i + 1) % facet.size() || (i2 + 1) % facet.size() == i) {
					//should intersect in an endpoint
					if (res && boost::get< K::Point_2 >(&*res)) {
						//great!
					} else {
						std::cerr << "ERROR: adjacent edges of facet don't intersect in a point." << std::endl;
						return false;
					}
				} else {
					//should not intersect
					if (res) {
						std::cerr << "ERROR: non-adjacent edges of facet intersect." << std::endl;
						return false;
					}
				}

			}
		}

		//"At source position, the union set of all facets exactly matches the initial square."
		//(we do this by computing area)
		CGAL::Gmpq area = 0;
		for (uint_fast32_t i = 1; i + 1 < facet.size(); ++i) {
			auto const &a = source[facet[0]];
			auto const &b = source[facet[i]];
			auto const &c = source[facet[i+1]];
			area += K::Triangle_2(a,b,c).area();
		}
		if (area < 0) {
			source_area -= area;
		} else {
			source_area += area;
		}

		//"Every facet at source position maps to its destination position, by a congruent transformation that maps its source vertices to corresponding destination vertices."
		if (facet.size() >= 2) {
			//express each point by its dot product with first edge and perp:
			auto const &s_origin = source[facet[0]];
			auto const &d_origin = destination[facet[0]];
			auto s_dir = source[facet[1]] - s_origin;
			auto d_dir = destination[facet[1]] - d_origin;

			//make sure there isn't a scale in the xform:
			auto s_len2 = s_dir.x() * s_dir.x() + s_dir.y() * s_dir.y();
			auto d_len2 = d_dir.x() * d_dir.x() + d_dir.y() * d_dir.y();
			if (s_len2 != d_len2) {
				std::cerr << "ERROR: facet " << &facet - &facets[0] << " has a scaling between its source and destination poses." << std::endl;
				return false;
			}

			int_fast32_t sign = 0;
			for (auto index : facet) {
				auto s_vec = source[index] - s_origin;
				auto s_x = s_vec.x() * s_dir.x() + s_vec.y() * s_dir.y();
				auto s_y = s_vec.x() *-s_dir.y() + s_vec.y() * s_dir.x();
				auto d_vec = destination[index] - d_origin;
				auto d_x = d_vec.x() * d_dir.x() + d_vec.y() * d_dir.y();
				auto d_y = d_vec.x() *-d_dir.y() + d_vec.y() * d_dir.x();
				if (s_x != d_x) {
					std::cerr << "ERROR: facet " << &facet - &facets[0] << " changes shape under transformation (different along-first-edge coordinate)." << std::endl;
					return false;
				}
				if (s_y == 0 && d_y == 0) {
					//not much one can learn, but it's not an error.
				} else if (s_y == d_y) {
					if (sign == 0) sign = 1;
					if (sign != 1) {
						std::cerr << "ERROR: facet " << &facet - &facets[0] << " has one edge that doesn't look mirrored, but others are mirrored." << std::endl;
						return false;
					}
				} else if (s_y == -d_y) {
					if (sign == 0) sign = -1;
					if (sign != -1) {
						std::cerr << "ERROR: facet " << &facet - &facets[0] << " has one edge that looks mirrored, but others look unmirrored." << std::endl;
						return false;
					}
				} else {
					std::cerr << "ERROR: facet " << &facet - &facets[0] << " changes shape under transformation (different perpendicular-to-first-edge coordinate)." << std::endl;
					return false;
				}

			}
		}
	}
	if (source_area != 1) {
		std::cerr << "ERROR: facets have total area of " << source_area << " but a square would have area 1." << std::endl;
		return false;
	}

	//"At source positions, if two different edges share a point, the point should always be one of the endpoints for both the edges. That is, an edge touching another edge, or edges crossing each other are prohibited."
	{
		//construct planar arrangement of all source edges:
		Arrangement_2 arr;
		CGAL::insert(arr, source_edges.begin(), source_edges.end());

		//any edges that touch at midpoints or cross will generate new vertices, so check that this didn't happen:
		if (arr.number_of_vertices() != used_verts.size()) {
			std::cerr << "Faces mention " << used_verts.size() << " vertices, but arrangement of facet edges has " << arr.number_of_vertices() << "; this implies there was an intersection." << std::endl;
			return false;
		}
	}

	return true;

}

std::unique_ptr< Solution > Solution::read(std::string const &filename) {
	#define ERROR( X ) \
		do { \
			std::cerr << "ERROR reading solution file '" << filename << "': " << X << std::endl; \
			return std::unique_ptr< Solution >(); \
		} while(0)
	std::unique_ptr< Solution > ret(new Solution);
	std::ifstream file(filename);
	uint_fast32_t vert_count = 0;
	if (!(file >> vert_count)) {
		ERROR("failed to read vertex count.");
	}
	ret->source.reserve(vert_count);
	for (uint_fast32_t v = 0; v < vert_count; ++v) {
		CGAL::Gmpq x,y;
		char comma;
		if (!(file >> x >> comma >> y)) {
			ERROR("failed to read source vertex " << v << ".");
		}
		if (comma != ',') {
			ERROR("expected ',' got '" << comma << "' reading source vertex " << v << ".");
		}
		ret->source.emplace_back(x,y);
	}
	uint_fast32_t facet_count = 0;
	if (!(file >> facet_count)) {
		ERROR("failed to read facet count.");
	}
	ret->facets.reserve(facet_count);
	for (uint_fast32_t f = 0; f < facet_count; ++f) {
		uint_fast32_t index_count = 0;
		if (!(file >> index_count)) {
			ERROR("failed to read index count for facet " << f << ".");
		}
		ret->facets.emplace_back();
		ret->facets.back().reserve(index_count);
		for (uint_fast32_t i = 0; i < index_count; ++i) {
			uint_fast32_t index = 0;
			if (!(file >> index)) {
				ERROR("failed to read index " << i << " for facet " << f << ".");
			}
			if (index >= vert_count) {
				ERROR("index '" << index << "' is too large (there are " << vert_count << " vertices).");
			}
			ret->facets.back().emplace_back(index);
		}
	}
	ret->destination.reserve(vert_count);
	for (uint_fast32_t v = 0; v < vert_count; ++v) {
		CGAL::Gmpq x,y;
		char comma;
		if (!(file >> x >> comma >> y)) {
			ERROR("failed to read destination vertex " << v << ".");
		}
		if (comma != ',') {
			ERROR("expected ',' got '" << comma << "' reading destination vertex " << v << ".");
		}
		ret->destination.emplace_back(x,y);
	}
	return ret;
	#undef ERROR
}

