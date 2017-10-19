//
//  ply_viewer.cpp - main PLY viewer source file;
//  initialized OpenGL using GLEW and GLFW, loads the PLY mesh, optionally
//  optimizes it using the vcache_optimizer, and renders it
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <assert.h>

#include "opengl.hpp"
#include "glQuickText.h"
#include <GL/glfw.h>
#include "ply_renderer.hpp"

#include "ply_mesh.hpp"
#include <vcache_optimizer/vcache_optimizer.hpp>
#include <vcache_optimizer/stdout_callback.hpp>


class viewer
{
protected:
	typedef std::auto_ptr < ply_mesh > ply_mesh_ptr_t;
	typedef std::auto_ptr < ply_renderer > ply_renderer_ptr_t;

public:


	explicit viewer(std::string const &ply_filename, bool const do_optimize):
		default_distance(2.0f),
		distance(default_distance),
		distance_delta(0.0f),
		width(0),
		height(0),
		framecount(0)
	{
		std::ifstream in_file(ply_filename.c_str());
		if (in_file.good())
			mesh = ply_mesh_ptr_t(new ply_mesh(in_file));
		else
			throw std::runtime_error(std::string("unable to open ") + ply_filename);


		if (do_optimize)
		{
			vcache_optimizer::vcache_optimizer < ply_mesh > optimizer;
			vcache_optimizer::stdout_callback cb; 
			optimizer(*mesh, "", cb);
		}


		render_ply = ply_renderer_ptr_t(new ply_renderer(*mesh));


		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClearDepth(1.0f);

		reset_modelview();
		glEnable(GL_CULL_FACE);


		update_timestamp();
		last_fps_timestamp = last_timestamp;
	}


	void resize(unsigned int const new_width, unsigned int const new_height)
	{
		float const scale = 0.2f;
		float aspect = float(new_width) / float(new_height);
		width = new_width;
		height = new_height;

		glViewport(0, 0, new_width, new_height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glFrustum(-aspect * scale, aspect * scale,   -scale, scale,   0.1f, 100.0f);
	}


	void render()
	{
		update_timestamp();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		distance += distance_delta * distance * time_delta;
		glTranslatef(0, 0, -distance);
		render_ply->render();

		render_text();
	}


	void reset_modelview()
	{
		distance = default_distance;
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -distance);
	}


	void set_distance_delta(float const new_delta)
	{
		distance_delta = new_delta;
	}


protected:
	void render_text()
	{
		// prepare the text to render

		double timespan = (last_timestamp - last_fps_timestamp);
		if (timespan >= 1.0)
		{
			last_fps_timestamp = last_timestamp;
			std::stringstream sstr;
			sstr << double(framecount) / timespan << " fps (" << (double(timespan) / framecount * 1000.0) << "ms/frame)\n";
			text = sstr.str();
			framecount = 0;
		}

		++framecount;


		// setup 2D projection for the FPS display
		// use an orthogonal projection matrix and an identity matrix for modelview

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, width, 0, height, 1, 100);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// Draw text using glQuickText

		glColor3f(1, 1, 1);
		glQuickText::printfAt(0, 0, -2, 1, "%s", text.c_str());

		// restore previous modelview and projection matrices

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}


	void update_timestamp()
	{
		double new_timestamp = glfwGetTime();
		time_delta = new_timestamp - last_timestamp;
		last_timestamp = new_timestamp;
	}


	ply_mesh_ptr_t mesh;
	ply_renderer_ptr_t render_ply;
	float default_distance;
	float distance;
	float distance_delta;
	unsigned int width, height;
	double last_timestamp, last_fps_timestamp, time_delta;
	unsigned int framecount;
	std::string text;
};



viewer *viewer_ptr = 0;

void GLFWCALL window_resized(int new_width, int new_height)
{
	assert(viewer_ptr != 0);
	viewer_ptr->resize(new_width, new_height);
}


void GLFWCALL key_pressed(int key, int action)
{
	assert(viewer_ptr != 0);
	float delta = (action == GLFW_PRESS) ? 1.0f : 0.0f;

	switch (key)
	{
		case GLFW_KEY_DOWN: viewer_ptr->set_distance_delta( delta); break;
		case GLFW_KEY_UP:   viewer_ptr->set_distance_delta(-delta); break;
		case 'R':           viewer_ptr->reset_modelview(); break;
		default: break;
	}
}





int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <PLY file> [<-optimize>]" << std::endl;
		return -1;
	}

	bool do_optimize = false;
	std::string ply_filename = argv[1];

	if (argc >= 3)
	{
		if (std::string(argv[2]) == "-optimize")
			do_optimize = true;
	}

	if (!glfwInit())
		return -1;

	if (!glfwOpenWindow(800, 600, 0, 0, 0, 0, 0, 0, GLFW_WINDOW))
	{
		glfwTerminate();
		return -2;
	}

	glfwSwapInterval(0);
	glewInit();

	glfwSetWindowTitle((ply_filename + " [" + (do_optimize ? "optimized" : "unoptimized") + "]").c_str());


	try
	{
		viewer viewer_(ply_filename, do_optimize);
		viewer_ptr = &viewer_;

		glfwSetWindowSizeCallback(&window_resized);
		glfwSetKeyCallback(&key_pressed);

		bool running = true;
		while (running)
		{
			glfwPollEvents();
			running = glfwGetWindowParam(GLFW_OPENED);
			viewer_.render();
			glfwSwapBuffers();
		}
	}
	catch (std::runtime_error const &error)
	{
		std::cerr << "Error: " << error.what() << std::endl;
	}


	glfwTerminate();

	return 0;
}

