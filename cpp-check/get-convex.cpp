#include "structures.hpp"
#include "rotations.hpp"
#include "folders.hpp"

#include <CGAL/convex_hull_2.h>

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
  std::cout << "Scaling " << seg << " by " << scale << "." << std::endl;
  return K::Segment_2(seg.source() - vec * scale, seg.source() + vec * scale);
}

bool fold_excess (State& state, const CGAL::Polygon_2<K>& goal) {
  for (auto ei = goal.edges_begin(); ei != goal.edges_end(); ++ei) {
    // TODO precompute far_endpoints
    auto far_endpoints = extend(*ei);
    bool excess = state.fold_dest(far_endpoints.source(), far_endpoints.target());
    if (excess) {
      std::cout << "Something is folded by the endpoints " << far_endpoints << "." << std::endl;
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cout << "Usage:\n\t./get-convex <problem> [solution-to-write]\n" << std::endl;
		return 1;
	}

	std::unique_ptr< Problem > problem = Problem::read(argv[1]);
	if (!problem) {
		std::cerr << "Failed to read problem." << std::endl;
		return 1;
	}
	std::cout << "Read problem with " << problem->silhouette.size() << " silhouette polygons and " << problem->skeleton.size() << " skeleton edges." << std::endl;

	//Idea: find closest bounding rectangle of silhouette, then fold that rectangle.

	std::vector< K::Point_2 > points;
	for (auto const &poly : problem->silhouette) {
		points.insert(points.end(), poly.begin(), poly.end());
	}

	std::vector< K::Point_2 > hull;
	CGAL::convex_hull_2(points.begin(), points.end(), std::back_inserter( hull ));
  CGAL::Polygon_2<K> hull_poly(hull.begin(), hull.end());
	std::cout << "Hull has " << hull.size() << " points." << std::endl;

  // get directions
	std::vector< K::Vector_2 > x_dirs = unit_vectors();
	

	{ //add directions that can be made rational:
		uint_fast32_t added = 0;
		uint_fast32_t skipped = 0;
    for (auto ei = hull_poly.edges_begin(); ei != hull_poly.edges_end(); ++ei) {
			K::Vector_2 dir = ei->to_vector();
			CGAL::Gmpq len2 = dir * dir;
			CGAL::Gmpz num2 = len2.numerator();
			CGAL::Gmpz den2 = len2.denominator();
			mpz_t num,den;
			mpz_init(num);
			mpz_init(den);
			if (mpz_root(num, num2.mpz(), 2) != 0 && mpz_root(den, den2.mpz(), 2)) {
				x_dirs.emplace_back((dir / CGAL::Gmpz(num)) * CGAL::Gmpz(den));
				++added;
			} else {
				++skipped;
			}
			mpz_clear(num);
			mpz_clear(den);
		}
		std::cout << "Addded " << added << " edge directions to test directions, skipped " << skipped << "." << std::endl;
	}

	auto get_fold = [&hull_poly](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> State {
    std::cout << "Running get_fold for the inputs x=" << x << " and min=" << min << " and max=" << max << "." << std::endl;
	  K::Vector_2 y = x.perpendicular(CGAL::COUNTERCLOCKWISE);
    K::Point_2 mid = min + (max - min) / 2;
    K::Point_2 midpoint = CGAL::ORIGIN + mid.x() * x + mid.y() * y;
    Facet square;
    square.source = {K::Point_2(0,0), K::Point_2(1,0), K::Point_2(1,1), K::Point_2(0,1)};
    square.destination = {midpoint-x/2-y/2, midpoint+x/2-y/2, midpoint+x/2+y/2, midpoint-x/2+y/2};
    std::cout << "The source is " << CGAL::Polygon_2<K>(square.source.begin(),square.source.end()) << "." << std::endl;
    std::cout << "The destination is " << CGAL::Polygon_2<K>(square.destination.begin(),square.destination.end()) << "." << std::endl;
    square.compute_xf();
    State state;
    state.push_back (square);
    // std::cout << "Ready to fold:" << std::endl;
    uint_fast32_t counter = 0;
    while (fold_excess(state, hull_poly)) {
      ++counter;
		  std::cout << "Folded " << counter << " times." << std::endl;
    }
		std::cout << "Folded " << counter << " times in total." << std::endl;
    state.print_solution(std::cout); //DEBUG
    // TODO calculate the score
    return state;
  };
    
  // TODO score
	// CGAL::Gmpq best_score = 0;
	struct {
		K::Vector_2 x = K::Vector_2(0,0);
		K::Point_2 min = K::Point_2(0,0); //relative to x, perp(x)
		K::Point_2 max = K::Point_2(0,0); //relative to x, perp(x)
	} best;
	for (auto const &x_dir : x_dirs) {
		assert(x_dir * x_dir == 1);
		K::Vector_2 y_dir = x_dir.perpendicular(CGAL::COUNTERCLOCKWISE);

		CGAL::Gmpq min_x = x_dir * K::Vector_2(hull[0].x(), hull[0].y());
		CGAL::Gmpq max_x = min_x;
		CGAL::Gmpq min_y = y_dir * K::Vector_2(hull[0].x(), hull[0].y());
		CGAL::Gmpq max_y = min_y;
		for (auto const &pt : hull) {
			CGAL::Gmpq x = x_dir * K::Vector_2(pt.x(), pt.y());
			CGAL::Gmpq y = y_dir * K::Vector_2(pt.x(), pt.y());
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
		State score = get_fold(x_dir, K::Point_2(min_x, min_y), K::Point_2(max_x, max_y));
/*
    if (score > best_score) {
			best_score = score;
			best.x = x_dir;
			best.min = K::Point_2(min_x, min_y);
			best.max = K::Point_2(max_x, max_y);
		}
*/
	}

  // TODO print best solutions

//	std::cout << " --- solution (score = " << best_score << ") --- \n" << out.str();

	if (argc == 3) {
		std::cout << "(Writing to " << argv[2] << ")" << std::endl;
		std::ofstream file(argv[2]);
		// file << out.str();
	}


	return 0;
}
