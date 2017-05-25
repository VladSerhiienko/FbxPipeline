#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    struct ExecutionSync
    {
        std::vector<VkFence>     hFences;
        std::vector<VkSemaphore> hSemaphores;
    };
}