#pragma once

namespace apemode
{
#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#define _Nesquik_Has_array_traits 1

    template <typename TArray>
    struct TArrayTraits;

    template <typename TArray, size_t N>
    struct TArrayTraits<TArray[ N ]>
    {
        static const size_t ArrayLength = N;
    };

    template < typename TArray, uint32_t N >
    uint32_t GetArraySize( TArray ( & )[ N ] ) {
        return N;
    }

#define ARRAYSIZE apemode::GetArraySize
}