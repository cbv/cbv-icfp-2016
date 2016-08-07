#include <unordered_map>
#include <unordered_set>

#include "structures.hpp"
#include "folders.hpp"

//------------- Facet --------------------

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
		assert(source[1] != source[0]);
		K::Vector_2 from_x = source[1] - source[0];
		K::Vector_2 to_x = destination[1] - destination[0];
		K::Vector_2 from_y = from_x.perpendicular(CGAL::COUNTERCLOCKWISE);
		K::Vector_2 to_y = to_x.perpendicular(CGAL::COUNTERCLOCKWISE);

		//check if transform needs mirroring by looking for non-collinear boundary points:
		for (uint32_t i = 2; i < source.size(); ++i) {
			auto s = (source[i] - source[0]) * from_y;
			if (s == 0) continue;
			auto d = (destination[i] - destination[0]) * to_y;
			if (s == d) {
				flipped = false;
			} else if (s == -d) {
				to_y = -to_y;
				flipped = true;
			} else {
				assert(s == d || s == -d);
			}
			break;
		}

		CGAL::Gmpq len2 = from_x * from_x;

		xf[0] = K::Vector_2(
			to_x.x() * from_x.x() + to_y.x() * from_y.x(),
			to_x.y() * from_x.x() + to_y.y() * from_y.x()
			) / len2;
		xf[1] = K::Vector_2(
			to_x.x() * from_x.y() + to_y.x() * from_y.y(),
			to_x.y() * from_x.y() + to_y.y() * from_y.y()
			) / len2;
		xf[2] = (destination[0] - CGAL::ORIGIN) - (source[0].x() * xf[0] + source[0].y() * xf[1]);

		//paranoia:
		for (uint32_t i = 0; i < source.size(); ++i) {
			assert(xf[0] * source[i].x() + xf[1] * source[i].y() + xf[2] == destination[i] - CGAL::ORIGIN);
		}
	}
}

//------------- State --------------------

void State::unfold() {
	refold(std::vector< K::Point_2 >());
}

bool State::refold(std::vector< K::Point_2 > const &marks) {
	struct Edge {
		enum Tag {
			Outside,
			Flat,
			Fold
		} tag = Outside;
		K::Point_2 a;
		K::Point_2 b;
		uint32_t fa = -1U;
		uint32_t fb = -1U;
	};
	std::vector< Edge > edges;
	std::unordered_map< std::string, uint32_t > edge_idx;
	auto add_edge = [&](K::Point_2 const &a, K::Point_2 const &b, uint32_t f) {
		assert(a.x() < b.x() || (a.x() == b.x() && a.y() <= b.y()));
		std::ostringstream name;
		name << a << "," << b;
		uint32_t idx = edge_idx.insert(std::make_pair(name.str(), edges.size())).first->second;
		if (idx == edges.size()) {
			edges.emplace_back();
			edges.back().a = a;
			edges.back().b = b;
			edges.back().fa = f;
		} else {
			assert(idx < edges.size());
			assert(edges[idx].a == a);
			assert(edges[idx].b == b);
			assert(edges[idx].tag == Edge::Outside);
			edges[idx].tag = Edge::Flat;
			assert(edges[idx].fa != -1U);
			assert(edges[idx].fb == -1U);
			edges[idx].fb = f;
		}
		assert(edges[idx].fa == f || edges[idx].fb == f);
	};
	for (auto const &facet : *this) {
		for (uint32_t i = 0; i < facet.source.size(); ++i) {
			auto const &a = facet.source[i];
			auto const &b = facet.source[(i+1)%facet.source.size()];
			if (a.x() < b.x() || (a.x() == b.x() && a.y() <= b.y())) {
				add_edge(a,b, &facet - &((*this)[0]));
			} else {
				add_edge(b,a, &facet - &((*this)[0]));
			}
		}
	}
	//sanity check outside edges:
	for (auto const &e : edges) {
		if (e.tag == Edge::Outside) {
			assert(
			       (e.a.x() == 0 && e.b.x() == 0)
				|| (e.a.x() == 1 && e.b.x() == 1)
				|| (e.a.y() == 0 && e.b.y() == 0)
				|| (e.a.y() == 1 && e.b.y() == 1)
			);
		}
		//std::cerr << "Edge '" << e.a << "," << e.b << "' has " << e.fa << " and " << int32_t(e.fb) << std::endl; //DEBUG
	}

	auto mark = [&edges](std::vector< K::Point_2 > const &pts, Edge::Tag mark) {
		for (auto const &pt : pts) {
			Edge *close = nullptr;
			CGAL::Gmpq close_len2 = 0;
			for (auto &e : edges) {
				K::Line_2 line(e.a, e.b);
				K::Vector_2 to_line = pt - line.projection(pt);
				CGAL::Gmpq len2 = to_line * to_line;
				if (close == nullptr || len2 < close_len2) {
					close_len2 = len2;
					close = &e;
				}
			}
			if (!close) {
				std::cerr << "WARNING: mark at " << pt << " was far from everything." << std::endl;
			} else {
				if (close->tag == Edge::Flat) {
					close->tag = mark;
				} else if (close->tag == Edge::Outside) {
					std::cerr << "WARNING: mark at " << pt << " trying to mark outside edge." << std::endl;
				} else if (close->tag == mark) {
					std::cerr << "WARNING: mark at " << pt << " is marking an edge twice." << std::endl;
				} else {
					std::cerr << "WARNING: mark at " << pt << " is contradicting a different marking." << std::endl;
				
				}
			}
		}
	};

	mark(marks, Edge::Fold);

	//iterate through facets and transform based on folds:
	std::unordered_set< uint32_t > done;
	std::vector< uint32_t > to_expand;
	{
		(*this)[0].destination = (*this)[0].source;
		(*this)[0].compute_xf();
		done.insert(0);
		to_expand.push_back(0);
	}
	while (!to_expand.empty()) {
		uint32_t f_index = to_expand.back();
		to_expand.pop_back();
		assert(done.count(f_index));
		assert(f_index < this->size());
		auto &f = (*this)[f_index];
		//so f's xf is already set, but we should check all the neighbors based on the marks:
		for (uint32_t i = 0; i < f.source.size(); ++i) {
			auto const &a = f.source[i];
			auto const &b = f.source[(i+1)%f.source.size()];
			std::ostringstream name;
			if (a.x() < b.x() || (a.x() == b.x() && a.y() <= b.y())) {
				name << a << "," << b;
			} else {
				name << b << "," << a;
			}
			//std::cerr << "Facet " << f_index << " found edge " << name.str() << std::endl; //DEBUG
			auto ii = edge_idx.find(name.str());
			assert(ii != edge_idx.end());
			assert(ii->second < edges.size());
			auto const &edge = edges[ii->second];
			uint32_t other_index = -1U;
			if (edge.fa == f_index) {
				other_index = edge.fb;
			} else if (edge.fb == f_index) {
				other_index = edge.fa;
			} else {
				assert(0 && "facet must participate in edge");
			}
			assert(other_index != f_index);
			if (other_index == -1U) {
				assert(edge.tag == Edge::Outside);
				continue;
			}
			assert(other_index < this->size());

			K::Vector_2 other_xf[3];
			if (edge.tag == Edge::Flat) {
				other_xf[0] = f.xf[0];
				other_xf[1] = f.xf[1];
				other_xf[2] = f.xf[2];
			} else if (edge.tag == Edge::Fold) {
				auto xa = CGAL::ORIGIN + (f.xf[0] * a.x() + f.xf[1] * a.y() + f.xf[2]);
				auto xb = CGAL::ORIGIN + (f.xf[0] * b.x() + f.xf[1] * b.y() + f.xf[2]);
				K::Vector_2 along = xb - xa;
				K::Vector_2 perp(-along.y(), along.x());

				K::Vector_2 flip_xf[3];
				auto len2 = along * along;
				flip_xf[0] = K::Vector_2(
					along.x() * along.x() + -perp.x() * perp.x(),
					along.y() * along.x() + -perp.y() * perp.x()
					) / len2;
				flip_xf[1] = K::Vector_2(
					along.x() * along.y() + -perp.x() * perp.y(),
					along.y() * along.y() + -perp.y() * perp.y()
					) / len2;
				flip_xf[2] = p2v(xa) - (flip_xf[0] * xa.x() + flip_xf[1] * xa.y());

				other_xf[0] = flip_xf[0] * f.xf[0].x() + flip_xf[1] * f.xf[0].y();
				other_xf[1] = flip_xf[0] * f.xf[1].x() + flip_xf[1] * f.xf[1].y();
				other_xf[2] = flip_xf[0] * f.xf[2].x() + flip_xf[1] * f.xf[2].y() + flip_xf[2];
				assert( other_xf[0] * a.x() + other_xf[1] * a.y() + other_xf[2] == f.xf[0] * a.x() + f.xf[1] * a.y() + f.xf[2] );
				assert( other_xf[0] * b.x() + other_xf[1] * b.y() + other_xf[2] == f.xf[0] * b.x() + f.xf[1] * b.y() + f.xf[2] );
			} else {
				assert(0 && "That should be it in terms of tags.");
			}

			auto &other = (*this)[other_index];
			if (done.count(other_index)) {
				if (!( other.xf[0] == other_xf[0]
					&& other.xf[1] == other_xf[1]
					&& other.xf[2] == other_xf[2])) {
					std::cerr << "ERROR: inconsistent marking; can't refold." << std::endl;
					unfold(); //reset marking
					return false;
				}
			} else {
				other.xf[0] = other_xf[0];
				other.xf[1] = other_xf[1];
				other.xf[2] = other_xf[2];
				other.destination.clear();
				for (auto const &pt : other.source) {
					other.destination.emplace_back(
						K::Point_2(0,0) +
						other.xf[0] * pt.x() + other.xf[1] * pt.y() + other.xf[2]
					);
				}
				done.insert(other_index);
				to_expand.push_back(other_index);
			}


		}
	}

	assert(done.size() == this->size());
	return true;
}

bool State::fold_dest(K::Point_2 const &a, K::Point_2 const &b) {
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

	uint32_t from_flip = 0;
	uint32_t from_noflip = 0;

	//any facet whose destination overlaps the line through a and b gets split.
	State result;
	for (auto const &facet : *this) {
		CGAL::Polygon_2< K > p(facet.destination.begin(), facet.destination.end());
		if (p.orientation() != CGAL::COUNTERCLOCKWISE) {
			p.reverse_orientation();
		}

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
				++from_flip;
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
				++from_noflip;
			}
		}
	}
	*this = result;

#ifndef NDEBUG
	if (from_noflip == 0) {
		std::cerr << "WARNING: folded *everything*" << std::endl;
	}
	if (from_flip == 0) {
		std::cerr << "WARNING: folded *nothing*" << std::endl;
	}
#endif
	return (from_flip > 0);
}

void State::print_solution(std::ostream& out) const {
	auto pp = [](CGAL::Gmpq const &q) -> std::string {
		std::ostringstream str;
		str << q.numerator();
		if (q.denominator() != 1) {
			str << "/" << q.denominator();
		}
		return str.str();
	};

	std::unordered_map< std::string, uint32_t > source_inds;
	std::vector< std::string > source_verts;
	std::vector< std::string > destination_verts;
	std::ostringstream facet_info;
	for (auto const &facet : *this) {
		assert(facet.source.size() == facet.destination.size());
		facet_info << facet.source.size();
		for (uint_fast32_t i = 0; i < facet.source.size(); ++i) {
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

	out << source_verts.size() << "\n";
	for (auto v : source_verts) {
		out << v << "\n";
	}
	out << this->size() << "\n";
	out << facet_info.str();
	for (auto v : destination_verts) {
		out << v << "\n";
	}
}
