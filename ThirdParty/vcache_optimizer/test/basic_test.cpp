#include <iostream>
#include <string>
#include <vcache_optimizer/vcache_optimizer.hpp>
#include <vcache_optimizer/stdout_callback.hpp>


struct mytriangle
{
	unsigned int indices[3];

	explicit mytriangle() {}

	explicit mytriangle(unsigned int const i1, unsigned int const i2, unsigned int const i3)
	{
		indices[0] = i1;
		indices[1] = i2;
		indices[2] = i3;
	}

	unsigned int operator[](int const index) const
	{
		return indices[index];
	}
};


struct myvtx
{
	std::string value;

	myvtx() {}
	myvtx(std::string const &value): value(value) {}
};


struct mymesh
{
	typedef std::vector < mytriangle > triangles_t;
	typedef std::vector < myvtx > vertices_t;

	triangles_t triangles;
	vertices_t vertices;
};


namespace vcache_optimizer
{


template < >
struct mesh_traits < mymesh >
{
	typedef std::string submesh_id_t;
	typedef myvtx vertex_t;
	typedef mytriangle triangle_t;
	typedef unsigned int vertex_index_t;
	typedef unsigned int triangle_index_t;
};


}


mytriangle create_new_triangle(mymesh &, unsigned int const i1, unsigned int const i2, unsigned int const i3)
{
	return mytriangle(i1, i2, i3);
}


std::size_t get_num_triangles(mymesh const &m, std::string const &)
{
	return m.triangles.size();
}

std::size_t get_num_vertices(mymesh const &m, std::string const &)
{
	return m.vertices.size();
}

mytriangle get_triangle(mymesh const &m, std::string const &, unsigned int const &index)
{
	return m.triangles[index];
}

myvtx get_vertex(mymesh const &m, std::string const &, unsigned int const &index)
{
	return m.vertices[index];
}

void set_triangle(mymesh &m, std::string const &, unsigned int const &index, mytriangle const &new_triangle)
{
	m.triangles[index] = new_triangle;
}

void set_vertex(mymesh &m, std::string const &, unsigned int const &index, myvtx const &new_vertex)
{
	m.vertices[index] = new_vertex;
}



void print(mymesh const &mesh)
{
	std::cout << "vertices:";
	for (std::vector < myvtx > ::const_iterator iter = mesh.vertices.begin(); iter != mesh.vertices.end(); ++iter)
		std::cout << ' ' << iter->value;
	std::cout << std::endl;

	std::cout << "triangles:";
	for (std::vector < mytriangle > ::const_iterator iter = mesh.triangles.begin(); iter != mesh.triangles.end(); ++iter)
		std::cout << " [ " << (*iter)[0] << ' ' << (*iter)[1] << ' ' << (*iter)[2] << " ]";
	std::cout << std::endl;
	std::cout << "          ";
	for (std::vector < mytriangle > ::const_iterator iter = mesh.triangles.begin(); iter != mesh.triangles.end(); ++iter)
		std::cout << " [ " << mesh.vertices[(*iter)[0]].value << ' ' << mesh.vertices[(*iter)[1]].value << ' ' << mesh.vertices[(*iter)[2]].value << " ]";
	std::cout << std::endl;
}


int main()
{
	vcache_optimizer::vcache_optimizer < mymesh > optimizer;

	mymesh mesh;
	mesh.triangles.push_back(mytriangle(0, 1, 6));
	mesh.triangles.push_back(mytriangle(0, 2, 3));
	mesh.triangles.push_back(mytriangle(4, 2, 3));
	mesh.triangles.push_back(mytriangle(5, 1, 4));
	mesh.triangles.push_back(mytriangle(2, 3, 5));
	mesh.vertices.push_back(myvtx("a"));
	mesh.vertices.push_back(myvtx("b"));
	mesh.vertices.push_back(myvtx("c"));
	mesh.vertices.push_back(myvtx("d"));
	mesh.vertices.push_back(myvtx("e"));
	mesh.vertices.push_back(myvtx("f"));
	mesh.vertices.push_back(myvtx("g"));

	std::cout << "=========== BEFORE OPTIMIZATION ===========\n";
	print(mesh);

	std::cout << "===========================================\n";

	vcache_optimizer::stdout_callback cb; 
	optimizer(mesh, "", cb);

	std::cout << "=========== AFTER OPTIMIZATION ===========\n";
	print(mesh);

	return 0;
}

