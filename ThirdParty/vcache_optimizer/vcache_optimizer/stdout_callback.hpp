#ifndef VCACHE_OPTIMIZER_STDOUT_CALLBACK_64137564317085613085678250510857012574____
#define VCACHE_OPTIMIZER_STDOUT_CALLBACK_64137564317085613085678250510857012574____


//
//  stdout_callback.hpp - stdout progress callback type; outputs to stdout
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <cstddef>
#include <iostream>


namespace vcache_optimizer
{


struct stdout_callback
{
	size_t maximum_optimizing_triangles_progress;
	size_t maximum_reordering_vertices_progress;
	float last_ot_percentage, last_rv_percentage;
};


inline void set_maximum_optimizing_triangles_progress(stdout_callback &stdcb, size_t const maximum)
{
	stdcb.maximum_optimizing_triangles_progress = maximum;
	stdcb.last_ot_percentage = 0;
}


inline void set_maximum_reordering_vertices_progress(stdout_callback &stdcb, size_t const maximum)
{
	stdcb.maximum_reordering_vertices_progress = maximum;
	stdcb.last_rv_percentage = 0;
}


inline void set_current_optimizing_triangles_progress(stdout_callback &stdcb, size_t const current)
{
	size_t max_ = stdcb.maximum_optimizing_triangles_progress;
	float percentage = float(current) / float(max_) * 100.0f;
	float diff_to_last = percentage - stdcb.last_ot_percentage;

	if ((diff_to_last < 5.0f) && (current < max_))
		return;

	stdcb.last_ot_percentage = percentage;

	std::cout << "Optimizing triangles: " << current << "/" << max_ << " (" << percentage << "%)              ";
	std::cout << ((current >= max_) ? "\n" : "\r") << std::flush;
}


inline void set_current_reordering_vertices_progress(stdout_callback &stdcb, size_t const current)
{
	size_t max_ = stdcb.maximum_reordering_vertices_progress;
	float percentage = float(current) / float(max_) * 100.0f;
	float diff_to_last = percentage - stdcb.last_rv_percentage;

	if ((diff_to_last < 5.0f) && (current < max_))
		return;

	stdcb.last_rv_percentage = percentage;

	std::cout << "Reordering vertices: " << current << "/" << max_ << " (" << percentage << "%)              ";
	std::cout << ((current >= max_) ? "\n" : "\r") << std::flush;
}


}


#endif

