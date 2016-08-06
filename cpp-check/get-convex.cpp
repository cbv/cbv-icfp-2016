#include "structures.hpp"
#include "rotations.hpp"
#include "folders.hpp"

#include <CGAL/convex_hull_2.h>

K::Segment_2 extend (const K::Segment_2& seg) {
  K::Vector_2 vec = seg.to_vector();
  K::FT bound, diff, scale;
  bound = 10;
  if (seg.is_vertical()) {
    diff = seg.min().y() - seg.max().y();
  } else {
    diff = seg.max().x() - seg.max().x();
  }
  scale = bound / diff;
  return K::Segment_2(seg.source() - vec * scale, seg.source() + vec * scale);
}

bool fold_excess (State& state, const CGAL::Polygon_2<K>& goal) {
  for (auto ei = goal.edges_begin(); ei != goal.edges_end(); ++ei) {
    auto cut = extend(*ei);
    bool excess = state.fold_dest(cut.source(), cut.target());
    if (excess) return true;
  }
  return false;
}

int main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cout << "Usage:\n\t./get-rect <problem> [solution-to-write]\n" << std::endl;
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

	std::cout << "Hull has " << hull.size() << " points." << std::endl;

	//Given the way that the score is computed (and_area / or_area)
	//it might actually make sense in some cases to avoid covering a portion of the figure.
	//particularly, in cases where the amount of and_area [as a fraction of total] is decreased less than the amount of or_area [as a fraction of total]
	
	//lazy version: ignore all of that.

	std::vector< K::Vector_2 > x_dirs = unit_vectors();
	

	{ //add directions that can be made rational:
		uint_fast32_t added = 0;
		uint_fast32_t skipped = 0;
		for (uint_fast32_t e = 0; e < hull.size(); ++e) {
			auto const &a = hull[e];
			auto const &b = hull[(e+1)%hull.size()];
			K::Vector_2 dir(b-a);
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

	auto get_fold = [&hull](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> State {
	  K::Vector_2 y(-x.y(), x.x());
    K::Point_2 mid = min + (max - min) / 2;
    K::Point_2 midpoint = CGAL::ORIGIN + mid.x() * x + mid.y() * y;
    Facet square;
    square.source = {K::Point_2(0,0), K::Point_2(1,0), K::Point_2(1,1), K::Point_2(0,1)};
    square.destination = {midpoint-x/2-y/2, midpoint+x/2-y/2, midpoint+x/2+y/2, midpoint-x/2+y/2};
    State state;
    state.push_back (square);
    CGAL::Polygon_2<K> hull_polygon = CGAL::Polygon_2<K>(hull.begin(), hull.end());
    while (fold_excess(state, hull_polygon));
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
		K::Vector_2 y_dir(-x_dir.y(), x_dir.x());

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
