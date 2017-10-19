//
//  ply_mesh.cpp - PLY mesh loader class
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include "ply_mesh.hpp"
#include <cmath>
#include <tr1/functional>
#include <tr1/tuple>
#include <limits>
#include <ply/ply_parser.hpp>


struct ply_mesh::callback_data
{
	vertices_t vertices;
	triangles_t triangles;
	vertex cur_vtx;
	unsigned int face_indices_elem_index;
	ply::int32 first_index, second_index;
	float pos_min[3], pos_max[3];


	explicit callback_data()
	{
		clear();
	}

	void clear()
	{
		vertices.clear();
		triangles.clear();
		pos_min[0] = pos_min[1] = pos_min[2] = +std::numeric_limits < float > ::infinity();
		pos_max[0] = pos_max[1] = pos_max[2] = -std::numeric_limits < float > ::infinity();
	}


	std::tr1::tuple < std::tr1::function < void() >, std::tr1::function < void() > > element_definition_callback(std::string const & element_name, std::size_t)
	{
		if (element_name == "vertex")
		{
			return std::tr1::tuple < std::tr1::function < void() >, std::tr1::function < void() > > (
				std::tr1::bind(&ply_mesh::callback_data::begin_vertex, this),
				std::tr1::bind(&ply_mesh::callback_data::end_vertex, this)
				);
		}
		else
		{
			return std::tr1::tuple < std::tr1::function < void() >, std::tr1::function < void() > > (0, 0);
		}
	}


	std::tr1::tuple <
		std::tr1::function < void(ply::uint8) >,
		std::tr1::function < void(ply::int32) >,
		std::tr1::function < void() >
	> list_property_definition_callback(std::string const & element_name, std::string const& property_name)
	{
		using namespace std::tr1::placeholders;

		std::cout << element_name << " " << property_name << std::endl;


		if ((element_name == "face") && ((property_name == "vertex_indices") || (property_name == "vertex_index")) )
		{
			std::cout << "ply: found faces" << std::endl;
			return std::tr1::tuple <
				std::tr1::function < void (ply::uint8) >,
				std::tr1::function < void (ply::int32) >,
				std::tr1::function < void () >
				> (
					std::tr1::bind(&ply_mesh::callback_data::begin_face_indices, this, _1),
					std::tr1::bind(&ply_mesh::callback_data::face_indices_element, this, _1),
					std::tr1::bind(&ply_mesh::callback_data::end_face_indices, this)
				);
		}
		else {
			return std::tr1::tuple<std::tr1::function < void(ply::uint8)>, std::tr1::function < void(ply::int32)>, std::tr1::function < void() > > (0, 0, 0);
		}
	}


	std::tr1::function < void(ply::float32) > scalar_property_definition_callback(std::string const & element_name, std::string const & property_name)
	{
		using namespace std::tr1::placeholders;


		if (element_name == "vertex")
		{
			if (property_name == "x")
			{
				return std::tr1::bind(&ply_mesh::callback_data::vertex_pos_x, this, _1);
			}
			else if (property_name == "y")
			{
				return std::tr1::bind(&ply_mesh::callback_data::vertex_pos_y, this, _1);
			}
			else if (property_name == "z")
			{
				return std::tr1::bind(&ply_mesh::callback_data::vertex_pos_z, this, _1);
			}
			else
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}


	void begin_vertex()
	{
	}

	void vertex_pos_x(ply::float32 x)
	{
		if (pos_min[0] > x) pos_min[0] = x;
		if (pos_max[0] < x) pos_max[0] = x;
		cur_vtx.position[0] = x;
	}

	void vertex_pos_y(ply::float32 y)
	{
		if (pos_min[1] > y) pos_min[1] = y;
		if (pos_max[1] < y) pos_max[1] = y;
		cur_vtx.position[1] = y;
	}

	void vertex_pos_z(ply::float32 z)
	{
		if (pos_min[2] > z) pos_min[2] = z;
		if (pos_max[2] < z) pos_max[2] = z;
		cur_vtx.position[2] = z;
	}

	void end_vertex()
	{
		vertices.push_back(cur_vtx);
	}

	void begin_face_indices(ply::uint8)
	{
		face_indices_elem_index = 0;
	}

	void face_indices_element(ply::int32 vertex_index)
	{
		switch (face_indices_elem_index)
		{
			case 0:
				first_index = vertex_index;
				break;
			case 1:
				second_index = vertex_index;
				break;
			default:
			{
				triangle tri;
				tri.indices[0] = first_index;
				tri.indices[1] = second_index;
				tri.indices[2] = vertex_index;
				triangles.push_back(tri);
				second_index = vertex_index;
			}
		};

		++face_indices_elem_index;
	}

	void end_face_indices()
	{
	}
};



ply_mesh::ply_mesh(std::istream &in)
{
	callbacks = new callback_data();

	using namespace std::tr1::placeholders;

	ply::ply_parser ply_parser;

	ply_parser.element_definition_callback(std::tr1::bind(&ply_mesh::callback_data::element_definition_callback, callbacks, _1, _2));

	ply::ply_parser::scalar_property_definition_callbacks_type scalar_property_definition_callbacks;
	ply::at < ply::float32 > (scalar_property_definition_callbacks) = std::tr1::bind(&ply_mesh::callback_data::scalar_property_definition_callback , callbacks, _1, _2);
	ply_parser.scalar_property_definition_callbacks(scalar_property_definition_callbacks);

	ply::ply_parser::list_property_definition_callbacks_type list_property_definition_callbacks;
	ply::at < ply::uint8, ply::int32 > (list_property_definition_callbacks) = std::tr1::bind(&ply_mesh::callback_data::list_property_definition_callback, callbacks, _1, _2);
	ply_parser.list_property_definition_callbacks(list_property_definition_callbacks);

	ply_parser.parse(in);

	float extents[3] = {
		callbacks->pos_max[0] - callbacks->pos_min[0],
		callbacks->pos_max[1] - callbacks->pos_min[1],
		callbacks->pos_max[2] - callbacks->pos_min[2]
	};

	float center[3] = {
		(callbacks->pos_max[0] + callbacks->pos_min[0]) / 2.0f,
		(callbacks->pos_max[1] + callbacks->pos_min[1]) / 2.0f,
		(callbacks->pos_max[2] + callbacks->pos_min[2]) / 2.0f
	};

	float inv_diameter = 2.0f / std::sqrt(extents[0] * extents[0] + extents[1] * extents[1] + extents[2] * extents[2]);

	for (vertices_t::iterator iter = callbacks->vertices.begin(); iter != callbacks->vertices.end(); ++iter)
	{
		iter->position[0] = (iter->position[0] - center[0]) * inv_diameter;
		iter->position[1] = (iter->position[1] - center[1]) * inv_diameter;
		iter->position[2] = (iter->position[2] - center[2]) * inv_diameter;
	}
}


ply_mesh::~ply_mesh()
{
	delete callbacks;
}


ply_mesh::vertices_t const & ply_mesh::get_vertices() const
{
	return callbacks->vertices;
}


ply_mesh::triangles_t const & ply_mesh::get_triangles() const
{
	return callbacks->triangles;
}


ply_mesh::vertices_t & ply_mesh::get_vertices()
{
	return callbacks->vertices;
}


ply_mesh::triangles_t & ply_mesh::get_triangles()
{
	return callbacks->triangles;
}







//////////////
// Free functions for letting the ply_mesh conform to the VcacheMesh concept
//////////////


ply_mesh::triangle create_new_triangle(ply_mesh &, uint32_t const i1, uint32_t const i2, uint32_t const i3)
{
	return ply_mesh::triangle(i1, i2, i3);
}


std::size_t get_num_triangles(ply_mesh const &m, std::string const &)
{
	return m.get_triangles().size();
}


std::size_t get_num_vertices(ply_mesh const &m, std::string const &)
{
	return m.get_vertices().size();
}


ply_mesh::triangle get_triangle(ply_mesh const &m, std::string const &, unsigned long const &index)
{
	return m.get_triangles()[index];
}


ply_mesh::vertex get_vertex(ply_mesh const &m, std::string const &, uint32_t const &index)
{
	return m.get_vertices()[index];
}


void set_triangle(ply_mesh &m, std::string const &, unsigned long const &index, ply_mesh::triangle const &new_triangle)
{
	m.get_triangles()[index] = new_triangle;
}


void set_vertex(ply_mesh &m, std::string const &, uint32_t const &index, ply_mesh::vertex const &new_vertex)
{
	m.get_vertices()[index] = new_vertex;
}

