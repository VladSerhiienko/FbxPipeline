#pragma once

namespace Vulkan {
    static const char* KnownValidationLayers[] = {
        "VK_LAYER_LUNARG_threading",
        "VK_LAYER_LUNARG_mem_tracker",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_draw_state",
        "VK_LAYER_LUNARG_param_checker",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_LUNARG_device_limits",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_GOOGLE_unique_objects",
    };

    static size_t const KnownValidationLayerCount = _Get_array_length(KnownValidationLayers);
}