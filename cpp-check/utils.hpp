#pragma once

#include "base.hpp"
#include <CGAL/Polygon_with_holes_2.h>

K::Vector_2 rotate_left90 (const K::Vector_2 &vec);

CGAL::Gmpq polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh);

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec);
