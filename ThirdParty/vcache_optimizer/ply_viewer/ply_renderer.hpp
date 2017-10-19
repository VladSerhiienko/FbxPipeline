#ifndef PLY_RENDERER_HPP_74831097568043575776051356103560137516____
#define PLY_RENDERER_HPP_74831097568043575776051356103560137516____


//
//  ply_renderer.hpp - PLY mesh rendering class
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include "opengl.hpp"


class ply_mesh;


class ply_renderer
{
public:
	explicit ply_renderer(ply_mesh const &mesh);
	~ply_renderer();


	void render();


protected:
	void bind_vb(bool const state);
	void bind_ib(bool const state);


	GLuint vertex_buffer, index_buffer;
	unsigned long num_triangles, num_vertices;
};


#endif

