#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemodevk
{
    struct ExecutionSync
    {
        std::vector<VkFence>     hFences;
        std::vector<VkSemaphore> hSemaphores;
    };
}