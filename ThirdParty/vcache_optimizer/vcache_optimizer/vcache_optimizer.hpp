#ifndef VCACHE_OPTIMIZER_01043967868054706570815610235347801523____
#define VCACHE_OPTIMIZER_01043967868054706570815610235347801523____


//
//  vcache_optimizer.hpp - mesh optimization code for reducing vertex cache misses,
//  using Tom Forsyth's linear-speed vertex cache optimisation algorithm
//  ( http://www.eelpi.gotdns.org/papers/papers.html )
//
//  Copyright (c) 2010 Carlos Rafael Giani
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//


#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <cmath>
#include <list>
#include <set>
#include <vector>

#include "null_callback.hpp"
#include "mesh_traits.hpp"


/*
vcache_optimizer works with surface meshes made up of triangles. Volume meshes, quad meshes etc. are not supported.

The meshes can be made up of submeshes, as it is common with meshes in computer games; there, submeshes are used
to support multiple materials in the same mesh (one material per submesh).

Submeshes in the mesh are expected to be separated from each other. This holds true at least for the triangles;
for example, a mixed set of triangles like "1 1 2 2 1 3 1", where 1/2/3 refers to a triangle of submesh 1/2/3,
is invalid. A valid version of this set would be "1 1 1 1 2 2 3". Another would be "2 2 1 1 1 1 3". If such
an ordering is inevitable, then the getters defined below must do some internal mapping. From the point of
view of the vcache_optimizer, the triangles are ordered contiguously and separated.

The submeshes may use a common set of vertices. In this case, get_num_vertices() should return the total number
of vertices and get_vertex() should ignore the submesh parameter. If submeshes share vertices, the vertex
reordering should be disabled, otherwise submeshes may get corrupted.


In order to use vcache_optimizer , the mesh type has to conform to the VcacheMesh concept. Several functions are required,
as well as a traits class defining some types.


The traits class has the following type definitions:

- submesh_id_t
  fulfills the CopyConstructibe and EqualityComparable concepts
  used for identifying submeshes

- vertex_t
  fulfills the CopyConstructibe concept
  contains or at least refers to a vertex. Copying an instance of this class copies a vertex.
  The vcache_optimizer code uses this type for using getting/setting vertices using get_vertex / set_vertex.

- triangles_t
  fulfills the CopyConstructibe concept
  contains or at least refers to a triangle. Copying an instance of this class copies a triangle.
  The vcache_optimizer code uses this type for using getting/setting triangles using get_triangle / set_triangle.
  In addition, the triangles_t type defines an index ( [] ) operator for retrieving triangle vertex indices.
  The index may be 0, 1, or 2. Triangles are made up of three vertex indices.

- vertex_index_t / triangle_index_t
  integral number types
  convertible to size_t



The traits class can be implemented and supplied in two ways:


1. Supply it as a template parameter to vcache_optimizer
   The traits class can have any name and be in any namespace.
   Example:
   vcache_optimizer < mymesh, float, mytraits > optimizer;


2. Specialize the mesh_traits template
   The traits class must be in the vcache_optimizer namespace.

   Example:

   namespace vcache_optimizer
   {
       template < >
       typename mesh_traits < mymesh >
       {
         ...
       }
   }

   vcache_optimizer < mymesh > optimizer;



The following free functions must be defined for the mesh:


- triangle_t create_new_triangle(VcacheMesh &vcache_mesh, vertex_index_t const &vtx1, vertex_index_t const &vtx2, vertex_index_t const &vtx3)
  Creates a new triangle of triangle_t type, made up of the vertex indices vtx1,vtx2,vtx3.

- std::size_t get_num_triangles(VcacheMesh const &vcache_mesh, submesh_id_t const &submesh_id)
  Returns the number of triangles the specified submesh has inside vcache_mesh.

- std::size_t get_num_vertices(VcacheMesh const &vcache_mesh, submesh_id_t const &submesh_id)
  Returns the number of vertices the specified submesh has inside vcache_mesh.

- triangle_t get_triangle(VcacheMesh const &vcache_mesh, submesh_id_t const &submesh_id, triangle_index_t const &index)
  Returns a triangle with the given index, from the specified submesh inside the vcache_mesh.
  The minimum index is 0, the maximum index is (get_num_triangles() - 1).

- vertex_t get_vertex(VcacheMesh const &vcache_mesh, submesh_id_t const &submesh_id, vertex_index_t const &index)
  Returns a vertex with the given index, from the specified submesh inside the vcache_mesh.
  The minimum index is 0, the maximum index is (get_num_vertices() - 1).

- void set_triangle(VcacheMesh &vcache_mesh, submesh_id_t const &submesh_id, triangle_index_t const &index, triangle_t const &new_triangle)
  Sets the given triangle in the specified submesh inside the vcache_mesh.
  The minimum index is 0, the maximum index is (get_num_triangles() - 1).

- void set_vertex(VcacheMesh &vcache_mesh, submesh_id_t const &submesh_id, vertex_index_t const &index, vertex_t const &new_vertex)
  Sets the given vertex in the specified submesh inside the vcache_mesh.
  The minimum index is 0, the maximum index is (get_num_vertices() - 1).

*/


namespace vcache_optimizer
{


template < typename VcacheMesh, typename NumericType = float, typename Traits = mesh_traits < VcacheMesh > >
class vcache_optimizer
{
private:
	typedef VcacheMesh mesh_t;
	typedef NumericType numeric_type_t;

	typedef typename Traits::submesh_id_t      submesh_id_t;
	typedef typename Traits::vertex_t          vertex_t;
	typedef typename Traits::vertex_index_t    vertex_index_t;
	typedef typename Traits::triangle_index_t  triangle_index_t;
	typedef typename Traits::triangle_t        triangle_t;

	typedef std::list < vertex_index_t > lru_cache_t;
	typedef std::set < triangle_index_t > tri_indices_using_t;


	// structure holding vertex related data such as its current score,
	// the indices of the triangles using this vertex etc.
	struct vertex_data
	{
		int position;
		numeric_type_t score; // the vertex score; higher = better vertex

		// the indices of the triangles that are using this vertex
		tri_indices_using_t tri_indices_using;

		vertex_data()
		{
			position = -1;
			score = 0;
		}
	};


	// structure holding triangle related data such as its current score,
	// the indices of the vertices this triangle is made up from etc.
	struct triangle_data
	{
		numeric_type_t score; // the triangle score; higher = better triangle; the sum of the vertex scores
		vertex_index_t indices[3]; // indices of the 3 vertices that make up this triangle
		bool disabled; // if true, this triangle will not be looked at when finding a best triangle

		triangle_data()
		{
			disabled = false;
			score = 0;
			indices[0] = indices[1] = indices[2] = 0;
		}
	};




public:
	explicit vcache_optimizer(
		numeric_type_t const cache_decay_power = numeric_type_t(1.5),
		numeric_type_t const last_tri_score = numeric_type_t(0.75),
		numeric_type_t const valence_boost_scale = numeric_type_t(2.0),
		numeric_type_t const valence_boost_power = numeric_type_t(0.5),
		std::size_t const max_cache_size = 32
	):
		cache_decay_power(cache_decay_power),
		last_tri_score(last_tri_score),
		valence_boost_scale(valence_boost_scale),
		valence_boost_power(valence_boost_power),
		max_cache_size(max_cache_size)
	{
	}


	void operator ()(VcacheMesh &vcache_mesh, submesh_id_t const &submesh_id, bool const reorder_vertices = true)
	{
		null_callback cb;
		operator()(vcache_mesh, submesh_id, cb, reorder_vertices);
	}


	template < typename ProgressCallback >
	void operator ()(VcacheMesh &vcache_mesh, submesh_id_t const &submesh_id, ProgressCallback & progress_callback, bool const reorder_vertices = true)
	{
		//////// prerequisites
		//////////////////////

		has_best_triangle = false;
		best_triangle = 0;

		optimized_tris.clear();
		lru_cache.clear();

		vtx_data.resize(get_num_vertices(vcache_mesh, submesh_id));
		tri_data.resize(get_num_triangles(vcache_mesh, submesh_id));

		// fetch the triangle data and put it in the internal vector
		// also fill the tri_indices_using vectors in the process
		{
			for (triangle_index_t tri_index = 0; tri_index < get_num_triangles(vcache_mesh, submesh_id); ++tri_index)
			{
				triangle_t triangle = get_triangle(vcache_mesh, submesh_id, tri_index);

				for (int i = 0; i < 3; ++i)
				{
					tri_data[tri_index].indices[i] = triangle[i];
					vtx_data[triangle[i]].tri_indices_using.insert(tri_index);
				}
			}
		}


		//////// optimize triangles
		///////////////////////////

		// calculate vertex and triangle scores
		{
			for (vertex_index_t vtx_index = 0; vtx_index < get_num_vertices(vcache_mesh, submesh_id); ++vtx_index)
			{
				vertex_data &vtx = vtx_data[vtx_index];
				vtx.score = find_vertex_score(vtx); // calculate the vertex score

				for (typename tri_indices_using_t::const_iterator tri_idx_iter = vtx.tri_indices_using.begin(); tri_idx_iter != vtx.tri_indices_using.end(); ++tri_idx_iter)
					tri_data[*tri_idx_iter].score += vtx.score; // add the vertex score to the triangles using this vertex
			}
		}

		// tell the progress callback the maximum progress value
		set_maximum_optimizing_triangles_progress(progress_callback, tri_data.size());

		/*
		the actual optimization step; reorder triangles by repeatedly finding a "best" triangle
		(= look at all the vertices in the LRU cache, and from all the triangles using these vertices,
		find the one with the highest score, remove this one from the vertices and put it in
		optimized_tris, and then find the next best triangle etc.)
		*/
		{
			size_t progress_counter = 0;
			while (optimized_tris.size() < tri_data.size())
			{
				push_best_triangle();
				++progress_counter;
				set_current_optimizing_triangles_progress(progress_callback, progress_counter);
			}
		}


		//////// reoder vertices
		////////////////////////

		if (reorder_vertices)
		{
			// even though *vertices* are reordered, reordering happens across *triangles*
			set_maximum_reordering_vertices_progress(progress_callback, tri_data.size());

			// get the vertices from the mesh
			std::vector < vertex_t > src_vertices;
			src_vertices.resize(vtx_data.size());
			for (vertex_index_t vtx_index = 0; vtx_index < vtx_data.size(); ++vtx_index)
				src_vertices[vtx_index] = get_vertex(vcache_mesh, submesh_id, vtx_index);

			// create and initialize the permutation sequence;
			// this sequence will be used for updating the triangle vertex indices later
			std::vector < std::pair < bool, vertex_index_t > > permutation;
			permutation.resize(vtx_data.size());
			std::fill(permutation.begin(), permutation.end(), std::pair < bool, vertex_index_t > (false, 0));

			/*
			reordering is done according to the order of access
			"access" refers to the triangles; for example, it makes no sense when the first
			triangle's vertices are at the end of the list of vertices, the second triangle's
			are in the middle etc.
			Instead, the first triangle's vertices should be at the beginning, the second triangle's
			should be right after these etc.
			*/
			size_t progress_counter = 0;
			vertex_index_t mesh_vertex_index = 0;
			for (triangle_index_t tri_index = 0; tri_index < optimized_tris.size(); ++tri_index)
			{
				triangle_data const &tri = tri_data[tri_index];

				// go through all 3 indices of each triangle,
				// and if it wasn't reordered, do so
				for (int i = 0; i < 3; ++i)
				{
					vertex_index_t vtx_index = tri.indices[i];
					if (!permutation[vtx_index].first) // first false -> was not reordered yet
					{
						// check for overflow; it should never happen, since
						// each vertex is reordered only once
						assert(mesh_vertex_index < src_vertices.size());

						// mark the vertex as reordered and store its new index
						permutation[vtx_index].first = true;
						permutation[vtx_index].second = mesh_vertex_index;

						// write the vertex at its new position in the mesh
						set_vertex(vcache_mesh, submesh_id, mesh_vertex_index, src_vertices[vtx_index]);
						++mesh_vertex_index;
					}
				}

				++progress_counter;
				set_current_reordering_vertices_progress(progress_callback, progress_counter);
			}

			// After the vertices have been reodered, the triangle vertex indices need to be updated
			for (triangle_index_t tri_index = 0; tri_index < optimized_tris.size(); ++tri_index)
			{
				triangle_data &tri = optimized_tris[tri_index];
				for (int i = 0; i < 3; ++i)
					tri.indices[i] = permutation[tri.indices[i]].second;
			}
		}

		// finally, store the contents of optimized_tris in the mesh
		{
			for (triangle_index_t tri_index = 0; tri_index < optimized_tris.size(); ++tri_index)
			{
				triangle_data const &tri = optimized_tris[tri_index];
				triangle_t new_triangle = create_new_triangle(
					vcache_mesh,
					tri.indices[0],
					tri.indices[1],
					tri.indices[2]
				);

				set_triangle(vcache_mesh, submesh_id, tri_index, new_triangle);
			}
		}
	}


private:
	// this function calculates a score from the given vertex
	// the score is -1 if no triangle uses the vertex, to keep it at the bottom of the LRU cache
	// (the score depends on the amount of triangles using this vertex)
	numeric_type_t find_vertex_score(vertex_data const &vertex_data_) const
	{
		if (vertex_data_.tri_indices_using.empty())
			return numeric_type_t(-1); // no triangle needs this vertex!

		numeric_type_t score = 0;
		int cache_position = vertex_data_.position;
		if (cache_position >= 0) // if cache_position < 0 then vertex is not in cache -> no score.
		{
			if (cache_position < 3)
			{
				score = last_tri_score;
			}
			else
			{
				assert(cache_position < int(max_cache_size)); // catch overflows
				numeric_type_t const scaler = numeric_type_t(1) / numeric_type_t(max_cache_size - 3);
				score = numeric_type_t(1) - numeric_type_t(cache_position - 3) * scaler;
				score = std::pow(score, cache_decay_power);
			}
		}
		
		numeric_type_t valence_boost = std::pow(numeric_type_t(vertex_data_.tri_indices_using.size()), -valence_boost_power);
		score += valence_boost_scale * valence_boost;

		return score;
	}


	void push_best_triangle()
	{
		/*
		there may be no known best triangle
		this happens in two cases:
		(1) in the very first iteration
		(2) towards the end, where the remaining triangles are spread around and isolated

		this is considerably slower than using the LRU cache, but typically happens rarely
		*/
		if (!has_best_triangle)
		{
			numeric_type_t score = numeric_type_t(-1);
			for (typename std::vector < triangle_data > ::iterator tri_iter = tri_data.begin(); tri_iter != tri_data.end(); ++tri_iter)
			{
				if (tri_iter->disabled)
					continue;

				if (tri_iter->score >= score)
				{
					score = tri_iter->score;
					best_triangle = tri_iter - tri_data.begin();
					has_best_triangle = true;
				}
			}
		}


		// the push_best_triangle() function is never called if there are no triangles to begin with,
		// and if there are some, the code above should always return a best triangle
		assert(has_best_triangle);


		// get the best triangle
		triangle_data &best_tri = tri_data[best_triangle];

		// push the best triangle's vertices to the top of the LRU cache, potentially expanding
		// the cache by up to 3 vertices
		// the loop is reversed to preserver order of the vertices inside the LRU cache; that is, triangle vertex index #0 shall
		// be the first in the LRU cache, index #1 the second etc.
		// TODO: reverse/nonreverse return different optimization results; test if this has an impact in cache efficiency
		{
			for (int i = 2; i >= 0; --i)
			{
				vertex_index_t idx = best_tri.indices[i];
				typename lru_cache_t::iterator cache_iter = std::find(lru_cache.begin(), lru_cache.end(), idx);
				if (cache_iter != lru_cache.end())
				{
					lru_cache.push_front(idx);
					lru_cache.erase(cache_iter);
				}
				else
				{
					lru_cache.push_front(idx);
				}
			}
		}

		// push the best triangle into the optimized triangle list, and disable it, meaning
		// it will not be considered a candidate for best triangle anymore
		optimized_tris.push_back(best_tri);
		best_tri.disabled = true;

		// remove the best triangle from the tri_indices_using vectors of its vertices
		for (int i = 0; i < 3; ++i)
		{
			vertex_data &vtx = vtx_data[best_tri.indices[i]];
			typename tri_indices_using_t::iterator iter = vtx.tri_indices_using.find(best_triangle);
			if (iter != vtx.tri_indices_using.end())
				vtx.tri_indices_using.erase(iter);
		}

		// now find a new best triangle
		{
			std::set < triangle_index_t > triangles_in_cache;

			// fill the triangles_in_cache vector by visiting each vertex in the LRU cache,
			// retrieving the indices of the triangles using these vertices, and storing them
			// in triangles_in_cache
			// the result is a set of indices of those triangles which are referred to by LRU cache
			// vertices; only these triangles are candidates for the new best triangle
			int pos = 0;
			for (typename lru_cache_t::iterator cache_iter = lru_cache.begin(); cache_iter != lru_cache.end(); ++cache_iter, ++pos)
			{
				vertex_data &vtx = vtx_data[*cache_iter];
				vtx.position = (pos < int(max_cache_size)) ? pos : -1;
				vtx.score = find_vertex_score(vtx);
				triangles_in_cache.insert(vtx.tri_indices_using.begin(), vtx.tri_indices_using.end());
			}

			numeric_type_t highest_score = 0;
			has_best_triangle = false;

			// amongst the candidate triangles, find the one with the highest score - this one
			// will become the new best triangle
			for (typename std::set < triangle_index_t > ::iterator tri_iter = triangles_in_cache.begin(); tri_iter != triangles_in_cache.end(); ++tri_iter)
			{
				triangle_data &tri_in_cache = tri_data[*tri_iter];
				if (tri_in_cache.disabled)
					continue; // if this triangle was a best triangle before, skip it

				// update the triangle's score
				tri_in_cache.score =
					vtx_data[tri_in_cache.indices[0]].score +
					vtx_data[tri_in_cache.indices[1]].score +
					vtx_data[tri_in_cache.indices[2]].score;

				// is this triangle's score higher than the highest score seen so far?
				// if so, it becomes the new candidate for the best triangle
				if (tri_in_cache.score > highest_score)
				{
					highest_score = tri_in_cache.score;
					best_triangle = *tri_iter;
					has_best_triangle = true;
				}
			}
		}

		// finally, prune the LRU cache if necessary
		while (lru_cache.size() > size_t(max_cache_size))
		{
			lru_cache.pop_back();
		}
	}
	


	numeric_type_t const cache_decay_power, last_tri_score, valence_boost_scale, valence_boost_power;
	std::size_t const max_cache_size;

	bool has_best_triangle;
	triangle_index_t best_triangle;

	std::vector < vertex_data > vtx_data;
	std::vector < triangle_data > tri_data, optimized_tris;
	lru_cache_t lru_cache;
};


}


#endif

