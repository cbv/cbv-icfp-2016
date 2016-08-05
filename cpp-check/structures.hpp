#pragma once

#include <CGAL/Gmpq.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polygon_set_2.h>

#include <memory>

typedef CGAL::Cartesian< CGAL::Gmpq > K;

struct Problem {
	std::vector< std::vector< K::Point_2 > > silhouette;
	std::vector< std::pair< K::Point_2, K::Point_2 > > skeleton;

	CGAL::Polygon_set_2< K > get_silhouette() const;

	static std::unique_ptr< Problem > read(std::string const &filename);
};

struct Solution {
	std::vector< K::Point_2 > source;
	std::vector< std::vector< uint32_t > > facets;
	std::vector< K::Point_2 > destination;

	CGAL::Polygon_set_2< K > get_silhouette() const;

	bool is_valid() const;
	static std::unique_ptr< Solution > read(std::string const &filename);
};
