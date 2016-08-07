//search trees tries to unfold a problem by building a source from the facets of the skeleton.

#include "structures.hpp"
#include "folders.hpp"

#include <unordered_map>


#ifdef VIZ

#include "Viz1.hpp"

inline glm::vec2 to_glm(K::Point_2 const &pt) {
	return glm::vec2(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
};


void show(std::vector< std::tuple< glm::vec2, glm::vec2, glm::u8vec4 > > const &lines, bool pause = true) {
	static Viz1 viz(glm::uvec2(400, 400));
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	glm::vec2 min(std::numeric_limits< float >::infinity());
	glm::vec2 max(-std::numeric_limits< float >::infinity());
	for (auto const &line : lines) {
		min = glm::min(min, std::get< 0 >(line));
		min = glm::min(min, std::get< 1 >(line));
		max = glm::max(max, std::get< 0 >(line));
		max = glm::max(max, std::get< 1 >(line));
	}

	viz.draw = [&](){
		float diameter = 2.0f;
		diameter = std::max(max.y - min.y, diameter);
		diameter = std::max((max.x - min.x) * 2.0f / viz.aspect, diameter);

		glClearColor(0.62f, 0.66f, 0.65f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLoadIdentity();
		glScalef(2.0f / (viz.aspect * diameter), 2.0f / diameter, 1.0f);
		glTranslatef(-0.5f * (max.x + min.x), -0.5f * (max.y + min.y), 0.0f);

		glBegin(GL_LINES);
		for (auto const &line : lines) {
			glColor4ub(std::get< 2 >(line).r, std::get< 2 >(line).g, std::get< 2 >(line).b, std::get< 2 >(line).a);
			glVertex2f(std::get< 0 >(line).x, std::get< 0 >(line).y);
			glVertex2f(std::get< 1 >(line).x, std::get< 1 >(line).y);
		}
		glEnd();

		if (!pause) viz.stop_flag = true;

	};
	viz.handle_event = [&](SDL_Event const &e) {
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
			exit(1);
		}
		if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
			viz.stop_flag = true;
		}
	};

	viz.stop_flag = false;
	viz.run();
}

#endif //VIZ


int main(int argc, char **argv) {
	if (argc != 2 && argc != 3) {
		std::cerr << "Usage:\n./normalize <solution> [file-to-write]" << std::endl;
		return 1;
	}
	std::unique_ptr< Solution > input = Solution::read(argv[1]);
	if (!input) {
		std::cerr << "ERROR: Failed to read solution." << std::endl;
		return 1;
	}

	State solution;
	solution.reserve(input->facets.size());
	for (auto const &f : input->facets) {
		solution.emplace_back();
		for (uint32_t idx : f) {
			solution.back().source.emplace_back(input->source[idx]);
			solution.back().destination.emplace_back(input->destination[idx]);
		}
		solution.back().compute_xf();
	}

	//now merge adjacent facets with the same xf:


	//helper:
	auto merge = [](Facet const &f1, uint32_t e1, Facet const &f2, uint32_t e2) -> Facet {
		Facet merged;
		assert(f1.xf[0] == f2.xf[0]);
		assert(f1.xf[1] == f2.xf[1]);
		assert(f1.xf[2] == f2.xf[2]);
		assert(f1.flipped == f2.flipped);
		merged.xf[0] = f1.xf[0];
		merged.xf[1] = f1.xf[1];
		merged.xf[2] = f1.xf[2];
		merged.flipped = f1.flipped;
		merged.source.reserve(f1.source.size() + f2.source.size());

		uint32_t c1 = (e1 + 1) % f1.source.size();
		while (c1 != e1) {
			merged.source.emplace_back(f1.source[c1]);
			c1 = (c1+1) % f1.source.size();
		}

		uint32_t c2 = (e2 + 1) % f2.source.size();
		while (c2 != e2) {
			merged.source.emplace_back(f2.source[c2]);
			c2 = (c2+1) % f2.source.size();
		}

		//now de-duplicate anything that looks like [..., a, b, a, ...]

		uint32_t s = 0;
		while (s < merged.source.size()) {
			assert(merged.source.size() > 3);
			auto const &a = merged.source[s];
			//auto const &b = merged.source[(s+1)%merged.source.size()];
			auto const &c = merged.source[(s+2)%merged.source.size()];
			if (a == c) {
				uint32_t i1 = (s+1)%merged.source.size();
				uint32_t i2 = (s+2)%merged.source.size();
				if (i2 < i1) std::swap(i1, i2);
				assert(i1 < i2);
				merged.source.erase(merged.source.begin() + i2);
				merged.source.erase(merged.source.begin() + i1);
				if (i1 < s) s -= 1;
				if (i2 < s) s -= 1;
				assert(s < merged.source.size());
			} else {
				//go to next pattern:
				++s;
			}
		}

		merged.destination.reserve(merged.source.size());
		for (auto const &s : merged.source) {
			merged.destination.emplace_back( CGAL::ORIGIN + merged.xf[0] * s.x() + merged.xf[1] * s.y() + merged.xf[2] );
		}

		return merged;
	};

	//make all facets ccw:
	uint32_t flipped = 0;
	for (auto &f : solution) {
		CGAL::Polygon_2< K > poly(f.source.begin(), f.source.end());
		if (poly.orientation() != CGAL::COUNTERCLOCKWISE) {
			++flipped;
			std::reverse(f.source.begin(), f.source.end());
			std::reverse(f.destination.begin(), f.destination.end());
		}
	}
	std::cerr << "Flipped " << flipped << " CW facets." << std::endl;


	uint32_t merges = 0;
	//(iterative quadratic all-pairs way; could use hash to make faster)
	bool progress = true;
	while (progress) {
		progress = false;

		for (auto const &f1 : solution) {
			for (auto const &f2 : solution) {
				if (&f2 >= &f1) break; //avoid doing pairs twice
				if (f1.xf[0] != f2.xf[0]) continue; //different xform
				if (f1.xf[1] != f2.xf[1]) continue;
				if (f1.xf[2] != f2.xf[2]) continue;
				//check for adjacent edge:
				for (uint32_t e1 = 0; e1 < f1.source.size(); ++e1) {
					auto const &a1 = f1.source[e1];
					auto const &b1 = f1.source[(e1+1)%f1.source.size()];
					for (uint32_t e2 = 0; e2 < f2.source.size(); ++e2) {
						auto const &a2 = f2.source[e2];
						auto const &b2 = f2.source[(e2+1)%f2.source.size()];

						if (a1 == a2 && b1 == b2) {
							std::cerr << "ERROR: same edge in same order in two facets." << std::endl;
							return 1;
						}
						if (!(a1 == b2 && b1 == a2)) continue;

						++merges;

						//okay, have an edge to merge along
						Facet new_facet = merge(f1, e1, f2, e2);

						uint32_t i1 = &f1 - &solution[0];
						uint32_t i2 = &f2 - &solution[0];
						assert(i2 < i1);
						solution.erase(solution.begin() + i1);
						solution.erase(solution.begin() + i2);
						solution.emplace_back(new_facet);

						progress = true;
						break;
					}
					if (progress) break;
				}
				if (progress) break;
			}
			if (progress) break;
		}

	}

	std::cerr << "Merged " << merges << " facets." << std::endl;


	//output the result:
	if (argc == 3) {
		std::cerr << "   (writing to " << argv[2] << ")" << std::endl;
		solution.print_solution(argv[2]);
	} else {
		solution.print_solution(std::cout);
	}

	return 0;
}
