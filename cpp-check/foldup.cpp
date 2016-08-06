//Idea:
// foldup in.solution in.folds out.solution
// takes a bunch of folds as input
//
// starts with a network of edges and a folding direction for each edge.
// builds folded example from network.

//Multiple ideas:
//1) work with creases in source domain only
//2) work with creases in folded domain
//3) work from creases and directions

#include "structures.hpp"

#include <vector>
#include <iostream>
#include <unordered_map>

struct Facet {
	std::vector< K::Point_2 > source;
	std::vector< K::Point_2 > destination;

	//transform from source->destination as 2x3 (column major) matrix:
	K::Vector_2 xf[3] = {
		K::Vector_2(1,0),
		K::Vector_2(0,1),
		K::Vector_2(0,0)
	};
	//for convenience (can be computed from xf):
	bool flipped = false;

	void compute_xf();
};

K::Vector_2 p2v(K::Point_2 const &p) {
	return K::Vector_2(p.x(), p.y());
}

void Facet::compute_xf() {
	assert(source.size() == destination.size());
	if (source.size() == 0) {
		xf[0] = K::Vector_2(1,0);
		xf[1] = K::Vector_2(0,1);
		xf[2] = K::Vector_2(0,0);
		flipped = false;
	} else if (source.size() == 1) {
		xf[0] = K::Vector_2(1,0);
		xf[1] = K::Vector_2(0,1);
		xf[2] = p2v(destination[0]) - (source[0].x() * xf[0] + source[0].y() * xf[1]);
		flipped = false;
	} else {
		K::Vector_2 from_x = source[1] - source[0];
		K::Vector_2 to_x = destination[1] - destination[0];
		K::Vector_2 from_y(-from_x.y(), from_x.x());
		K::Vector_2 to_y(-to_x.y(), to_x.x());

		CGAL::Gmpq len2 = from_x * from_x;

		xf[0] = K::Vector_2(
			to_x.x() * from_x.x() + to_y.x() * from_y.x(),
			to_x.y() * from_x.x() + to_y.y() * from_y.x()
			) / len2;
		xf[1] = K::Vector_2(
			to_x.x() * from_x.y() + to_y.x() * from_y.y(),
			to_x.y() * from_x.y() + to_y.y() * from_y.y()
			) / len2;
		xf[2] = p2v(destination[0]) - (source[0].x() * xf[0] + source[0].y() * xf[1]);

		int32_t sign = 0;
		for (uint32_t i = 2; i < source.size(); ++i) {
			K::Vector_2 temp = xf[0] * source[i].x() + xf[1] * source[i].y() + xf[2];
			assert(temp.x() == destination[i].x());
			if (temp.y() == destination[i].y()) {
				if (sign == 0) sign = 1;
				assert(sign == 1);
			} else if (temp.y() == -destination[i].y()) {
				if (sign == 0) sign = -1;
				assert(sign == -1);
			} else {
				assert(temp.y() == destination[i].y());
			}
		}
		if (sign == -1) {
			xf[1] = -xf[1];
			flipped = true;
		} else {
			flipped = false;
		}
	}
}

struct State : public std::vector< Facet > {
	void fold_dest(K::Point_2 const &a, K::Point_2 const &b);
};

void State::fold_dest(K::Point_2 const &a, K::Point_2 const &b) {
	assert(a != b);

	K::Line_2 line(a,b);

	K::Vector_2 along = b - a;
	K::Vector_2 perp(-along.y(), along.x());

	K::Point_2 min = a;
	CGAL::Gmpq min_amt = p2v(min) * along;
	K::Point_2 max = b;
	CGAL::Gmpq max_amt = p2v(max) * along;
	K::Point_2 out = a + perp;
	CGAL::Gmpq out_amt = p2v(out) * perp;

	for (auto const &facet : *this) {
		for (auto const &pt : facet.destination) {
			auto along_amt = along * p2v(pt);
			if (along_amt < min_amt) {
				min_amt = along_amt;
				min = pt;
			}
			if (along_amt > max_amt) {
				max_amt = along_amt;
				max = pt;
			}
			auto perp_amt = perp * p2v(pt);
			if (perp_amt > out_amt) {
				out_amt = perp_amt;
				out = pt;
			}
		}
	}

	CGAL::Polygon_2< K > to_fold;

	min = min - along;
	max = max + along;
	out = out + perp;

	auto to_out = out - line.projection(out);

	to_fold.push_back(line.projection(min));
	to_fold.push_back(line.projection(max));
	to_fold.push_back(line.projection(max) + to_out);
	to_fold.push_back(line.projection(min) + to_out);

	assert(to_fold.orientation() == CGAL::COUNTERCLOCKWISE);

	K::Vector_2 flip_xf[3];
	{
		auto len2 = along * along;
		flip_xf[0] = K::Vector_2(
			along.x() * along.x() + -perp.x() * perp.x(),
			along.y() * along.x() + -perp.y() * perp.x()
			) / len2;
		flip_xf[1] = K::Vector_2(
			along.x() * along.y() + -perp.x() * perp.y(),
			along.y() * along.y() + -perp.y() * perp.y()
			) / len2;
		flip_xf[2] = p2v(a) - (flip_xf[0] * a.x() + flip_xf[1] * a.y());

		assert(flip_xf[0] * (b + perp).x() + flip_xf[1] * (b + perp).y() + flip_xf[2] == p2v(b - perp));
	}


	//any facet whose destination overlaps the line through a and b gets split.
	State result;
	for (auto const &facet : *this) {
		CGAL::Polygon_2< K > p(facet.destination.begin(), facet.destination.end());
		assert(p.orientation() == CGAL::COUNTERCLOCKWISE);

		auto set_source_from_dest = [](Facet &f) {
			for (auto const &v : f.destination) {
				f.source.emplace_back(
					K::Point_2(0,0)
					+ K::Vector_2(f.xf[0].x(), f.xf[1].x()) * (v - f.xf[2]).x()
					+ K::Vector_2(f.xf[0].y(), f.xf[1].y()) * (v - f.xf[2]).y()
				);
			}
			assert(f.source.size() == f.destination.size());
			for (uint32_t i = 0; i < f.source.size(); ++i) {
				assert(f.xf[0] * f.source[i].x() + f.xf[1] * f.source[i].y() + f.xf[2] == p2v(f.destination[i]));
			}
		};

		CGAL::Polygon_set_2< K > flip;
		flip.join(p);
		flip.intersection(to_fold);
		{ //read back the flipped parts:
			std::list< CGAL::Polygon_with_holes_2< K > > res;
			flip.polygons_with_holes( std::back_inserter( res ) );
			for (auto const &poly : res) {
				assert(!poly.is_unbounded());
				assert(poly.holes_begin() == poly.holes_end()); //no holes in facet
				auto boundary = poly.outer_boundary();
				Facet f;
				f.xf[0] = flip_xf[0] * facet.xf[0].x() + flip_xf[1] * facet.xf[0].y();
				f.xf[1] = flip_xf[0] * facet.xf[1].x() + flip_xf[1] * facet.xf[1].y();
				f.xf[2] = flip_xf[0] * facet.xf[2].x() + flip_xf[1] * facet.xf[2].y() + flip_xf[2];
				f.flipped = !facet.flipped;
				for (auto vi = boundary.vertices_begin(); vi != boundary.vertices_end(); ++vi) {
					K::Point_2 v = *vi;
					K::Point_2 flipped = line.projection(v) + (line.projection(v) - v); //flip over the line
					v = K::Point_2(0,0) + flip_xf[0] * v.x() + flip_xf[1] * v.y() + flip_xf[2]; //line.projection(v) + (line.projection(v) - v); //flip over the line
					assert(v == flipped);
					f.destination.emplace_back(v);
				}
				set_source_from_dest(f);
				result.emplace_back(f);
			}
		}

		CGAL::Polygon_set_2< K > noflip;
		noflip.join(p);
		noflip.difference(to_fold);
		{ //read back the not-flipped parts:
			std::list< CGAL::Polygon_with_holes_2< K > > res;
			noflip.polygons_with_holes( std::back_inserter( res ) );
			for (auto const &poly : res) {
				assert(!poly.is_unbounded());
				assert(poly.holes_begin() == poly.holes_end()); //no holes in facet
				auto boundary = poly.outer_boundary();
				Facet f;
				f.xf[0] = facet.xf[0];
				f.xf[1] = facet.xf[1];
				f.xf[2] = facet.xf[2];
				f.flipped = facet.flipped;
				for (auto vi = boundary.vertices_begin(); vi != boundary.vertices_end(); ++vi) {
					K::Point_2 v = *vi;
					f.destination.emplace_back(v);
				}
				set_source_from_dest(f);
				result.emplace_back(f);
			}
		}
	}

	*this = result;
}

State make_square() {
	State ret;
	ret.clear();
	ret.emplace_back();
	ret.back().source.emplace_back(0,0);
	ret.back().source.emplace_back(1,0);
	ret.back().source.emplace_back(1,1);
	ret.back().source.emplace_back(0,1);
	ret.back().destination = ret.back().source;
	ret.back().compute_xf();
	assert(ret.back().flipped == false);
	return ret;
}

int main(int argc, char **argv) {
	State state = make_square();

	state.fold_dest(
		K::Point_2(CGAL::Gmpq("0"),CGAL::Gmpq("1/4")),
		K::Point_2(CGAL::Gmpq("1"),CGAL::Gmpq("3/4"))
	);

	std::cout << "Now have " << state.size() << " facets." << std::endl;

	auto pp = [](CGAL::Gmpq const &q) -> std::string {
		std::ostringstream str;
		str << q.numerator();
		if (q.denominator() != 1) {
			str << "/" << q.denominator();
		}
		return str.str();
	};

/*
	//DEBUG:
	for (auto const &facet : state) {
		std::cout << "   ------\n";
		for (uint32_t i = 0; i < facet.source.size(); ++i) {
			std::string src_name = pp(facet.source[i].x()) + "," + pp(facet.source[i].y());
			std::string dst_name = pp(facet.destination[i].x()) + "," + pp(facet.destination[i].y());
			std::cout << "     " << src_name << " -> " << dst_name << "\n";
		}
		std::cout.flush();
	}
	std::cout << "------\n";
*/

	//print out solution -- need to de-duplicate verts for this.
	std::unordered_map< std::string, uint32_t > source_inds;
	std::vector< std::string > source_verts;
	std::vector< std::string > destination_verts;
	std::ostringstream facet_info;
	for (auto const &facet : state) {
		assert(facet.source.size() == facet.destination.size());
		facet_info << facet.source.size();
		for (uint32_t i = 0; i < facet.source.size(); ++i) {
			std::string src_name = pp(facet.source[i].x()) + "," + pp(facet.source[i].y());
			std::string dst_name = pp(facet.destination[i].x()) + "," + pp(facet.destination[i].y());
			uint32_t idx = source_inds.insert(std::make_pair(src_name, source_inds.size())).first->second;
			if (idx == source_verts.size()) {
				source_verts.emplace_back(src_name);
				destination_verts.emplace_back(dst_name);
			}
			assert(source_verts.size() == destination_verts.size());
			assert(idx < source_verts.size());
			assert(src_name == source_verts[idx]);
			assert(idx < destination_verts.size());
			assert(dst_name == destination_verts[idx]);

			facet_info << " " << idx;
		}
		facet_info << "\n";
	}

	std::ostringstream out;
	out << source_verts.size() << "\n";
	for (auto v : source_verts) {
		out << v << "\n";
	}
	out << state.size() << "\n";
	out << facet_info.str();
	for (auto v : destination_verts) {
		out << v << "\n";
	}

	std::cout << out.str();


	return 0;
}
