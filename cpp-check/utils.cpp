#include "base.hpp"
#include "utils.hpp"

#include <CGAL/Gmpq.h>
#include <CGAL/Polygon_set_2.h>

K::FT polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh) {
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
}

K::FT polygon_set_area (CGAL::Polygon_set_2< K > const &ps) {
	std::list< CGAL::Polygon_with_holes_2< K > > res;
	ps.polygons_with_holes( std::back_inserter( res ) );
	CGAL::Gmpq ret = 0;
	for (auto const &poly : res) {
		ret += polygon_with_holes_area(poly);
	}
	assert(ret >= 0);
	return ret;
}

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec) {
	CGAL::Gmpq len2 = vec * vec;
	CGAL::Gmpz scaled_len2 = len2.numerator() * len2.denominator();
	mpz_t scaled_len;
	mpz_init(scaled_len);
	if (mpz_root(scaled_len, scaled_len2.mpz(), 2)) {
		// exact
		CGAL::Gmpq len (scaled_len, len2.denominator());
		mpz_clear(scaled_len);
		return std::make_pair(true, vec / len);
	} else {
		// inexact
		mpz_clear(scaled_len);
		auto x_abs = CGAL::abs(vec.x());
		auto y_abs = CGAL::abs(vec.y());
		bool x_is_short = x_abs < y_abs;
		auto short_abs = x_is_short ? x_abs : y_abs;
		double len_approx = std::sqrt(len2.to_double());
		auto m_over_n = CGAL::Gmpq(std::sqrt (2 / (1 - short_abs.to_double() / len_approx) - 1));
		auto m = m_over_n.numerator();
		auto n = m_over_n.denominator();
		auto new_short = m*m - n*n;
		auto new_long = 2*m*n;
		auto new_hypo = m*m + n*n;
		auto new_x = vec.x().sign() * (x_is_short ? new_short : new_long); 
		auto new_y = vec.y().sign() * (x_is_short ? new_long : new_short); 
		return std::make_pair(false, K::Vector_2(new_x, new_y, new_hypo));
	}
}
