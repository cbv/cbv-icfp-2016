#pragma once

#include "base.hpp"
#include <CGAL/Polygon_with_holes_2.h>

inline K::Vector_2 p2v(K::Point_2 const &p) {
	return p - CGAL::ORIGIN;
}

inline K::Vector_2 prep (const K::Vector_2 &vec) {
	return vec.perpendicular(CGAL::COUNTERCLOCKWISE);
}

CGAL::Gmpq polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh);

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec);
