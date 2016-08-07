#include "Viz1.hpp"
#include "structures.hpp"

#include <CGAL/Partition_traits_2.h>
#include <CGAL/Triangle_2.h>
#include <CGAL/partition_2.h>

inline
glm::vec2 to_pt (K::Point_2 const &pt) {
	return glm::vec2 (CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
}

inline
void mark_pt (K::Point_2 const &pt) {
	glVertex2f (CGAL::to_double(pt.x()), CGAL::to_double(pt.y()));
}

inline
void draw_segment_2pt (K::Point_2 const &pt1, K::Point_2 const &pt2, float r, float g, float b, float a=1.0f) {
	glBegin(GL_LINES);
	glColor4f(r, g, b, a);
	mark_pt(pt1);
	mark_pt(pt2);
        glEnd();
}

inline
void draw_segment (K::Segment_2 const& seg, float r, float g, float b, float a=1.0f) {
	draw_segment_2pt (seg[0], seg[1], r, g, b, a);
}

inline
void draw_poly_edges (CGAL::Polygon_2<K> const& poly, float r, float g, float b, float a=1.0f) {
	for (auto ei = poly.edges_begin(); ei != poly.edges_end(); ++ei) {
		draw_segment (*ei, r, g, b, a);
	}
}

inline
void draw_triangle_inner (K::Triangle_2 const& tri, float r, float g, float b, float a=1.0f) {
	glBegin(GL_TRIANGLES);
        glColor4f(r, g, b, a);
	mark_pt(tri[0]);
	mark_pt(tri[1]);
	mark_pt(tri[2]);
        glEnd();
}

inline
void draw_convex_poly_inner (CGAL::Polygon_2<K> const& poly, float r, float g, float b, float a=1.0f) {
	assert (poly.is_counterclockwise_oriented());
	assert (poly.is_simple());
	assert (poly.is_convex());
	for (size_t i = 1; i + 1 < poly.size(); i++) {
		draw_triangle_inner (K::Triangle_2(poly[0], poly[i], poly[i+1]), r, g, b, a);
	}
}

inline
void draw_poly_inner (CGAL::Polygon_2<K> const& poly, float r, float g, float b, float a=1.0f) {
	assert (poly.is_counterclockwise_oriented());
	assert (poly.is_simple());
	std::vector<CGAL::Polygon_2<K> > convex_partition;
	CGAL::approx_convex_partition_2(poly.vertices_begin(),
		poly.vertices_end(),
		std::back_inserter(convex_partition));
	for (auto const &poly : convex_partition) {
		draw_convex_poly_inner(poly, r, g, b, a);
	}
}

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n./show <solution|problem>\n (Will try to parse as solution and problem.)" << std::endl;
		return 1;
	}
	std::unique_ptr< Solution > solution;
	try {
		solution = Solution::read(argv[1]);
	} catch (...) { }
	std::unique_ptr< Problem > problem;
	try {
		problem = Problem::read(argv[1]);
	} catch (...) { }

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
		std::cerr << "Usage:\n./foldup [in.solution] instructions.folds out.solution\n" << std::endl;
		return 1;
	}
*/

	Viz1 viz(glm::uvec2(800, 400));
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

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
		if (problem) {
			for (auto const &poly : problem->silhouette) {
				for (auto const &pt_ : poly) {
					glm::vec2 pt = to_pt(pt_);
					src_min = glm::min(src_min, pt);
					src_max = glm::max(src_max, pt);
				}
			}
			for (auto const &line : problem->skeleton) {
				glm::vec2 a = to_pt(line.first);
				glm::vec2 b = to_pt(line.second);
				src_min = glm::min(src_min, a);
				src_min = glm::min(src_min, b);
				src_max = glm::max(src_max, a);
				src_max = glm::max(src_max, b);
			}
			dst_min = src_min;
			dst_max = src_max;
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

		if (solution)
		for (auto const &f : solution->facets) {
			CGAL::Polygon_2<K> poly;
			for (auto const &i : f) {
				poly.push_back(solution->source[i]);
			}
			draw_poly_edges (poly, 0., 0., 0.);
			draw_poly_inner (poly, 0., 0., 0., 0.3);
		}
		if (problem) {
			for (auto const &pts : problem->silhouette) {
				CGAL::Polygon_2<K> poly(pts.begin(), pts.end());
				draw_poly_edges (poly, 0., 0., 0.);
				if (poly.is_counterclockwise_oriented())
					draw_poly_inner (poly, 0., 0., 0., 0.3);
			}
		}

		glLoadIdentity();
		glScalef(2.0f / (viz.aspect * diameter), 2.0f / diameter, 1.0f);
		glTranslatef(-0.25f * viz.aspect * diameter - 0.5f * (dst_max.x + dst_min.x), - 0.5f * (dst_max.y + dst_min.y), 0.0f);

		glBegin(GL_LINES);
		if (solution)
		for (auto const &f : solution->facets) {
			CGAL::Polygon_2<K> poly;
			for (auto const &i : f) {
				poly.push_back(solution->destination[i]);
			}
			draw_poly_edges (poly, 0., 0., 0.);
			draw_poly_inner (poly, 0., 0., 0., 0.3);
		}

		if (problem) {
			for (auto const &line : problem->skeleton) {
				draw_segment_2pt(line.first, line.second, 1.0f, 1.0f, 1.0f, 0.5f);
			}

			for (auto const &pts : problem->silhouette) {
				CGAL::Polygon_2<K> poly(pts.begin(), pts.end());
				draw_poly_edges (poly, 0., 0., 0.);
				if (poly.is_counterclockwise_oriented())
					draw_poly_inner (poly, 0., 0., 0., 0.1);
			}

		}
		glEnd();

	};
	viz.run();

	return 0;
}
