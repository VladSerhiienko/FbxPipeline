#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    struct ExecutionSync
    {
        std::vector<VkFence>     hFences;
        std::vector<VkSemaphore> hSemaphores;
    };
}