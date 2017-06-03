#pragma once

//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <ResultHandle.Vulkan.h>

namespace TbbAux
{
    // TODO:
    //      Track what objects were created
    //      Track memory leaks

    class _Graphics_ecosystem_dll_api AllocationCallbacks
    {
        static void * VKAPI_PTR AllocationFunction(void * pUserData, size_t size, size_t alignment,
                                                   VkSystemAllocationScope allocationScope);

        static void * VKAPI_PTR ReallocationFunction(void * pUserData, void * pOriginal, size_t size, size_t alignment,
                                                     VkSystemAllocationScope allocationScope);

        static void VKAPI_PTR FreeFunction(void * pUserData, void * pMemory);

        static void VKAPI_PTR InternalAllocationNotification(void * pUserData, size_t size,
                                                             VkInternalAllocationType allocationType,
                                                             VkSystemAllocationScope allocationScope);

        static void VKAPI_PTR InternalFreeNotification(void * pUserData, size_t size,
                                                       VkInternalAllocationType allocationType,
                                                       VkSystemAllocationScope allocationScope);

    public:
        static VkAllocationCallbacks AllocCallbacks;
        static void Initialize(void * pUserData);

    public:
        inline operator VkAllocationCallbacks *()
        {
            return &AllocCallbacks;
        }
        inline operator VkAllocationCallbacks const *() const
        {
            return &AllocCallbacks;
        }
    };
}
