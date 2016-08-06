#pragma once

#include <CGAL/Polygon_with_holes_2.h>

CGAL::Gmpq polygon_with_holes_area (CGAL::Polygon_with_holes_2< K > const &pwh);

std::pair<bool, CGAL::Vector_2<K>> pythagorean_unit_approx (CGAL::Vector_2< K > const &vec);
