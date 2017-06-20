
#include <FbxViewerOS.h>
#include <new>

#include <time.h>
#define MSPACES 1
#define USE_DL_PREFIX 1
#define ONLY_MSPACES 1
#define MALLOC_ALIGNMENT (apemode::kDefaultAlignment)
#include "malloc.c.h"

static thread_local mspace tlms = create_mspace(0, 0);

namespace apemode {
    void* malloc(size_t size, size_t alignment) {
        void *p = nullptr;

#ifdef __ANDROID__
        ::posix_memalign( &p, alignment, size );
#else
        p = ::_aligned_malloc( size, alignment );
#endif

        return p;
    }

    void free (void * p) {
#ifdef __ANDROID__
        ::free( p );
#else
        ::_aligned_free( p );
#endif
    }

    void * thread_local_malloc(size_t size, size_t alignment) {
        return mspace_malloc2(tlms, size, alignment, 0);
    }

    void * thread_local_realloc(void *p, size_t size, size_t alignment) {
        return mspace_realloc2(tlms, p, size, alignment, 0);
    }

    void thread_local_free(void *p) {
        mspace_free(tlms, p);
    }

}

void *operator new( size_t size ) {
    return apemode::malloc( size, apemode::kDefaultAlignment );
}

void *operator new[]( size_t size ) {
    return apemode::malloc( size, apemode::kDefaultAlignment );
}

void *operator new[]( size_t size, const char * /*name*/, int /*flags*/, unsigned /*debugFlags*/, const char * /*file*/, int /*line*/ ) {
    return apemode::malloc( size, apemode::kDefaultAlignment );
}

void *operator new[]( size_t size,
                      size_t alignment,
                      size_t /*alignmentOffset*/,
                      const char * /*name*/,
                      int /*flags*/,
                      unsigned /*debugFlags*/,
                      const char * /*file*/,
                      int /*line*/ ) {
    return apemode::malloc( size, alignment );
}

void *operator new( size_t size, size_t alignment ) {
    return apemode::malloc( size, alignment );
}

void *operator new( size_t size, size_t alignment, const std::nothrow_t & ) throw( ) {
    return apemode::malloc( size, alignment );
}

void *operator new[]( size_t size, size_t alignment ) {
    return apemode::malloc( size, alignment );
}

void *operator new[]( size_t size, size_t alignment, const std::nothrow_t & ) throw( ) {
    return apemode::malloc( size, alignment );
}

// C++14 deleter
void operator delete(void *p, std::size_t sz) throw( ) {
    apemode::free( p );
    (void)( sz );
}

void operator delete[]( void *p, std::size_t sz ) throw( ) {
    apemode::free( p );
    (void)( sz );
}

void operator delete(void *p) throw( ) {
    apemode::free( p );
}

void operator delete[]( void *p ) throw( ) {
    apemode::free( p );
}
