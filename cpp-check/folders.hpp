#include "structures.hpp"
#include <vector>

struct Facet {
	std::vector< K::Point_2 > source;
	std::vector< K::Point_2 > destination;

	//transform from source->destination as 2x3 (column major) matrix:
	K::Vector_2 xf[3] = {
		K::Vector_2(1,0),
		K::Vector_2(0,1),
		K::Vector_2(0,0)
	};
	//for convenience (can be computed from xf):
	bool flipped = false;

	void compute_xf();
};

struct State : public std::vector< Facet > {
        // return true if folding happened
	bool fold_dest(K::Point_2 const &a, K::Point_2 const &b);
	void unfold();
	void refold(std::vector< K::Point_2 > const &marks);
};
