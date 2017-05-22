#pragma once

namespace Nesquik
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

#ifdef _Get_array_length
#undef _Get_array_length
#endif

#ifdef _Get_array_length_u
#undef _Get_array_length_u
#endif

#define _Get_array_length(Array) Nesquik::TArrayTraits<decltype (Array)>::ArrayLength
#define _Get_array_length_u(Array) static_cast<unsigned int> (_Get_array_length (Array))
#define ARRAYSIZE _Get_array_length_u
}