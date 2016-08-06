#include "Viz1.hpp"
#include "structures.hpp"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cout << "Usage:\n./show <solution|problem>\n (Will try to parse as solution and problem.)" << std::endl;
		return 1;
	}
	std::unique_ptr< Solution > solution = Solution::read(argv[1]);
	std::unique_ptr< Problem > problem = Problem::read(argv[1]);

	if (solution && problem) {
		std::cerr << "ERROR: That parsed as both a solution and a problem. Not sure how to deal with it." << std::endl;
		return 1;
	} else if (!solution && !problem) {
		std::cerr << "ERROR: That didn't parse as either a solution or a problem." << std::endl;
		return 1;
	}
/*
	std::string in_file, instructions_file, out_file;
	if (argc == 3) {
		in_file = "";
		instructions_file = argv[1];
		out_file = argv[2];
	} else if (argc == 4) {
		in_file = argv[1];
		instructions_file = argv[2];
		out_file = argv[3];
	} else {
		std::cout << "Usage:\n./foldup [in.solution] instructions.folds out.solution\n" << std::endl;
		return 1;
	}
*/

	auto to_pt = [](K::Point_2 const &pt) -> glm::vec2 {
		return glm::vec2(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
	};

	Viz1 viz(glm::uvec2(800, 400));
	viz.draw = [&](){
		glm::vec2 src_min(std::numeric_limits< float >::infinity());
		glm::vec2 src_max(-std::numeric_limits< float >::infinity());
		glm::vec2 dst_min(std::numeric_limits< float >::infinity());
		glm::vec2 dst_max(-std::numeric_limits< float >::infinity());
		if (solution) {
			for (auto const &pt_ : solution->source) {
				glm::vec2 pt = to_pt(pt_);
				src_min = glm::min(src_min, pt);
				src_max = glm::max(src_max, pt);
			}
			for (auto const &pt_ : solution->destination) {
				glm::vec2 pt = to_pt(pt_);
				dst_min = glm::min(dst_min, pt);
				dst_max = glm::max(dst_max, pt);
			}
		}

		src_max += glm::vec2(0.1f);
		src_min -= glm::vec2(0.1f);
		dst_max += glm::vec2(0.1f);
		dst_min -= glm::vec2(0.1f);

		float diameter = 2.0f;
		diameter = std::max(src_max.y - src_min.y, diameter);
		diameter = std::max(dst_max.y - dst_min.y, diameter);
		diameter = std::max((src_max.x - src_min.x) * 2.0f / viz.aspect, diameter);
		diameter = std::max((dst_max.x - dst_min.x) * 2.0f / viz.aspect, diameter);

		glClearColor(0.7f, 0.66f, 0.62f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glLoadIdentity();
		glScalef(2.0f / (viz.aspect * diameter), 2.0f / diameter, 1.0f);
		glTranslatef(0.25f * viz.aspect * diameter - 0.5f * (src_max.x + src_min.x), - 0.5f * (src_max.y + src_min.y), 0.0f);

		glBegin(GL_LINES);

		if (solution)
		for (auto const &f : solution->facets) {
			for (uint32_t i = 0; i < f.size(); ++i) {
				glm::vec2 a = to_pt(solution->source[f[i]]);
				glm::vec2 b = to_pt(solution->source[f[(i+1)%f.size()]]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glVertex2f(a.x, a.y);
				glVertex2f(b.x, b.y);
			}
		}
		glEnd();

		glLoadIdentity();
		glScalef(2.0f / (viz.aspect * diameter), 2.0f / diameter, 1.0f);
		glTranslatef(-0.25f * viz.aspect * diameter - 0.5f * (dst_max.x + dst_min.x), - 0.5f * (dst_max.y + dst_min.y), 0.0f);

		glBegin(GL_LINES);
		if (solution)
		for (auto const &f : solution->facets) {
			for (uint32_t i = 0; i < f.size(); ++i) {
				glm::vec2 a = to_pt(solution->destination[f[i]]);
				glm::vec2 b = to_pt(solution->destination[f[(i+1)%f.size()]]);
				glColor3f(0.0f, 0.0f, 0.0f);
				glVertex2f(a.x, a.y);
				glVertex2f(b.x, b.y);
			}
		}
		glEnd();

	};
	viz.run();

	return 0;
}
