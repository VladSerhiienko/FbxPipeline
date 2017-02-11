#include <fbxvpch.h>

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

namespace fbxv {
    void *Malloc( size_t bytes ) {
        return dlmalloc( bytes );
    }
    void *Memalign( size_t alignment, size_t bytes ) {
        return dlmemalign( alignment, bytes );
    }
    void Free( void *p ) {
        return dlfree( p );
    }
}