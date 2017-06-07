#pragma once

#include <memory>
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <mutex>
#include <assert.h>

#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>

#define _Graphics_ecosystem_dll_api
#define _Game_engine_Assert(...) //assert(__VA_ARGS__)
#define apemode_likely(...) __VA_ARGS__
#define _Game_engine_Unlikely(...) __VA_ARGS__
#define _Game_engine_Error(...)
#define _Game_engine_Halt(...)
#define _Get_collection_length_u(c) ((uint32_t) c.size())

namespace apemodevk {
    struct ScalableAllocPolicy {};
    struct NoCopyAssignPolicy {};
}

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#define _Aux_TArrayTraits_Has_array_traits 1

namespace apemodevk
{
    template <typename TArray>
    struct TArrayTraits;

    template <typename TArray, size_t N>
    struct TArrayTraits<TArray[ N ]> {
        static const size_t ArrayLength = N;
    };

    template <typename TArray, uint32_t TArraySize>
    inline uint32_t GetArraySizeU(TArray(&)[TArraySize]) {
        return TArraySize;
    }
}

namespace apemodevk {

#ifdef ZeroMemory
#undef ZeroMemory
#endif

    template < typename T >
    inline void ZeroMemory( T &pObj ) {
        memset( &pObj, 0, sizeof( T ) );
    }

    template < typename T, size_t TCount >
    inline void ZeroMemory( T ( &pObj )[ TCount ] ) {
        memset( &pObj[ 0 ], 0, sizeof( T ) * TCount );
    }

    template < typename T, size_t Count >
    inline void ZeroMemory( T *pObj, size_t Count ) {
        memset( pObj, 0, sizeof( T ) * Count );
    }

    template < typename T, typename... TArgs >
    inline T &PushBackAndGet( std::vector< T > &_collection, TArgs... args ) {
        _collection.emplace_back( std::forward< TArgs >( args )... );
        return _collection.back( );
    }
}

#ifdef _Get_array_length
#undef _Get_array_length
#endif

#ifdef _Get_array_length_u
#undef _Get_array_length_u
#endif

#define _Get_array_length(Array) apemodevk::TArrayTraits<decltype (Array)>::ArrayLength
#define _Get_array_length_u(Array) static_cast<unsigned int> (_Get_array_length (Array))
#define ARRAYSIZE _Get_array_length_u

namespace apemodevk
{
    namespace Details
    {
        using FlagsType = unsigned int;
        using VoidPtr   = void *;
        using IdType    = unsigned long long;
    }

    template <typename TDataFlags  = Details::FlagsType,
        typename TDataSrcPtr = Details::VoidPtr,
        typename TDataId     = Details::IdType>
        struct TDataHandle
    {
        TDataId     DataId;
        TDataFlags  DataFlags;
        TDataSrcPtr DataAddress;

        TDataHandle ()
            : DataId (TDataId (0))
            , DataFlags (TDataFlags (0))
            , DataAddress (TDataSrcPtr (nullptr))
        {
        }
    };

    //
    // This can be usable for some native enums that have no operators defined.
    //

    /** Returns 'a | b' bits. */
    template <typename U> inline static U FlagsOr(_In_ U a, _In_ U b) { return static_cast<U>(a | b); }
    /** Returns 'a & b' bits. */
    template <typename U> inline static U FlagsAnd(_In_ U a, _In_ U b) { return static_cast<U>(a & b); }
    /** Returns 'a ^ b' bits. */
    template <typename U> inline static U FlagsXor(_In_ U a, _In_ U b) { return static_cast<U>(a ^ b); }
    /** Returns inverted 'a' bits. */
    template <typename U> inline static U FlagsInv(_In_ U a) { return static_cast<U>(~a); }

    /** Sets 'b' bits to 'a'. Also see 'FlagsOr'. */
    template <typename U, typename V> inline static void SetFlag(_Out_ U &a, _In_ V b) { a |= static_cast<U>(b); }
    /** Removes 'b' bits from 'a'. Also see 'FlagsInv', 'FlagsAnd'. */
    template <typename U, typename V> inline static void RemoveFlag(_Out_ U &a, _In_ V b) { a &= ~static_cast<U>(b); }
    /** Returns true, if exactly 'b' bits are present in 'a'. */
    template <typename U, typename V> inline static bool HasFlagEql(_In_ U a, _In_ V b) { return V(static_cast<V>(a) & b) == b; }
    /** Returns true, if some of 'b' bits are present in 'a'. */
    template <typename U, typename V> inline static bool HasFlagAny(_In_ U a, _In_ V b) { return V(static_cast<V>(a) & b) != static_cast<V>(0); }

    typedef TDataHandle<> GenericUserDataHandle;
}

#define _Aux_DebugTraceFunc

namespace apemodevk
{
    template <typename T>
    static void TSafeDeleteObj (_Inout_ _Maybenull_ T *& pObj)
    {
        if (apemode_likely (pObj != nullptr))
            delete pObj, pObj = nullptr;
    }

    template <typename T>
    static void TSafeDeleteObj (_Inout_ _Maybenull_ T const *& pObj)
    {
        if (apemode_likely (pObj != nullptr))
            delete const_cast<T *> (pObj), pObj = nullptr;
    }

    template <class TDecoratedObj>
    struct TSafeDeleteObjOp
    {
        using TObj = typename std::decay<TDecoratedObj>::type;
        void operator () (_In_ _Maybenull_ TObj *pObj) { apemodevk::TSafeDeleteObj<TObj>(pObj); }
        void operator () (_In_ _Maybenull_ TObj const *pObj) { apemodevk::TSafeDeleteObj<TObj>(pObj); }
    };
}



namespace apemodevk
{
    template <size_t _TEnumSize_>
    struct TIntTypeForEnumOfSize;

    template <>
    struct TIntTypeForEnumOfSize<1>
    {
        static_assert (sizeof (char) == 1, "Size mismatch.");
        using SignedIntType  = char;
        using UnignedIntType = unsigned char;
    };

    template <>
    struct TIntTypeForEnumOfSize<2>
    {
        static_assert (sizeof (short) == 2, "Size mismatch.");
        using SignedIntType  = short;
        using UnignedIntType = unsigned short;
    };

    template <>
    struct TIntTypeForEnumOfSize<4>
    {
        static_assert (sizeof (int) == 4, "Size mismatch.");
        using SignedIntType  = int;
        using UnignedIntType = unsigned int;
    };

    template <>
    struct TIntTypeForEnumOfSize<8>
    {
        static_assert (sizeof (long long) == 8, "Size mismatch.");
        using SignedIntType  = long long;
        using UnignedIntType = unsigned long long;
    };

    // used as an approximation of std::underlying_type<T>
    template <class _TEnum_>
    struct TIntTypeForEnum
    {
        using SignedIntType  = typename TIntTypeForEnumOfSize<sizeof (_TEnum_)>::SignedIntType;
        using UnignedIntType = typename TIntTypeForEnumOfSize<sizeof (_TEnum_)>::UnignedIntType;
    };
}

#define _Game_engine_Define_enum_flag_operators(_TEnum_)                                           \
    \
extern "C++"                                                                                       \
    {                                                                                              \
        \
inline  _TEnum_                                                                                    \
        operator| (_TEnum_ a, _TEnum_ b)                                                           \
        {                                                                                          \
            return _TEnum_ (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) a)                     \
                            | ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));                 \
        }                                                                                          \
        \
inline  _TEnum_ &                                                                                  \
        operator|= (_TEnum_ & a, _TEnum_ b)                                                        \
        {                                                                                          \
            return (_TEnum_ &) (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType &) a)               \
                                |= ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));            \
        }                                                                                          \
        \
inline _TEnum_                                                                                     \
        operator& (_TEnum_ a, _TEnum_ b)                                                           \
        {                                                                                          \
            return _TEnum_ (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) a)                     \
                            & ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));                 \
        }                                                                                          \
        \
inline  _TEnum_ &                                                                                  \
        operator&= (_TEnum_ & a, _TEnum_ b)                                                        \
        {                                                                                          \
            return (_TEnum_ &) (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType &) a)               \
                                &= ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));            \
        }                                                                                          \
        \
inline _TEnum_                                                                                     \
        operator~ (_TEnum_ a)                                                                      \
        {                                                                                          \
            return _TEnum_ (~((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) a));                  \
        }                                                                                          \
        \
inline  _TEnum_                                                                                    \
        operator^ (_TEnum_ a, _TEnum_ b)                                                           \
        {                                                                                          \
            return _TEnum_ (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) a)                     \
                            ^ ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));                 \
        }                                                                                          \
        \
inline  _TEnum_ &                                                                                  \
        operator^= (_TEnum_ & a, _TEnum_ b)                                                        \
        {                                                                                          \
            return (_TEnum_ &) (((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType &) a)               \
                                ^= ((apemodevk::TIntTypeForEnum<_TEnum_>::SignedIntType) b));            \
        }                                                                                          \
    \
}


namespace apemodevk
{
    struct ResultHandle
    {
        enum EValues
        {
            bTrue  = VK_TRUE,
            bFalse = VK_FALSE,

            Success              = VK_SUCCESS,
            NotReady             = VK_NOT_READY,
            Timeout              = VK_TIMEOUT,
            EventSet             = VK_EVENT_SET,
            EventReset           = VK_EVENT_RESET,
            Incomplete           = VK_INCOMPLETE,
            Suboptimal           = VK_SUBOPTIMAL_KHR,
            OutOfSystemMemory    = VK_ERROR_OUT_OF_HOST_MEMORY,
            OutOfDeviceMemory    = VK_ERROR_OUT_OF_DEVICE_MEMORY,
            InitializationFailed = VK_ERROR_INITIALIZATION_FAILED,
            DeviceLost           = VK_ERROR_DEVICE_LOST,
            MemoryMapFailed      = VK_ERROR_MEMORY_MAP_FAILED,
            LayerNotPresent      = VK_ERROR_LAYER_NOT_PRESENT,
            ExtensionNotPresent  = VK_ERROR_EXTENSION_NOT_PRESENT,
            FeatureNotPresent    = VK_ERROR_FEATURE_NOT_PRESENT,
            IncompatibleDriver   = VK_ERROR_INCOMPATIBLE_DRIVER,
            TooManyObjects       = VK_ERROR_TOO_MANY_OBJECTS,
            FormatNotSupproted   = VK_ERROR_FORMAT_NOT_SUPPORTED,
            SurfaceLost          = VK_ERROR_SURFACE_LOST_KHR,
            NativeWindowInUse    = VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,
            OutOfDate            = VK_ERROR_OUT_OF_DATE_KHR,
            IncompatibleDisplay  = VK_ERROR_INCOMPATIBLE_DISPLAY_KHR,
            ValidationFailed     = VK_ERROR_VALIDATION_FAILED_EXT,
            BeginRange           = VK_RESULT_BEGIN_RANGE,
            EndRange             = VK_RESULT_END_RANGE,
            RangeSize            = VK_RESULT_RANGE_SIZE,
        };

        EValues eError;

        inline ResultHandle () : eError (Success)
        {
        }

        inline ResultHandle (VkResult NativeHandle) : eError (static_cast<EValues> (NativeHandle))
        {
        }

        ResultHandle & operator= (ResultHandle const & Other)
        {
            eError = Other.eError;
            return *this;
        }

        inline operator bool () const
        {
            return eError == Success;
        }

        template <typename TEnum>
        inline bool operator== (TEnum Other) const
        {
            return eError == Other;
        }

        template <typename TEnum>
        inline bool operator!= (TEnum Other) const
        {
            return eError != Other;
        }

        template <>
        inline bool operator==<ResultHandle> (ResultHandle Other) const
        {
            return eError == Other.eError;
        }

        template <>
        inline bool operator!=<ResultHandle> (ResultHandle Other) const
        {
            return eError != Other.eError;
        }

        inline bool Succeeded () const
        {
            return eError == Success;
        }

        inline bool Failed () const
        {
            return eError < Success;
        }

        inline VkResult GetNativeObj () const
        {
            return static_cast<VkResult> (eError);
        }

        inline static bool Succeeded (VkResult NativeObj)
        {
            return NativeObj == Success;
        }

        inline static bool Failed (VkResult NativeObj)
        {
            return NativeObj < Success;
        }

        inline static bool Succeeded (VkBool32 NativeObj)
        {
            return NativeObj != bFalse;
        }

        inline static bool Failed (VkBool32 NativeObj)
        {
            return NativeObj == bFalse;
        }
    };

    /* Breaks on failures */
    inline void CheckedCall( const ResultHandle &returnValue ) {
#ifdef _DEBUG
        if ( false == returnValue.Succeeded( ) ) {
            DebugBreak( );
        }
#endif
    }
}

#include <CityHash.h>