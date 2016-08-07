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
	assert(hull.orientation() == CGAL::COUNTERCLOCKWISE);

	// get directions
	std::vector< K::Vector_2 > x_dirs = unit_vectors();

	// get directions from the convex hull
	std::cerr << "Adding edges in the convex hull..." << std::endl;
	{
		uint_fast32_t exact = 0;
		uint_fast32_t inexact = 0;
		for (auto ei = hull.edges_begin(); ei != hull.edges_end(); ++ei) {
			auto pair = pythagorean_unit_approx(ei->to_vector());
			if (pair.first) {
				++exact;
				x_dirs.emplace(x_dirs.begin(), pair.second);
			} else {
				++inexact;
				std::cerr << "Approximate " << ei->to_vector() << " by " << pair.second << "." << std::endl;
				x_dirs.emplace_back(pair.second);
			}
		}
		std::cerr << "Addded " << exact << " exact directions and " << inexact << " inexact directions." << std::endl;
	}

	// sort the directions
	std::sort(x_dirs.begin(), x_dirs.end(), [](K::Vector_2 const &a, K::Vector_2 const &b){
		return vector_bit_complexity(a) < vector_bit_complexity(b);
	});

	auto get_score = [&hull,&problem](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> CGAL::Gmpq {
		CGAL::Polygon_2< K > square;
		{
			insert_square(x, min, std::back_inserter(square));
			assert(square.orientation() == CGAL::COUNTERCLOCKWISE);
		}
		CGAL::Polygon_set_2<K> final_shape;
		final_shape.join(hull); final_shape.intersection(square);
		return problem->get_score(final_shape);
	};


	auto get_fold = [&hull](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> State {
		Facet square;
		insert_square(K::Vector_2(1,0), CGAL::ORIGIN, back_inserter(square.source));
		insert_square(x, min, back_inserter(square.destination));
		square.compute_xf();
		State state;
		state.push_back (square);
		uint_fast32_t counter = 0;
		while (fold_excess(state, hull)) {
			++counter;
#ifndef NDEBUG
			std::cerr << "Folded " << counter << " times." << std::endl;
#endif
		}
#ifndef NDEBUG
		std::cerr << "Folded " << counter << " times in total." << std::endl;
#endif
		return state;
	};

	auto output = [&argv, argc] (std::string &str) -> void {
		if (argc == 3) {
			std::cerr << "  (Writing to " << argv[2] << ")" << std::endl;
			std::ofstream file(argv[2]);
			file << str;
		} else {
			std::cout << str;
		}
	};

	uint32_t best_count = UINT32_MAX;
	CGAL::Gmpq best_score = 0;
	std::string best_solution;
	// TODO do the exact/approximate phases
	for (auto const &x_dir : x_dirs) {
		assert(x_dir * x_dir == 1);
		K::Vector_2 y_dir = prep(x_dir);

		CGAL::Gmpq min_x = x_dir * p2v(hull[0]);
		CGAL::Gmpq max_x = min_x;
		CGAL::Gmpq min_y = y_dir * p2v(hull[0]);
		CGAL::Gmpq max_y = min_y;
		for (auto vi = hull.vertices_begin(); vi != hull.vertices_end(); ++vi) {
			CGAL::Gmpq x = x_dir * p2v(*vi);
			CGAL::Gmpq y = y_dir * p2v(*vi);
			if (x < min_x) min_x = x;
			if (x > max_x) max_x = x;
			if (y < min_y) min_y = y;
			if (y > max_y) max_y = y;
		}
		if (max_x > min_x + 1) {
			auto c = (max_x + min_x) / 2;
			min_x = c - CGAL::Gmpq(1,2);
			max_x = c + CGAL::Gmpq(1,2);
		}
		if (max_y > min_y + 1) {
			auto c = (max_y + min_y) / 2;
			min_y = c - CGAL::Gmpq(1,2);
			max_y = c + CGAL::Gmpq(1,2);
		}
		auto score = get_score(x_dir, K::Point_2(min_x, min_y), K::Point_2(max_x, max_y));
		if (score >= best_score && score > 0) {
			if (score > best_score) {
				best_score = score;
				best_count = UINT32_MAX;
			}

			auto state = get_fold(x_dir, K::Point_2(min_x, min_y), K::Point_2(max_x, max_y)).normalized();
			std::string solution;
			{
				std::ostringstream out;
				state.print_solution(out);
				solution = out.str();
			}
			uint32_t count = count_non_whitespace(solution);
			/*
			// > solution_size_limit) {
#ifndef NDEBUG
				std::cerr << "Found a good but long solution of length " << count_non_whitespace(solution) << "." << std::endl;
#endif
				continue;
			}*/
			if (count < best_count) {
				std::cerr << "Best score so far: " << score;
				std::cerr << " ~= " << std::fixed << std::setprecision(7) << score.to_double() << std::endl;
				std::cerr << " (in " << count << " non-whitespace characters)" << std::endl;

				if (score == 1 || count <= solution_size_limit) {
					output (best_solution);
				}

				best_count = count;
				best_solution = solution;
				if (score == 1 && count < solution_size_limit) break;
			}
		}
	}

	return 0;
}
