//Idea:
// foldup in.solution in.folds out.solution
// takes a bunch of folds as input
//
// starts with a network of edges and a folding direction for each edge.
// builds folded example from network.

//Multiple ideas:
//1) work with creases in source domain only
//2) work with creases in folded domain
//3) work from creases and directions

#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "structures.hpp"
#include "folders.hpp"

State make_square() {
	State ret;
	ret.clear();
	ret.emplace_back();
	ret.back().source.emplace_back(0,0);
	ret.back().source.emplace_back(1,0);
	ret.back().source.emplace_back(1,1);
	ret.back().source.emplace_back(0,1);
	ret.back().destination = ret.back().source;
	ret.back().compute_xf();
	assert(ret.back().flipped == false);
	return ret;
}

int main(int argc, char **argv) {
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

	State state;
	if (argc == 4) {
		std::unique_ptr< Solution > soln = Solution::read(in_file);
		if (!soln) {
			std::cerr << "Error reading starting state from '" << in_file << "'." << std::endl;
			return 1;
		}
		std::cout << "Starting with the folded state of '" << in_file << "'." << std::endl;
		for (auto const &facet : soln->facets) {
			Facet f;
			for (auto index : facet) {
				f.source.emplace_back(soln->source[index]);
				f.destination.emplace_back(soln->source[index]);
			}
			f.compute_xf();
			state.emplace_back(f);
		}
	} else {
		std::cout << "Starting with a square." << std::endl;
		state = make_square();
	}

	{
		std::vector< K::Point_2 > marks;
		std::cout << "Applying instructions from '" << instructions_file << "'." << std::endl;
		std::ifstream inst(instructions_file);
		std::string line;
		while (std::getline(inst, line)) {
			std::cout <<  "> " << line << std::endl;
			for (uint32_t i = 0; i < line.size(); ++i) {
				if (line[i] == '#') {
					line = line.substr(0, i);
					break;
				}
			}
			std::istringstream str(line);
			std::string tok;
			if (!(str >> tok)) continue;
			if (tok == "fold") {
				CGAL::Gmpq x1,y1,x2,y2;
				char comma1, comma2;
				if (!(str >> x1 >> comma1 >> y1 >> x2 >> comma2 >> y2) || comma1 != ',' || comma2 != ',') {
					std::cerr << "For 'fold' instruction, expecting x1,y1 x2,y2" << std::endl;
					return 1;
				}
				state.fold_dest(K::Point_2(x1, y1), K::Point_2(x2, y2));
				std::cout << "  after fold, have " << state.size() << " facets." << std::endl;
			} else if (tok == "unfold") {
				state.unfold();
				marks.clear();
			} else if (tok == "mark") {
				CGAL::Gmpq x,y;
				char comma;
				if (!(str >> x >> comma >> y) || comma != ',') {
					std::cerr << "For 'mark' instruction, expecting x,y" << std::endl;
					return 1;
				}
				marks.emplace_back(x,y);
			} else if (tok == "refold") {
				if (marks.empty()) {
					std::cerr << "WARNING: refolding with no marks." << std::endl;
				}
				if (!state.refold(marks)) {
					std::cerr << "WARNING: refolding failed." << std::endl;
				}
				marks.clear();
			} else {
				std::cerr << "ERROR: unknown folding instruction '" << tok << "'." << std::endl;
			}
		}
	}

	std::cout << "Now have " << state.size() << " facets." << std::endl;

	auto pp = [](CGAL::Gmpq const &q) -> std::string {
		std::ostringstream str;
		str << q.numerator();
		if (q.denominator() != 1) {
			str << "/" << q.denominator();
		}
		return str.str();
	};

	//DEBUG:
	for (auto const &facet : state) {
		std::cout << "   ------\n";
		for (uint32_t i = 0; i < facet.source.size(); ++i) {
			std::string src_name = pp(facet.source[i].x()) + "," + pp(facet.source[i].y());
			std::string dst_name = pp(facet.destination[i].x()) + "," + pp(facet.destination[i].y());
			std::cout << "     " << src_name << " -> " << dst_name << "\n";
		}
		std::cout.flush();
	}
	std::cout << "------\n";

	//print out solution -- need to de-duplicate verts for this.
	std::unordered_map< std::string, uint32_t > source_inds;
	std::vector< std::string > source_verts;
	std::vector< std::string > destination_verts;
	std::ostringstream facet_info;
	for (auto const &facet : state) {
		assert(facet.source.size() == facet.destination.size());
		facet_info << facet.source.size();
		for (uint32_t i = 0; i < facet.source.size(); ++i) {
			std::string src_name = pp(facet.source[i].x()) + "," + pp(facet.source[i].y());
			std::string dst_name = pp(facet.destination[i].x()) + "," + pp(facet.destination[i].y());
			uint32_t idx = source_inds.insert(std::make_pair(src_name, source_inds.size())).first->second;
			if (idx == source_verts.size()) {
				source_verts.emplace_back(src_name);
				destination_verts.emplace_back(dst_name);
			}
			assert(source_verts.size() == destination_verts.size());
			assert(idx < source_verts.size());
			assert(src_name == source_verts[idx]);
			assert(idx < destination_verts.size());
			assert(dst_name == destination_verts[idx]);

			facet_info << " " << idx;
		}
		facet_info << "\n";
	}

	std::ostringstream out;
	out << source_verts.size() << "\n";
	for (auto v : source_verts) {
		out << v << "\n";
	}
	out << state.size() << "\n";
	out << facet_info.str();
	for (auto v : destination_verts) {
		out << v << "\n";
	}

	std::cout << out.str();

	{
		std::ofstream soln(out_file);
		soln << out.str();
	}


	return 0;
}
