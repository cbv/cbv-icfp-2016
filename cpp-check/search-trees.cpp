//search trees tries to unfold a problem by building a source from the facets of the skeleton.

#include "structures.hpp"

#include <CGAL/Arrangement_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_curve_data_traits_2.h>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n./search-trees <problem>" << std::endl;
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

	}


	return 0;
}
