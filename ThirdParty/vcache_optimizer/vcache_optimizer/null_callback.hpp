#ifndef VCACHE_OPTIMIZER_NULL_CALLBACK_01289365279456043825682741318924271043____
#define VCACHE_OPTIMIZER_NULL_CALLBACK_01289365279456043825682741318924271043____


//
//  null_callback.hpp - null progress callback type; does not output anything
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <cstddef>


namespace vcache_optimizer
{


struct null_callback
{
};


inline void set_maximum_optimizing_triangles_progress(null_callback &, size_t const)
{
}


inline void set_maximum_reordering_vertices_progress(null_callback &, size_t const)
{
}


inline void set_current_optimizing_triangles_progress(null_callback &, size_t const)
{
}


inline void set_current_reordering_vertices_progress(null_callback &, size_t const)
{
}


}


#endif

