#include "structures.hpp"
#include "rotations.hpp"
#include "folders.hpp"

#include <CGAL/convex_hull_2.h>

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
	std::cout << "Scaling " << seg << " by " << scale << "." << std::endl;
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

	auto get_score = [&hull_poly,&problem](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> CGAL::Gmpq {
		CGAL::Polygon_set_2< K > a = problem->get_silhouette();
		CGAL::Polygon_2< K > b;

		{ //build square from min/max:
			K::Vector_2 y(-x.y(), x.x());
			b.push_back(K::Point_2(0,0) + min.x() * x + min.y() * y);
			b.push_back(K::Point_2(0,0) + max.x() * x + min.y() * y);
			b.push_back(K::Point_2(0,0) + max.x() * x + max.y() * y);
			b.push_back(K::Point_2(0,0) + min.x() * x + max.y() * y);
			assert(b.orientation() == CGAL::COUNTERCLOCKWISE);
		}
		assert(hull_poly.orientation() == CGAL::COUNTERCLOCKWISE);

		CGAL::Polygon_set_2< K > a_and_b;
		CGAL::Polygon_set_2< K > a_or_b;
		a_and_b.join(hull_poly);
		a_and_b.intersection(b);

		a_and_b.intersection(a);


		a_or_b.join(hull_poly);
		a_or_b.intersection(b);

		a_or_b.join(a);


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
	};


	auto get_fold = [&hull_poly](K::Vector_2 const &x, K::Point_2 const &min, K::Point_2 const &max) -> State {
		//std::cout << "Running get_fold for the inputs x=" << x << " and min=" << min << " and max=" << max << "." << std::endl;
		K::Vector_2 y = x.perpendicular(CGAL::COUNTERCLOCKWISE);
		Facet square;
		square.source = {K::Point_2(0,0), K::Point_2(1,0), K::Point_2(1,1), K::Point_2(0,1)};
		square.destination = {min, min+x, min+x+y, min+y};
		//std::cout << "The source is " << CGAL::Polygon_2<K>(square.source.begin(),square.source.end()) << "." << std::endl;
		//std::cout << "The destination is " << CGAL::Polygon_2<K>(square.destination.begin(),square.destination.end()) << "." << std::endl;
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
		for (auto const &pt : hull) {
			CGAL::Gmpq x = x_dir * K::Vector_2(pt.x(), pt.y());
			CGAL::Gmpq y = y_dir * K::Vector_2(pt.x(), pt.y());
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
		std::cout << "Found exact wrapping. This is as good as we can do." << std::endl;
	} else {
		std::cout << "Using approximate wrapping." << std::endl;
	}
	State best_wrap = get_fold(best.x, best.min, best.max);
	if (argc == 3) {
		std::cout << "(Writing to " << argv[2] << ")" << std::endl;
		std::ofstream file(argv[2]);
		best_wrap.print_solution(file);
	} else {
		best_wrap.print_solution(std::cout);
	}

	return 0;
}
