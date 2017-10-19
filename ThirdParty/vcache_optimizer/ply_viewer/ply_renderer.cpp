//
//  ply_renderer.cpp - PLY mesh rendering class
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include "opengl.hpp"
#include "ply_mesh.hpp"
#include "ply_renderer.hpp"


ply_renderer::ply_renderer(ply_mesh const &mesh)
{
	glGenBuffers(1, &vertex_buffer);
	glGenBuffers(1, &index_buffer);

	num_vertices = mesh.get_vertices().size();
	num_triangles = mesh.get_triangles().size();

	bind_vb(true);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ply_mesh::vertex) * num_vertices, &mesh.get_vertices()[0], GL_STATIC_DRAW);
	bind_vb(false);

	bind_ib(true);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ply_mesh::triangle) * num_triangles, &mesh.get_triangles()[0], GL_STATIC_DRAW);
	bind_ib(false);
}


ply_renderer::~ply_renderer()
{
	bind_vb(false);
	bind_ib(false);

	glDeleteBuffers(1, &vertex_buffer);
	glDeleteBuffers(1, &index_buffer);
}


void ply_renderer::render()
{
	bind_vb(true);
	bind_ib(true);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glDrawRangeElements(GL_TRIANGLES, 0, num_vertices - 1, num_triangles * 3, GL_UNSIGNED_INT, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
	bind_vb(false);
	bind_ib(false);
}


void ply_renderer::bind_vb(bool const state)
{
	glBindBuffer(GL_ARRAY_BUFFER, state ? vertex_buffer : 0);
}


void ply_renderer::bind_ib(bool const state)
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state ? index_buffer : 0);
}

