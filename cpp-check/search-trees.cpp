//search trees tries to unfold a problem by building a source from the facets of the skeleton.

#include "structures.hpp"
#include "folders.hpp"
#include "sha1.hpp"
#include "Viz1.hpp"

#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_curve_data_traits_2.h>

#include <unordered_map>

typedef uint64_t Key;

//describe topology of destination:
struct SearchEdge {
	uint32_t a = -1U, b = -1U;
	uint32_t ae = -1U, be = -1U;
	//CGAL::Gmpq length; //maybe?
	//edge is assumed to be face[a].boundary[ae] -> face[a].boundary[ae+1] or face[b].boundary[be+1] -> face[b].boundary[be]
};
struct SearchFace {
	std::vector< K::Point_2 > boundary;
	std::vector< uint32_t > edges;
	//CGAL::Gmpq area; //maybe?
};

//unroll in source domain:
struct ActiveEdge {
	uint32_t edge = -1U; //edge in edges[]
	K::Point_2 a,b; //points as they appear in edges[edge].a
	//bool a_is_out = false; //is edges[edge].a the outward face?
	bool perp_is_out = false; //is the ccw perpendicular direction the out direction?
};

struct UnrollState {
	uint64_t source_key = 0;
	uint32_t added_face = -1U;
	K::Vector_2 added_xf[3];

	std::vector< ActiveEdge > active_edges;

	std::set< uint32_t > unused_faces;
	CGAL::Gmpq remaining_area = 1; //area left in source square
	CGAL::Gmpq unused_face_area = 0; //total area of unused faces
	
	//create a key that uniquely identifies this unrolling step:
	Key compute_key() const {
		std::vector< std::string > names;
		names.reserve(active_edges.size());
		for (auto const &e : active_edges) {
			std::ostringstream name;
			//this was being slightly too good about disambiguating things when it sorted the endpoints.
			name << e.perp_is_out << "|" << e.edge << "|" << e.a.x() << "|" << e.a.y() << "|" << e.b.x() << "|" << e.b.y();
			names.emplace_back(name.str());
		}
		std::sort(names.begin(), names.end());
		static SHA1_CTX ctx;
		uint8_t digest[SHA1_DIGEST_SIZE];

		SHA1_Init(&ctx);
		for (auto const &name : names) {
			SHA1_Update(&ctx, reinterpret_cast< const uint8_t * >(name.c_str()), name.size());
		}
		SHA1_Final(&ctx, digest);

		static_assert(SHA1_DIGEST_SIZE == 20, "digest is big enough");
		uint64_t ret = 0;
		for (uint32_t i = 0; i < 8; ++i) {
			ret = (ret << 8) ^ digest[i];
		}

		return ret;
	}
};

inline glm::vec2 to_glm(K::Point_2 const &pt) {
	return glm::vec2(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
};


void show(std::vector< std::tuple< glm::vec2, glm::vec2, glm::u8vec4 > > const &lines, bool pause = true) {
	static Viz1 viz(glm::uvec2(400, 400));
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	glm::vec2 min(std::numeric_limits< float >::infinity());
	glm::vec2 max(-std::numeric_limits< float >::infinity());
	for (auto const &line : lines) {
		min = glm::min(min, std::get< 0 >(line));
		min = glm::min(min, std::get< 1 >(line));
		max = glm::max(max, std::get< 0 >(line));
		max = glm::max(max, std::get< 1 >(line));
	}

	viz.draw = [&](){
		float diameter = 2.0f;
		diameter = std::max(max.y - min.y, diameter);
		diameter = std::max((max.x - min.x) * 2.0f / viz.aspect, diameter);

		glClearColor(0.62f, 0.66f, 0.65f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLoadIdentity();
		glScalef(2.0f / (viz.aspect * diameter), 2.0f / diameter, 1.0f);
		glTranslatef(-0.5f * (max.x + min.x), -0.5f * (max.y + min.y), 0.0f);

		glBegin(GL_LINES);
		for (auto const &line : lines) {
			glColor4ub(std::get< 2 >(line).r, std::get< 2 >(line).g, std::get< 2 >(line).b, std::get< 2 >(line).a);
			glVertex2f(std::get< 0 >(line).x, std::get< 0 >(line).y);
			glVertex2f(std::get< 1 >(line).x, std::get< 1 >(line).y);
		}
		glEnd();

		if (!pause) viz.stop_flag = true;

	};
	viz.handle_event = [&](SDL_Event const &e) {
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
			exit(1);
		}
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
			viz.stop_flag = true;
		}
	};

	viz.stop_flag = false;
	viz.run();
}


std::vector< SearchEdge > edges;
std::vector< SearchFace > faces;
std::vector< CGAL::Gmpq > face_areas;

int main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cerr << "Usage:\n./search-trees <problem> [file-to-write]" << std::endl;
		return 1;
	}
	std::unique_ptr< Problem > problem = Problem::read(argv[1]);
	if (!problem) {
		std::cerr << "ERROR: Failed to read problem." << std::endl;
		return 1;
	}

	{ //okay, first make an arrangement to extract facets from skeleton:
		struct Merge {
			int32_t operator()(int32_t a, int32_t b) {
				return a ^ b;
			}
		};
		typedef CGAL::Arr_segment_traits_2< K >::Curve_2 Curve_2;
		typedef CGAL::Arr_curve_data_traits_2< CGAL::Arr_segment_traits_2< K >, int32_t, Merge > Tr;
		typedef CGAL::Arr_face_extended_dcel< Tr, int32_t > Dcel;
		typedef CGAL::Arrangement_2< Tr, Dcel > Arrangement_2;
		Arrangement_2 arr;

		std::vector< Tr::Curve_2 > segs;
		for (auto const &poly : problem->silhouette) {
			//these are counterclockwise oriented, but since I'm not adjusting half-edges this doesn't matter.
			for (uint32_t i = 0; i < poly.size(); ++i) {
				auto const &a = poly[i];
				auto const &b = poly[(i+1)%poly.size()];
				segs.emplace_back(Curve_2(a,b),1);
			}
		}
		for (auto const &line : problem->skeleton) {
			segs.emplace_back(Curve_2(line.first,line.second),0);
		}

		CGAL::insert(arr, segs.begin(), segs.end());

		//mark all faces as not-visited:
		for (auto f = arr.faces_begin(); f != arr.faces_end(); ++f) {
			f->set_data(0);
		}

		//okay, now iterate faces from outside, keeping track of current inside/outside flag.
		std::vector< Arrangement_2::Face_const_handle > to_visit;
		arr.unbounded_face()->set_data(-1); //mark as visited, outside

		to_visit.emplace_back(arr.unbounded_face());

		while (!to_visit.empty()) {
			Arrangement_2::Face_const_handle f = to_visit.back();
			to_visit.pop_back();
			assert(f->data() == -1 || f->data() == 1);

			auto do_ccb = [&](Arrangement_2::Ccb_halfedge_const_circulator start) {
				auto he = start;
				do {
					auto other_face = he->twin()->face();

					int32_t other_mark;

					if ((he->curve().data() & 1)) {
						//boundary
						other_mark = (f->data() == 1 ? -1 : 1);
					} else {
						other_mark = f->data();
					}

					if (other_face->data() == 0) {
						arr.non_const_handle(other_face)->set_data(other_mark);
						to_visit.push_back(other_face);
					} else {
						if (other_face->data() != other_mark) {
							std::cerr << "Weirdly enough, got conflicting marks for a face. Is the boundary reasonable?" << std::endl;
						}
					}

					++he;
				} while (he != start);
				
			};

			if (!f->is_unbounded()) {
				do_ccb(f->outer_ccb());
			}

			for (Arrangement_2::Hole_const_iterator h = f->holes_begin(); h != f->holes_end(); ++h) {
				do_ccb(*h);
			}
		}

		//check that all faces are marked:
		uint32_t inside = 0;
		uint32_t outside = 0;
		for (auto f = arr.faces_begin(); f != arr.faces_end(); ++f) {
			if (f->data() == 1) {
				assert(!f->is_unbounded());
				++inside;
			} else if (f->data() == -1) {
				++outside;
			} else {
				//all faces should be marked at this point
				assert(f->data() == -1 || f->data() == 1);
			}
		}
		std::cerr << "Have " << inside << " inside faces and " << outside << " outside faces." << std::endl;

		//clear edge markings (will use to store indices):
		for (auto e = arr.edges_begin(); e != arr.edges_end(); ++e) {
			e->curve().set_data(-1);
		}

		//now add edges and faces into 'edges' and 'faces' arrays:
		edges.reserve(arr.number_of_edges());
		faces.reserve(inside);
		for (auto f = arr.faces_begin(); f != arr.faces_end(); ++f) {
			if (f->data() == -1) {
				//skip outside faces
				continue;
			}

			assert(f->data() == 1 && "Face should be marked as inner.");
			assert(!f->is_unbounded() && "Inner faces should be bounded.");
			assert(f->holes_begin() == f->holes_end() && "Facets with holes shouldn't happen.");

			uint32_t face_idx = faces.size();
			f->data() = face_idx;
			faces.emplace_back();
			auto &face = faces.back();

			auto start = f->outer_ccb();
			auto he = start;
			do {
				if (he->curve().data() == -1) {
					he->curve().set_data(edges.size());
					edges.emplace_back();
				}
				uint32_t edge_idx = he->curve().data();
				assert(edge_idx < edges.size());
				auto &edge = edges[edge_idx];
				if (edge.a == -1U) {
					assert(edge.b == -1U);
					edge.a = face_idx;
					edge.ae = face.edges.size();
				} else if (edge.b == -1U) {
					assert(edge.a != face_idx);
					edge.b = face_idx;
					edge.be = face.edges.size();
								} else {
					assert(0 && "Edge should have at most two adjacent faces.");
				}

				face.boundary.emplace_back(he->source()->point());
				face.edges.emplace_back(edge_idx);

				++he;
			} while (he != start);
		}
	}

	//check edge <-> face consistency:
	for (auto const &e : edges) {
		//check that edge has endpoints that we will later expect:
		assert(e.a < faces.size());
		auto const &fa = faces[e.a];
		assert(e.ae < fa.edges.size());
		assert(fa.edges[e.ae] == &e - &edges[0]);

		if (e.b == -1U) continue;

		assert(e.b < faces.size());
		auto const &fb = faces[e.b];
		assert(e.be < fb.edges.size());
		assert(fb.edges[e.be] == &e - &edges[0]);

		assert(fa.boundary[e.ae] == fb.boundary[(e.be+1) % fb.boundary.size()]);
		assert(fa.boundary[(e.ae+1)%fa.boundary.size()] == fb.boundary[e.be]);
	}

	std::cerr << "Extracted " << faces.size() << " faces and " << edges.size() << " edges." << std::endl;

	face_areas.reserve(faces.size());
	for (auto const &face : faces) {
		CGAL::Polygon_2< K > poly(face.boundary.begin(), face.boundary.end());
		assert(poly.orientation() == CGAL::COUNTERCLOCKWISE);
		face_areas.emplace_back(poly.area());
		//std::cerr << "  " <<  face_areas.back() << std::endl;//DEBUG
	}
	assert(face_areas.size() == faces.size());


	UnrollState root;
	{
		root.unused_face_area = 0;
		for (uint32_t f = 0; f < faces.size(); ++f) {
			root.unused_faces.insert(f);
			root.unused_face_area += face_areas[f];
		}
		root.remaining_area = 1;
		assert(root.remaining_area >= root.unused_face_area);
	}
	Key root_key = root.compute_key();

	//root isn't in states because root also looks like "solved" and that confuses the code.

	std::unordered_map< Key, UnrollState > states;

	struct {
		uint32_t expanded = 0;
		uint32_t tried = 0;
		uint32_t added = 0;
		uint32_t repeat = 0;
		uint32_t no_face_area = 0;
		uint32_t no_other_area = 0;
		uint32_t out_of_box = 0;
		uint32_t intersection = 0;
		uint32_t imperfect_overlap = 0;
		uint32_t dead_ends = 0;
		void dump() {
			std::cerr << "Have expanded " << expanded << " states, tried to add " << tried << " and added " << added << ":\n"
					<< "  " << repeat << " were already added\n"
					<< "  " << no_face_area << " didn't have area for face\n"
					<< "  " << no_other_area << " didn't have area for unused faces\n"
					<< "  " << out_of_box << " were out of box\n"
					<< "  " << intersection << " intersected active edges\n"
					<< "  " << imperfect_overlap << " exactly overlapped different edges\n"
					<< "  " << dead_ends << " discarded dead ends\n"
				;
				std::cerr.flush();
		}
	} stats;

	auto report = [&stats,&states,&root_key,&argc,&argv](UnrollState const &end) {

		stats.dump();
		std::cerr << " ----- found solution -----" << std::endl;

		assert(end.compute_key() == root_key); //solution always looks like root

		State solution;
		const UnrollState *at = &end;
		while (true) {
			assert(at->added_face < faces.size());
			solution.emplace_back();
			Facet &facet = solution.back();
			for (auto const &d : faces[at->added_face].boundary) {
				facet.destination.emplace_back(d);
				facet.source.emplace_back(CGAL::ORIGIN + at->added_xf[0] * d.x() + at->added_xf[1] * d.y() + at->added_xf[2]);
			}
			assert(facet.source.size() == facet.destination.size());
			//sanity check distances:
			for (uint32_t i = 0; i < facet.destination.size(); ++i) {
				for (uint32_t j = 0; j < facet.destination.size(); ++j) {
					auto len2_src = (facet.source[i] - facet.source[j]) * (facet.source[i] - facet.source[j]);
					auto len2_dst = (facet.destination[i] - facet.destination[j]) * (facet.destination[i] - facet.destination[j]);
					assert(len2_src == len2_dst);
				}
			}
			facet.compute_xf();

			if (at->source_key == root_key) break;
			auto f = states.find(at->source_key);
			assert(f != states.end());
			at = &f->second;
		}
#ifdef DEBUG_SOLN
		{
			std::vector< std::tuple< glm::vec2, glm::vec2, glm::u8vec4 > > lines;
			for (auto const &facet : solution) {
				for (uint32_t i = 0; i < facet.source.size(); ++i) {
					lines.emplace_back(
						to_glm(facet.source[i]), to_glm(facet.source[(i+1)%facet.source.size()]), glm::u8vec4(0xff, 0xff, 0xff, 0xff)
					);
					glm::vec2 ofs(2.0f, 0.0f);
					lines.emplace_back(
						to_glm(facet.destination[i]) + ofs, ofs + to_glm(facet.destination[(i+1)%facet.destination.size()]), glm::u8vec4(0x00, 0x00, 0x00, 0xff)
					);
					lines.emplace_back(to_glm(facet.source[i]), ofs + to_glm(facet.destination[i]), glm::u8vec4(0xff, 0x00, 0x00, 0x88));
				}
			}
			show(lines);
		}
#endif

		if (argc == 3) {
			std::cerr << "   (writing to " << argv[2] << ")" << std::endl;
			solution.print_solution(argv[2]);
		} else {
			solution.print_solution(std::cout);
		}
		exit(0);
	};

	//helper to manage expanding states:
	auto try_adding_face = [&stats, &states, &report](Key const &key, UnrollState const &us, uint32_t face_idx, K::Vector_2 (&xf)[3], std::unordered_map< Key, UnrollState > *new_states) -> bool {
//#define DEBUG_ADD 1
//#define DEBUG_SHOW 1

		assert(face_idx < faces.size());
		assert(faces.size() == face_areas.size());

		++stats.tried;

		#if defined(DEBUG_ADD) || defined(DEBUG_SHOW)
		std::vector< std::tuple< glm::vec2, glm::vec2, glm::u8vec4 > > show_lines;
		#if defined(DEBUG_ADD)
		{
		#elif defined(DEBUG_SHOW)
		static uint32_t count = 0;
		count += 1;
		if (count >= 100)
		{
			count = 0;
		#endif

			show_lines.emplace_back(glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::u8vec4(0xaa, 0x88, 0x88, 0xff));
			show_lines.emplace_back(glm::vec2(1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::u8vec4(0xaa, 0x88, 0x88, 0xff));
			show_lines.emplace_back(glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::u8vec4(0xaa, 0x88, 0x88, 0xff));
			show_lines.emplace_back(glm::vec2(0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::u8vec4(0xaa, 0x88, 0x88, 0xff));
			for (auto const &ae : us.active_edges) {
				glm::vec2 a = to_glm(ae.a);
				glm::vec2 b = to_glm(ae.b);
				show_lines.emplace_back(a, b, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
				glm::vec2 along = glm::normalize(b - a);
				glm::vec2 out;
				glm::u8vec4 color;
				if (ae.perp_is_out) {
					out = glm::vec2(-along.y, along.x);
					color = glm::u8vec4(0x00, 0x00, 0xff, 0xff);
				} else {
					out =-glm::vec2(-along.y, along.x);
					color = glm::u8vec4(0x00, 0xff, 0x00, 0xff);
				}
				show_lines.emplace_back(0.5f * (a+b), 0.5f * (a+b) + 0.02f * out, color);
			}

			SearchFace const &face = faces[face_idx];
			for (uint32_t fe = 0; fe < face.boundary.size(); ++fe) {
				K::Point_2 const &a = face.boundary[fe];
				K::Point_2 const &b = face.boundary[(fe + 1) % face.boundary.size()];

				K::Point_2 xa = CGAL::ORIGIN + xf[0] * a.x() + xf[1] * a.y() + xf[2];
				K::Point_2 xb = CGAL::ORIGIN + xf[0] * b.x() + xf[1] * b.y() + xf[2];

				K::Vector_2 out = (b - a).perpendicular(CGAL::CLOCKWISE); //face is in ccw order
				K::Vector_2 xout = xf[0] * out.x() + xf[1] * out.y();

				show_lines.emplace_back(to_glm(xa), to_glm(xb), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
				show_lines.emplace_back(0.5f * (to_glm(xa) + to_glm(xb)), 0.5f * (to_glm(xa) + to_glm(xb)) + 0.02f * glm::normalize(to_glm(CGAL::ORIGIN + xout)), glm::u8vec4(0xff, 0xff, 0xff, 0x88));
			}
			#if defined(DEBUG_ADD)
			show(show_lines);
			#else
			show(show_lines, false);
			#endif
		}
		#endif //DEBUG_ADD

		UnrollState ns = us;
		ns.source_key = key;
		ns.added_face = face_idx;
		ns.added_xf[0] = xf[0];
		ns.added_xf[1] = xf[1];
		ns.added_xf[2] = xf[2];

		{ //area check:
			//is there enough area left to place this face?
			if (ns.remaining_area < face_areas[face_idx]) {
				++stats.no_face_area;

				#ifdef DEBUG_ADD
				std::cerr << " -- no area for face" << std::endl; //DEBUG
				#endif //DEBUG_ADD

				return false;
			}
			ns.remaining_area -= face_areas[face_idx];

			auto f = ns.unused_faces.find(face_idx);
			if (f != ns.unused_faces.end()) {
				ns.unused_faces.erase(f);
				assert(ns.unused_face_area >= face_areas[face_idx]);
				ns.unused_face_area -= face_areas[face_idx];
			}
			//is there enough area left for everything else?
			if (ns.unused_face_area > ns.remaining_area) {
				++stats.no_other_area;

				#ifdef DEBUG_ADD
				std::cerr << " -- no area unused" << std::endl; //DEBUG
				#endif //DEBUG_ADD

				return false;
			}
		}

		//build active edges (and check for out-of-bounds):

		std::vector< ActiveEdge > new_edges;
		SearchFace const &face = faces[face_idx];
		for (uint32_t fe = 0; fe < face.edges.size(); ++fe) {
			K::Point_2 const &a = face.boundary[fe];
			K::Point_2 const &b = face.boundary[(fe + 1) % face.boundary.size()];

			K::Point_2 xa = CGAL::ORIGIN + xf[0] * a.x() + xf[1] * a.y() + xf[2];
			K::Point_2 xb = CGAL::ORIGIN + xf[0] * b.x() + xf[1] * b.y() + xf[2];

			if (xa.x() < 0 || xa.x() > 1
			 || xa.y() < 0 || xa.y() > 1
			 || xb.x() < 0 || xb.x() > 1
			 || xb.y() < 0 || xb.x() > 1) {
				//xform takes face outside of valid source region
				++stats.out_of_box;

				#ifdef DEBUG_ADD
				std::cerr << " -- out of box" << std::endl; //DEBUG
				#endif //DEBUG_ADD

				return false;
			}

			//ignore boundary edges
			if (
			    (xa.x() == 0 && xb.x() == 0)
			 || (xa.x() == 1 && xb.x() == 1)
			 || (xa.y() == 0 && xb.y() == 0)
			 || (xa.y() == 1 && xb.y() == 1)
			) {
				continue;
			}
			new_edges.emplace_back();
			new_edges.back().edge = face.edges[fe];
			assert(new_edges.back().edge < edges.size());
			auto const &edge = edges[new_edges.back().edge];
			assert(edge.a != -1U);

			K::Vector_2 out = (b - a).perpendicular(CGAL::CLOCKWISE); //face is in ccw order
			K::Vector_2 xout = xf[0] * out.x() + xf[1] * out.y();
			bool perp_is_out;
			{
				auto d = xout * (xb - xa).perpendicular(CGAL::COUNTERCLOCKWISE);
				auto len2 = (xb - xa) * (xb - xa);
				assert(d == len2 || d == -len2);
				perp_is_out = (d == len2);
			}
			if (edge.a == face_idx) {
				new_edges.back().a = xa;
				new_edges.back().b = xb;
				//new_edges.back().a_is_out = false; //adding 'a' side, so 'b' side must be out
				new_edges.back().perp_is_out = perp_is_out;
			} else {
				assert(edge.b == face_idx);
				new_edges.back().a = xb;
				new_edges.back().b = xa;
				//new_edges.back().a_is_out = true; //adding the 'b' side, so 'a' side must be out
				new_edges.back().perp_is_out = !perp_is_out;
			}
		}
		//check new edges vs existing active edges.

		//valid things:
		// -> xformed edges exactly overlap active edges (expect this to happen at least once)
		// -> xformed edges don't overlap any active edges
		// -> xformed edges intersect active edges at endpoints
		//invalid things:
		// -> xformed edges *cross* active edges
		// -> all xformed edges overlap existing face (this should be taken care of during input, as its hard to detect)
		// -> xformed edges overlap boundary edges (sigh, we forget boundary edges so it's hard to check)
		ns.active_edges.clear();
		ns.active_edges.reserve(new_edges.size() + us.active_edges.size());

		std::vector< bool > cancel_active(us.active_edges.size(), false);
		for (auto const &ne : new_edges) {
			bool cancel = false;
			for (auto const &ae : us.active_edges) {
				if ( (ae.a == ne.a && ae.b == ne.b) || (ae.a == ne.b && ae.b == ne.a) ) {
					if (ae.a == ne.a && ae.b == ne.b && ae.edge == ne.edge) {
						//perfect overlap, edges cancel.
						assert(!cancel_active[&ae - &us.active_edges[0]]);
						cancel_active[&ae - &us.active_edges[0]] = true;
						assert(!cancel);
						cancel = true;
						break; //assume that perfect overlap implies no further problems
					} else {
						++stats.imperfect_overlap;
						return false;
					}
				} else {
					auto result = CGAL::intersection(K::Segment_2(ae.a, ae.b), K::Segment_2(ne.a, ne.b));
					if (result) {
						const K::Point_2 *p = boost::get< K::Point_2 >(&*result);
						if (p) {
							if ((*p == ae.a || *p == ae.b) && (*p == ne.a || *p == ne.b)) {
								//intersects in a single endpoint, which is fine.
							} else {
								//intersects at a non-endpoint point
								++stats.intersection;
								return false;
							}
						} else {
							//intersects in a segment
							++stats.intersection;
							return false;
						}
					}
				}
			}
			if (!cancel) {
				ns.active_edges.emplace_back(ne);
			}
		}
		for (auto const &ae : us.active_edges) {
			if (!cancel_active[&ae - &us.active_edges[0]]) {
				ns.active_edges.emplace_back(ae);
			}
		}

		Key ns_key = ns.compute_key();
		//std::cout << ns_key << " from " << ns.source_key << std::endl; //DEBUG
		if (!states.count(ns_key)) {
			if (!new_states || new_states->insert(std::make_pair(ns_key, ns)).second) {
				++stats.added;

				#ifdef DEBUG_ADD
				show(show_lines);
				#endif


				//std::cout << "adding with: "<< ns.remaining_area << std::endl; //DEBUG

				if (ns.remaining_area == 0) {
					assert(ns.unused_faces.empty()); //otherwise would have bailed already

					#ifdef DEBUG_ADD
					std::cerr << " !! solution" << std::endl;
					show(show_lines, false);
					#endif //DEBUG_ADD

					report(ns);
				}

				return true;
			} else {
				#ifdef DEBUG_ADD
				std::cerr << " -- not very new" << std::endl;
				#endif //DEBUG_ADD
				++stats.repeat;
			}
		} else {
			#ifdef DEBUG_ADD
			std::cerr << " -- not new" << std::endl;
			#endif //DEBUG_ADD
			++stats.repeat;
		}
		return true; //still feasible if a repeat
	};



	{ //seed with all possible bottom-left edges:

		struct {
			uint32_t made = 0;
			uint32_t made_flipped = 0;
			uint32_t reflex = 0;
			uint32_t too_high = 0;
			uint32_t too_wide = 0;
			uint32_t not_corner = 0;
			uint32_t irrational = 0;
			void dump() {
				std::cerr << "When making " << made << " unflipped and " << made_flipped << " flipped seed states, discarded:\n"
					<< "  " << irrational << " were of irrational length\n"
					<< "  " << reflex << " because of non-convex facets\n"
					<< "  " << too_high << " that were too high\n"
					<< "  " << too_wide << " that were too wide\n"
					<< "  " << not_corner << " that were not the corner facet\n"
				;
				std::cerr.flush();
			}
		} seed_stats;
		for (uint32_t f = 0; f < faces.size(); ++f) {
			auto const &face = faces[f];
			assert(face.edges.size() == face.boundary.size());
			for (uint32_t fe = 0; fe < face.edges.size(); ++fe) {
				//std::cerr << "faces[" << f << "].edges[" << fe << "]:"; //DEBUG
				//Figure out how to map this edge to the x-axis.
				//note: an irrational-length edge can't appear on the boundary,
				// so if no mapping exists don't use this as a seed edge.
				auto const &a = face.boundary[fe];
				auto const &b = face.boundary[(fe + 1) % face.boundary.size()];
				//std::cerr << "  " << a << " to " << b << std::endl; //DEBUG
				K::Vector_2 dir(b-a);
				CGAL::Gmpq len2 = dir * dir;
				CGAL::Gmpz num2 = len2.numerator();
				CGAL::Gmpz den2 = len2.denominator();
				mpz_t num,den;
				mpz_init(num);
				mpz_init(den);
				//compute sqrt, if possible:
				if (mpz_root(num, num2.mpz(), 2) == 0 || mpz_root(den, den2.mpz(), 2) == 0) {
					mpz_clear(num);
					mpz_clear(den);
					++seed_stats.irrational;
					//std::cerr << "  (irrational)" << std::endl; //DEBUG
					continue;
				}
				dir = dir * CGAL::Gmpq(den, num); //normalize
				mpz_clear(num);
				mpz_clear(den);
				assert(dir * dir == 1);

				K::Vector_2 perp = dir.perpendicular(CGAL::COUNTERCLOCKWISE);

				//make sure this direction works:
				CGAL::Gmpq min_x = 0;
				CGAL::Gmpq max_x = 0;
				CGAL::Gmpq min_y = 0;
				CGAL::Gmpq max_y = 0;
				for (auto const &pt : face.boundary) {
					CGAL::Gmpq proj_x = dir * (pt - a);
					CGAL::Gmpq proj_y = perp * (pt - a);
					min_x = std::min(min_x, proj_x);
					max_x = std::max(max_x, proj_x);
					min_y = std::min(min_y, proj_y);
					max_y = std::max(max_y, proj_y);
				}
				//std::cerr << "    x: [" << min_x << ", " << max_x << "]" << std::endl; //DEBUG
				//std::cerr << "    y: [" << min_y << ", " << max_y << "]" << std::endl; //DEBUG
				if (min_y < 0) {
					//std::cerr << "  (reflex)" << std::endl; //DEBUG
					++seed_stats.reflex;
					continue;
				}
				if (max_y > 1) {
					//std::cerr << "  (too high)" << std::endl; //DEBUG
					++seed_stats.too_high;
					continue;
				}
				if (max_x - min_x > 1) {
					//std::cerr << "  (too wide)" << std::endl; //DEBUG
					++seed_stats.too_wide;
					continue;
				}

				bool corner = false;

				if (min_x == dir * (a - a)) {
					corner = true;
					//std::cerr << "  making [a->b]" << std::endl; //DEBUG
					
					K::Vector_2 to_dir(1,0);
					K::Vector_2 to_perp(0,1);
					assert(to_perp == to_dir.perpendicular(CGAL::COUNTERCLOCKWISE));
					K::Vector_2 to_a(0, 0); //this puts min point touching left edge

					K::Vector_2 xf[3];
					xf[0] = to_dir * dir.x() + to_perp * perp.x();
					xf[1] = to_dir * dir.y() + to_perp * perp.y();
					xf[2] = to_a - (xf[0] * a.x() + xf[1] * a.y());

					assert(CGAL::ORIGIN + xf[0] * a.x() + xf[1] * a.y() + xf[2] == K::Point_2(0,0));
					assert(CGAL::ORIGIN + xf[0] * dir.x() + xf[1] * dir.y() == K::Point_2(1,0));
					assert(CGAL::ORIGIN + xf[0] * perp.x() + xf[1] * perp.y() == K::Point_2(0,1));

					bool feasible = try_adding_face(root_key, root, f, xf, &states);
					assert(feasible);
					++seed_stats.made;
				}
				if (max_x == dir * (b - a)) {
					corner = true;
					//flipped-direction works for this edge
					//std::cerr << "  making [b->a]" << std::endl; //DEBUG
					//std::cerr << "   x range " << min_x << " "  << max_x << std::endl; //DEBUG
					//std::cerr << "   a: " << a << std::endl; //DEBUG
					//std::cerr << "   b: " << b << std::endl; //DEBUG

					K::Vector_2 to_dir(-1,0);
					K::Vector_2 to_perp(0,1);
					assert(to_perp == to_dir.perpendicular(CGAL::CLOCKWISE));
					K::Vector_2 to_b(0, 0); //this puts min point touching left edge

					K::Vector_2 xf[3];
					xf[0] = to_dir * dir.x() + to_perp * perp.x();
					xf[1] = to_dir * dir.y() + to_perp * perp.y();
					xf[2] = to_b - (xf[0] * b.x() + xf[1] * b.y());

					bool feasible = try_adding_face(root_key, root, f, xf, &states);
					assert(feasible);
					++seed_stats.made_flipped;
				}

				if (!corner) {
					//std::cerr << "  (not corner)" << std::endl; //DEBUG
					++seed_stats.not_corner;
				}
			}
		}
		//seed_stats.dump();
	}


	//always expands the thing with the smallest expand value...
	//Fill in area:
	typedef double ExpandValue;
	auto get_expand_value = [](UnrollState const &us) -> ExpandValue {
		return CGAL::to_double(us.remaining_area);
	};
#if 0
	//From lower left:
	typedef std::pair< double, double > ExpandValue;
	auto get_expand_value = [](UnrollState const &us) -> ExpandValue {
		ExpandValue ret(1.0, 1.0);
		auto lower = [&ret](K::Point_2 const &pt) {
			ExpandValue val(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
			if (val < ret) ret = val;
		};
		for (auto const &ae : us.active_edges) {
			lower(ae.a);
			lower(ae.b);
		}
		return ret;
	};
#endif

	//TODO: consider a priority queue
	std::multimap< ExpandValue, Key > to_expand;
	for (auto const &kv : states) {
		assert(kv.first != root_key); //root shouldn't be in states
		to_expand.insert(std::make_pair(get_expand_value(kv.second), kv.first));
	}


	while (!to_expand.empty()) {
		{
			static uint32_t count = 0;
			++count;
			if (count == 1000) {
				stats.dump();
				count = 0;
			}
		}
		Key key = to_expand.begin()->second;
		to_expand.erase(to_expand.begin());
		++stats.expanded;

		assert(states.count(key));
		UnrollState const &us = states.find(key)->second;

		if (us.active_edges.empty()) continue; //should have been report'd already?

		std::unordered_map< Key, UnrollState > fresh_states;

		//*check* that state is free along all edges, but only expand along *one* edge
		ActiveEdge const *expand_edge = &(us.active_edges[0]);
		{ //find the edge with the bottom-most left-most endpoint:
			K::Point_2 const *low_pt = &(expand_edge->a);

			auto better = [](K::Point_2 const &a, K::Point_2 const &b) {
				if (a.y() != b.y()) return a.y() < b.y();
				else return a.x() < b.x();
			};
			for (auto const &ae : us.active_edges) {
				K::Point_2 const *edge_pt = better(ae.a, ae.b) ? &ae.a : &ae.b;
				if (better(*edge_pt, *low_pt)) {
					low_pt = edge_pt;
					expand_edge = &ae;
				}
				//TODO: if multiple edges sort ccw or something?
			}
		}

		//try expanding along all edges:
		for (auto const &ae : us.active_edges) {

			decltype(fresh_states) *target = (&ae == expand_edge ? &fresh_states : nullptr);

			assert(ae.edge < edges.size());
			auto const &edge = edges[ae.edge];

			K::Vector_2 to_dir = ae.b - ae.a;
			K::Vector_2 to_out = to_dir.perpendicular(CGAL::COUNTERCLOCKWISE);
			if (!ae.perp_is_out) to_out = -to_out;

			CGAL::Gmpq len2 = to_dir * to_dir;
			assert(len2 > 0);
			assert(len2 == to_out * to_out);

			bool expanded = false;

			if (edge.a != -1U) {
				//can place face 'a'?
				assert(edge.a < faces.size());
				auto const &face = faces[edge.a];
				assert(edge.ae < face.boundary.size());
				K::Point_2 const &a = face.boundary[edge.ae];
				K::Point_2 const &b = face.boundary[(edge.ae+1)%face.boundary.size()];
				K::Vector_2 dir = b-a;
				K::Vector_2 out = dir.perpendicular(CGAL::COUNTERCLOCKWISE); //always move inside of 'a' to out direction

				K::Vector_2 xf[3];
				xf[0] = (to_dir * dir.x() + to_out * out.x()) / len2;
				xf[1] = (to_dir * dir.y() + to_out * out.y()) / len2;
				assert(xf[0] * xf[0] == 1);
				assert(xf[1] * xf[1] == 1);
				xf[2] = (ae.a - CGAL::ORIGIN) - (xf[0] * a.x() + xf[1] * a.y());

				assert(CGAL::ORIGIN + xf[0] * a.x() + xf[1] * a.y() + xf[2] == ae.a);
				assert(CGAL::ORIGIN + xf[0] * b.x() + xf[1] * b.y() + xf[2] == ae.b);
				assert(CGAL::ORIGIN + xf[0] * (b + out).x() + xf[1] * (b + out).y() + xf[2] == ae.b + to_out);

				if (try_adding_face(key, us, edge.a, xf, target)) {
					expanded = true;
				}
			}
			if (edge.b != -1U) {
				//can place face 'b'?
				assert(edge.b < faces.size());
				auto const &face = faces[edge.b];
				assert(edge.be < face.boundary.size());
				K::Point_2 const &a = face.boundary[(edge.be+1)%face.boundary.size()];
				K::Point_2 const &b = face.boundary[edge.be];
				K::Vector_2 dir = b-a;
				K::Vector_2 out = dir.perpendicular(CGAL::CLOCKWISE); //always move inside of 'b' to out direction

				K::Vector_2 xf[3];
				xf[0] = (to_dir * dir.x() + to_out * out.x()) / len2;
				xf[1] = (to_dir * dir.y() + to_out * out.y()) / len2;
				assert(xf[0] * xf[0] == 1);
				assert(xf[1] * xf[1] == 1);
				xf[2] = (ae.a - CGAL::ORIGIN) - (xf[0] * a.x() + xf[1] * a.y());

				assert(CGAL::ORIGIN + xf[0] * a.x() + xf[1] * a.y() + xf[2] == ae.a);
				assert(CGAL::ORIGIN + xf[0] * b.x() + xf[1] * b.y() + xf[2] == ae.b);
				assert(CGAL::ORIGIN + xf[0] * (b + out).x() + xf[1] * (b + out).y() + xf[2] == ae.b + to_out);

				if (try_adding_face(key, us, edge.b, xf, target)) {
					expanded = true;
				}
			}

			if (!expanded) {
				++stats.dead_ends;
				fresh_states.clear();
				break;
			}
		}
		for (auto const &kv : fresh_states) {
			states.insert(kv);
			to_expand.insert(std::make_pair(get_expand_value(kv.second), kv.first));
		}
	}

	stats.dump();

	std::cerr << "ERROR: shouldn't have finished without solving." << std::endl;
	return 1; //this *should* be complete, so it's weird.
}
