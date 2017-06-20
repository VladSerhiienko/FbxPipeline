#pragma once

#include <stdlib.h>

namespace apemode {
    /**
     * The default alignment for memory allocations.
     */
    static const size_t kDefaultAlignment = sizeof(void*) << 1;

    /**
     * The regular malloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* malloc(size_t size, size_t alignment = kDefaultAlignment);

    /**
     * The regular free call.
     * @param p The memory chunk address.
     */
    void free (void* p);

    /**
     * The malloc call with aligment from the thread-local memory space.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void * thread_local_malloc(size_t size, size_t alignment = kDefaultAlignment);

    /**
     * The realloc call with aligment from the thread-local memory space.
     * Do not align with less then kDefaultAlignment bytes.
     * @param p Address of the old memroy chunk.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The reallocated memory chunk address.
     */
    void * thread_local_realloc(void *p, size_t size, size_t alignment = kDefaultAlignment);

    /**
     * The free call from the thread-local memory space.
     * @param p The memory chunk address.
     */
    void thread_local_free(void *p);

    template <size_t uAlignment = kDefaultAlignment, bool bThreadLocal = false>
    struct TNew {

        inline static void *operator new( size_t size) {
            if (bThreadLocal)
                return apemode::thread_local_malloc( size, uAlignment );
            else
                return apemode::malloc( size, uAlignment );
        }

        inline static void *operator new[]( size_t size) {
            if (bThreadLocal)
                return apemode::thread_local_malloc( size, uAlignment );
            else
                return apemode::malloc( size, uAlignment );
        }

        inline static void operator delete( void* ptr) {
            if (bThreadLocal)
                apemode::thread_local_free( ptr );
            else
                apemode::free( ptr );
        }

        inline static void operator delete[]( void* ptr) {
            if (bThreadLocal)
                apemode::thread_local_free( ptr );
            else
                apemode::free( ptr );
        }
    };

    template <typename TDerived, bool bThreadLocal = false>
    struct TNewOf : TNew< alignof (TDerived), bThreadLocal> {
    };
}
