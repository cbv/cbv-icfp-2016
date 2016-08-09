#pragma once

#include "base.hpp"
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>

inline K::Vector_2 p2v(K::Point_2 const &p) {
	return p - CGAL::ORIGIN;
}

inline K::Vector_2 prep (const K::Vector_2 &vec) {
	return vec.perpendicular(CGAL::COUNTERCLOCKWISE);
}

inline size_t count_non_whitespace (std::string &str) {
	size_t count = 0;
	for (auto c : str) {if (!(::isspace(c))) {count++;}}
	return count;
}

inline size_t vector_bit_complexity (const K::Vector_2 &vec) {
	return vec.x().numerator().bit_size() + vec.x().denominator().bit_size()
		+ vec.y().numerator().bit_size() + vec.y().denominator().bit_size();
}

template<typename OutputIterator>
inline void insert_square (const K::Vector_2 &x, const K::Point_2 &min, OutputIterator out) {
	K::Vector_2 y = prep(x);
	auto min_point = CGAL::ORIGIN + min.x() * x + min.y() * y;
	*(out++) = min_point;
	*(out++) = min_point+x;
	*(out++) = min_point+x+y;
	*(out++) = min_point+y;
}

K::FT polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh);
K::FT polygon_set_area (CGAL::Polygon_set_2< K > const &ps);

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec);
