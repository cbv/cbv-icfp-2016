#pragma once

#include "base.hpp"
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
	*(out++) = min;
	*(out++) = min+x;
	*(out++) = min+x+y;
	*(out++) = min+y;
}

CGAL::Gmpq polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh);

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec);
