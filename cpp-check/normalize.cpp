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

	solution = solution.normalized();

	auto refl = [&solution](){
		for (auto &facet : solution) {
			for (auto &pt : facet.source) {
				pt = K::Point_2(1 - pt.x(), pt.y());
			}
			facet.compute_xf();
		}
	};
	auto rot90 = [&solution](){
		for (auto &facet : solution) {
			for (auto &pt : facet.source) {
				pt = K::Point_2(1 - pt.y(), pt.x());
			}
			facet.compute_xf();
		}
	};

	std::string best = "";
	uint32_t best_count = 999999;
	auto test = [&solution, &best, &best_count]() {
		std::ostringstream out;
		solution.print_solution(out);

		std::string str = out.str();
		uint32_t count = 0;
		for (char c : str) {
			if (isgraph(c)) ++count;
		}
		//std::cerr << "Count is " << count << std::endl;
		if (count < best_count) {
			best_count = count;
			best = str;
		}
	};

	test();
	rot90(); test();
	rot90(); test();
	rot90(); test();
	refl();
	rot90(); test();
	rot90(); test();
	rot90(); test();
	rot90(); test();

	//output the result:
	if (argc == 3) {
		std::cerr << "   (writing to " << argv[2] << ")" << std::endl;
		std::ofstream file(argv[2]);
		file << best;
	} else {
		std::cout << best;
	}

	return 0;
}
