#include "rotations.hpp"

#include <CGAL/convex_hull_2.h>

//vectors with non-negative X,Y components that are (exactly) unit length:
std::vector< K::Vector_2 > const & unit_vectors() {
	static std::vector< K::Vector_2 > ret;
	if (ret.empty()) {
		/* This supposedly will hit all triples, but it seems to produce a few gaps
		std::vector< std::tuple< CGAL::Gmpz, CGAL::Gmpz, CGAL::Gmpz > > triples;
		const uint_fast32_t Count = 3*100000 + 1;
		triples.reserve(Count);
		triples.emplace_back(3,4,5);
		for (uint_fast32_t i = 0; i < (Count-1)/3; ++i) {
			assert(i < triples.size());
			CGAL::Gmpz a = std::get< 0 >(triples[i]);
			CGAL::Gmpz b = std::get< 1 >(triples[i]);
			CGAL::Gmpz c = std::get< 2 >(triples[i]);
			assert(a*a + b*b == c*c);
			assert(a > 0 && b > 0);
			triples.emplace_back(
				1 * a + -2 * b + 2 * c,
				2 * a + -1 * b + 2 * c,
				2 * a + -2 * b + 3 * c
			);
			triples.emplace_back(
				1 * a +  2 * b + 2 * c,
				2 * a +  1 * b + 2 * c,
				2 * a +  2 * b + 3 * c
			);
			triples.emplace_back(
				-1 * a + 2 * b + 2 * c,
				-2 * a + 1 * b + 2 * c,
				-2 * a + 2 * b + 3 * c
			);
		}
		assert(triples.size() == Count);

		std::vector< K::Vector_2 > vecs;
		vecs.reserve(triples.size()+2);
		for (auto const &t : triples) {
			CGAL::Gmpz const &a = std::get< 0 >(t);
			CGAL::Gmpz const &b = std::get< 1 >(t);
			CGAL::Gmpz const &c = std::get< 2 >(t);
			assert(a*a + b*b == c*c);
			vecs.emplace_back(CGAL::Gmpq(a, c), CGAL::Gmpq(b, c));
			assert(vecs.back() * vecs.back() == 1);
		}
		vecs.emplace_back(1,0);
		vecs.emplace_back(0,1);
		assert(vecs.size() == triples.size()+2);
		*/
		//N.b. might work better with a better starting triple.
		//lazier version based on lots of rotating:
		std::vector< K::Vector_2 > vecs;
		vecs.emplace_back(0,1);
		vecs.emplace_back(1,0);
		vecs.reserve(1002);
		while (vecs.size() < 1000) {
			auto const &v = vecs.back();
			K::Vector_2 perp(-v.y(), v.x());
			K::Vector_2 next = (v * 4 + perp * 3) / 5;
			if (next.x() < 0 || next.y() < 0) {
				next = K::Vector_2(-next.y(), next.x());
			}
			if (next.x() < 0 || next.y() < 0) {
				next = K::Vector_2(-next.y(), next.x());
			}
			if (next.x() < 0 || next.y() < 0) {
				next = K::Vector_2(-next.y(), next.x());
			}
			assert(next.x() >= 0 && next.y() >= 0);
			vecs.emplace_back(next);
			assert(vecs.back() * vecs.back() == 1);
		}


		//sort in counterclockwise order:
		std::sort(vecs.begin(), vecs.end(), [](K::Vector_2 const &a, K::Vector_2 const &b){
			return a.x() > b.x();
		});
		ret.reserve(vecs.size());

		CGAL::Gmpq thresh(std::cos(1.0f / 360.0f * M_PI));
		uint_fast32_t over = 0;
		for (auto const &v : vecs) {
			if (!ret.empty() && ret.back() * v < thresh) ++over;
			if (ret.size() >= 2 && ret[ret.size()-2] * v > thresh) {
				ret.back() = v;
			} else {
				ret.emplace_back(v);
			}
		}
		std::cerr << "Had " << over << " gaps with over threshold angle, made " << ret.size() << " angles." << std::endl;
	}
	return ret;
}
