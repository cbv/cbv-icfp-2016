#include "utils.hpp"
#include "structures.hpp"
#include "rotations.hpp"
#include "folders.hpp"

#include <CGAL/convex_hull_2.h>
#include <CGAL/Boolean_set_operations_2.h>

/* fold_dest already does this, weirdly enough:
K::Segment_2 extend (const K::Segment_2& seg) {
	K::Vector_2 vec = seg.to_vector();
	K::FT bound, diff, scale;
	bound = 10;
	if (seg.is_vertical()) {
		diff = seg.max().y() - seg.min().y();
	} else {
		diff = seg.max().x() - seg.min().x();
	}
	scale = bound / diff;
	std::cerr << "Scaling " << seg << " by " << scale << "." << std::endl;
	return K::Segment_2(seg.source() - vec * scale, seg.source() + vec * scale);
}
*/

bool fold_excess (State& state, const CGAL::Polygon_2<K>& goal) {
	uint32_t old_facets = state.size();
	for (auto ei = goal.edges_begin(); ei != goal.edges_end(); ++ei) {
		//the goal is ccw oriented, so fold with reverse of edge, since fold flips 'left-of' stuff onto right:
		state.fold_dest(ei->target(), ei->source());
	}
	assert(old_facets <= state.size());
	return old_facets != state.size();
}

template<typename OutputIterator>
void make_centered_square (const K::Vector_2 &x, const K::Point_2 &center, OutputIterator out) {
	K::Vector_2 y = prep(x);
	*(out++) = center-x/2-y/2;
	*(out++) = center+x/2-y/2;
	*(out++) = center+x/2+y/2;
	*(out++) = center-x/2+y/2;
}

int main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cerr << "Usage:\n\t./get-convex <problem> [solution-to-write]\n" << std::endl;
		return 1;
	}

	std::unique_ptr< Problem > problem = Problem::read(argv[1]);
	if (!problem) {
		std::cerr << "Failed to read problem." << std::endl;
		return 1;
	}
	std::cerr << "Read problem with " << problem->silhouette.size() << " silhouette polygons and " << problem->skeleton.size() << " skeleton edges." << std::endl;

	//Idea: find closest bounding rectangle of silhouette, then fold that rectangle.

	std::vector< K::Point_2 > points;
	for (auto const &poly : problem->silhouette) {
		points.insert(points.end(), poly.begin(), poly.end());
	}

	CGAL::Polygon_2<K> hull;
	CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter( hull ));
	std::cerr << "Hull has " << hull.size() << " points." << std::endl;

	// get directions
	std::vector< K::Vector_2 > x_dirs = unit_vectors();
	

	{ //add directions that can be made rational:
		uint_fast32_t exact = 0;
		uint_fast32_t inexact = 0;
		for (auto ei = hull.edges_begin(); ei != hull.edges_end(); ++ei) {
			auto pair = pythagorean_unit_approx(ei->to_vector());
			if (pair.first) {
				++exact;
			} else {
				++inexact;
				std::cerr << "Approximate " << *ei << " by " << pair.second << "." << std::endl;
			}
			x_dirs.emplace_back(pair.second);
		}
		std::cerr << "Addded " << exact << " exact directions and " << inexact << " inexact directions." << std::endl;
	}

	auto get_score = [&hull,&problem](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> CGAL::Gmpq {
		CGAL::Polygon_2< K > square;
		{ //build square from min/max:
			auto center = min + (max - min)/2;
			make_centered_square(x, center, std::back_inserter(square));
			assert(square.orientation() == CGAL::COUNTERCLOCKWISE);
		}
		assert(hull.orientation() == CGAL::COUNTERCLOCKWISE);
		CGAL::Polygon_set_2<K> final_shape;
		final_shape.join(hull); final_shape.intersection(square);
		return problem->get_score(final_shape);
	};


	auto get_fold = [&hull](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> State {
		//std::cerr << "Running get_fold for the inputs x=" << x << " and min=" << min << " and max=" << max << "." << std::endl;
		auto center = min + (max - min)/2;
		Facet square;
		make_centered_square(K::Vector_2(1,0),K::Point_2(1/2,1/2),back_inserter(square.source));
		make_centered_square(x,center,back_inserter(square.destination));
		//std::cerr << "The source is " << CGAL::Polygon_2<K>(square.source.begin(),square.source.end()) << "." << std::endl;
		//std::cerr << "The destination is " << CGAL::Polygon_2<K>(square.destination.begin(),square.destination.end()) << "." << std::endl;
		square.compute_xf();
		State state;
		state.push_back (square);
		// std::cerr << "Ready to fold:" << std::endl;
		uint_fast32_t counter = 0;
		while (fold_excess(state, hull)) {
			++counter;
			std::cerr << "Folded " << counter << " times." << std::endl;
		}
		std::cerr << "Folded " << counter << " times in total." << std::endl;
		return state;
	};

	CGAL::Gmpq best_score = 0;
	struct {
		K::Vector_2 x;
		K::Point_2 min, max;
	} best;
	bool exact_found = false;
	for (auto pass : {'e', '~'}) //in first pass, look for an exact wrapping, in second pass check approximate
	for (auto const &x_dir : x_dirs) {
		assert(x_dir * x_dir == 1);
		K::Vector_2 y_dir = x_dir.perpendicular(CGAL::COUNTERCLOCKWISE);

		CGAL::Gmpq min_x = x_dir * K::Vector_2(hull[0].x(), hull[0].y());
		CGAL::Gmpq max_x = min_x;
		CGAL::Gmpq min_y = y_dir * K::Vector_2(hull[0].x(), hull[0].y());
		CGAL::Gmpq max_y = min_y;
		for (auto vi = hull.vertices_begin(); vi != hull.vertices_end(); ++vi) {
			CGAL::Gmpq x = x_dir * (*vi - CGAL::ORIGIN);
			CGAL::Gmpq y = y_dir * (*vi - CGAL::ORIGIN);
			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
		}
		bool fits = true;
		if (max_x > min_x + 1) {
			auto c = (max_x + min_x) / 2;
			min_x = c - CGAL::Gmpq(1,2);
			max_x = c + CGAL::Gmpq(1,2);
			fits = false;
		}
		if (max_y > min_y + 1) {
			auto c = (max_y + min_y) / 2;
			min_y = c - CGAL::Gmpq(1,2);
			max_y = c + CGAL::Gmpq(1,2);
			fits = false;
		}
		if (fits) {
			best.x = x_dir;
			best.min = K::Point_2(min_x, min_y);
			best.max = K::Point_2(max_x, max_y);
			//this should be as good as it can be -- it should cover the hull.
			exact_found = true;
			break;
		} else if (pass == '~') {
			auto score = get_score(x_dir, K::Point_2(min_x, min_y), K::Point_2(max_x, max_y));
			if (score > best_score) {
				best_score = score;
				best.x = x_dir;
				best.min = K::Point_2(min_x, min_y);
				best.max = K::Point_2(max_x, max_y);
			}
		}
	}

	if (exact_found) {
		std::cerr << "Found exact wrapping. This is as good as we can do." << std::endl;
	} else {
		std::cerr << "Using approximate wrapping." << std::endl;
	}
	State best_wrap = get_fold(best.x, best.min, best.max);
	if (argc == 3) {
		std::cerr << "(Writing to " << argv[2] << ")" << std::endl;
		std::ofstream file(argv[2]);
		best_wrap.print_solution(file);
	} else {
		best_wrap.print_solution(std::cout);
	}

	return 0;
}
