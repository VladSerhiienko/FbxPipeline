#ifndef PLY_MESH_HPP_10237956243754885149242708574559517001____
#define PLY_MESH_HPP_10237956243754885149242708574559517001____


//
//  ply_mesh.hpp - PLY mesh loader class
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <iostream>
#include <vector>
#include <stdint.h>

#include <vcache_optimizer/mesh_traits.hpp>


class ply_mesh
{
public:
	struct vertex
	{
		float position[3];

		explicit vertex() {}

		explicit vertex(float const x, float const y, float const z)
		{
			position[0] = x;
			position[1] = y;
			position[2] = z;
		}
	};


	struct triangle
	{
		uint32_t indices[3];

		explicit triangle() {}

		explicit triangle(uint32_t const i1, uint32_t const i2, uint32_t const i3)
		{
			indices[0] = i1;
			indices[1] = i2;
			indices[2] = i3;
		}


		uint32_t operator[](int const index) const
		{
			return indices[index];
		}
	};


	typedef std::vector < vertex > vertices_t;
	typedef std::vector < triangle > triangles_t;


	explicit ply_mesh(std::istream &in);
	~ply_mesh();


	vertices_t const & get_vertices() const;
	triangles_t const & get_triangles() const;
	vertices_t & get_vertices();
	triangles_t & get_triangles();


protected:
	struct callback_data;
	callback_data *callbacks;
};





//////////////
// Typedefs and free functions for letting the ply_mesh conform to the VcacheMesh concept
//////////////


namespace vcache_optimizer
{

template < >
struct mesh_traits < ply_mesh >
{
	typedef std::string submesh_id_t;
	typedef ply_mesh::vertex vertex_t;
	typedef ply_mesh::triangle triangle_t;
	typedef uint32_t vertex_index_t;
	typedef unsigned long triangle_index_t;
};

}

ply_mesh::triangle create_new_triangle(ply_mesh &m, uint32_t const i1, uint32_t const i2, uint32_t const i3);
std::size_t get_num_triangles(ply_mesh const &m, std::string const &);
std::size_t get_num_vertices(ply_mesh const &m, std::string const &);
ply_mesh::triangle get_triangle(ply_mesh const &m, std::string const &, unsigned long const &index);
ply_mesh::vertex get_vertex(ply_mesh const &m, std::string const &, uint32_t const &index);
void set_triangle(ply_mesh &m, std::string const &, unsigned long const &index, ply_mesh::triangle const &new_triangle);
void set_vertex(ply_mesh &m, std::string const &, uint32_t const &index, ply_mesh::vertex const &new_vertex);


#endif

