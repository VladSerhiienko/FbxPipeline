#pragma once

#include <Platform.Vulkan.h>

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
            platform::DebugBreak( );
        }
#endif
    }
}

#include <CityHash.h>