#include <fbxppch.h>
#include <new>

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT 16
#endif

#ifndef USE_DL_PREFIX
#define USE_DL_PREFIX
#endif

#include "malloc.c.h"

void *operator new( size_t size ) {
    return dlmalloc( size );
}

void *operator new[]( size_t size ) {
    return dlmalloc( size );
}

void *operator new[]( size_t size, const char *, int, unsigned, const char *, int ) {
    return dlmalloc( size );
}

void *operator new[]( size_t size, size_t alignment, size_t, const char *, int, unsigned, const char *, int ) {
    return dlmemalign( alignment, size );
}

void *operator new( size_t size, size_t alignment ) {
    return dlmemalign( alignment, size );
}

void *operator new( size_t size, size_t alignment, const std::nothrow_t & ) throw( ) {
    return dlmemalign( alignment, size );
}

void *operator new[]( size_t size, size_t alignment ) {
    return dlmemalign( alignment, size );
}

void *operator new[]( size_t size, size_t alignment, const std::nothrow_t & ) throw( ) {
    return dlmemalign( alignment, size );
}

void operator delete( void *p, std::size_t sz ) throw( ) {
    dlfree( p );
}

void operator delete[]( void *p, std::size_t sz ) throw( ) {
    dlfree( p );
}

void operator delete( void *p ) throw( ) {
    dlfree( p );
}

void operator delete[]( void *p ) throw( ) {
    dlfree( p );
}
