#include <fstream>
#include <vcache_optimizer/vcache_optimizer.hpp>
#include <vcache_optimizer/stdout_callback.hpp>
#include "ply_mesh.hpp"



int main()
{
	vcache_optimizer::vcache_optimizer < ply_mesh > optimizer;

	char const *ply_file = "ply_viewer/Armadillo.ply";

	std::ifstream in_file(ply_file);
	if (!in_file.good())
	{
		std::cerr << "could not read " << ply_file << " - are you in the wrong directory?\n";
		return -1;
	}

	ply_mesh mesh(in_file);

	vcache_optimizer::stdout_callback cb; 
	optimizer(mesh, "", cb);

	return 0;
}

