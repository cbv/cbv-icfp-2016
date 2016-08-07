#pragma once

#include "base.hpp"

#include <CGAL/Polygon_set_2.h>

const size_t solution_size_limit = 5000;

struct Problem {
	std::vector< std::vector< K::Point_2 > > silhouette;
	std::vector< std::pair< K::Point_2, K::Point_2 > > skeleton;

	CGAL::Polygon_set_2< K > get_silhouette() const;

	static std::unique_ptr< Problem > read(std::string const &filename);

        K::FT get_score (const CGAL::Polygon_set_2<K>&) const;
        K::FT get_score1 (const CGAL::Polygon_2<K>&) const;
};

struct Solution {
	std::vector< K::Point_2 > source;
	std::vector< std::vector< uint_fast32_t > > facets;
	std::vector< K::Point_2 > destination;

	CGAL::Polygon_set_2< K > get_silhouette() const;

	bool is_valid() const;
	static std::unique_ptr< Solution > read(std::string const &filename);
};
