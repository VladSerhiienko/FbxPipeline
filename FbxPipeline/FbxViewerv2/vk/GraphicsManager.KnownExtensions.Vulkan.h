#pragma once

namespace Vulkan
{
    // Device extensions to enable
    static const char * KnownDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Global extensions to enable
    static char const * KnownInstanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if VK_USE_PLATFORM_WIN32_KHR
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_ANDROID_KHR
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
    };

    static size_t const KnownDeviceExtensionCount   = _Get_array_length(KnownDeviceExtensions);
    static size_t const KnownInstanceExtensionCount = _Get_array_length(KnownInstanceExtensions);
}