//Note: requires CGAL and GMP (for big rationals)
// ... that makes everything else ~easy like cake~
//Usage ./check problem solution

#include <iostream>
#include <memory>
#include <fstream>
#include <unordered_set>

#include "structures.hpp"

CGAL::Gmpq check_solution(Problem const &problem, Solution const &solution) {
	//To check this solution we'll use CGAL's polygon set functionality
	
	CGAL::Polygon_set_2< K > a = problem.get_silhouette();
	CGAL::Polygon_set_2< K > b = solution.get_silhouette();

	CGAL::Polygon_set_2< K > a_and_b;
	CGAL::Polygon_set_2< K > a_or_b;
	a_and_b.intersection(a,b);
	a_or_b.join(a,b);

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

	auto and_area = polygon_set_area(a_and_b);
	auto or_area = polygon_set_area(a_or_b);

	return and_area / or_area;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		std::cerr << "Usage:\n\t./check <problem> <solution>\n" << std::endl;
		return 1;
	}

	std::unique_ptr< Problem > problem = Problem::read(argv[1]);
	if (!problem) {
		std::cerr << "Failed to read problem." << std::endl;
		return 1;
	}
	std::cerr << "Read problem with " << problem->silhouette.size() << " silhouette polygons and " << problem->skeleton.size() << " skeleton edges." << std::endl;

	std::unique_ptr< Solution > solution = Solution::read(argv[2]);
	if (!solution) {
		std::cerr << "Failed to read solution." << std::endl;
		return 1;
	}

	std::cerr << "Read solution with " << solution->source.size() << " vertices and " << solution->facets.size() << " facets." << std::endl;

	//Check solution for consistency with specification:
	if (!solution->is_valid()) {
		std::cerr << "Solution isn't valid. Stopping." << std::endl;
		return 1;
	}

	//Intersect solution with problem silhouette:
	auto score = check_solution(*problem, *solution);

	std::cerr << "Score: " << score << " ~= " << CGAL::to_double(score) << std::endl;

	return 0;
}
