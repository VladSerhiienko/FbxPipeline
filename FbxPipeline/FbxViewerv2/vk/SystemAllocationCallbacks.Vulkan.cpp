//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <ScalableAlignedPool.h>
#include <SystemAllocationCallbacks.Vulkan.h>

VkAllocationCallbacks TbbAux::AllocationCallbacks::AllocCallbacks;

void * TbbAux::AllocationCallbacks::AllocationFunction(void * pUserData, size_t size, size_t alignment,
                                                       VkSystemAllocationScope allocationScope)
{
    _Unreferenced_scope(pUserData);
    _Unreferenced_scope(allocationScope);
    return scalable_aligned_malloc(size, alignment);
}

void * TbbAux::AllocationCallbacks::ReallocationFunction(void * pUserData, void * pOriginal, size_t size,
                                                         size_t alignment, VkSystemAllocationScope allocationScope)
{
    _Unreferenced_scope(pUserData);
    _Unreferenced_scope(allocationScope);
    return scalable_aligned_realloc(pOriginal, size, alignment);
}

void TbbAux::AllocationCallbacks::FreeFunction(void * pUserData, void * pMemory)
{
    _Unreferenced_scope(pUserData);
    return scalable_aligned_free(pMemory);
}

void TbbAux::AllocationCallbacks::InternalAllocationNotification(void * pUserData, size_t size,
                                                                 VkInternalAllocationType allocationType,
                                                                 VkSystemAllocationScope allocationScope)
{
    _Unreferenced_scope(pUserData);
    _Unreferenced_scope(size);
    _Unreferenced_scope(allocationType);
    _Unreferenced_scope(allocationScope);
}

void TbbAux::AllocationCallbacks::InternalFreeNotification(void * pUserData, size_t size,
                                                           VkInternalAllocationType allocationType,
                                                           VkSystemAllocationScope allocationScope)
{
    _Unreferenced_scope(pUserData);
    _Unreferenced_scope(size);
    _Unreferenced_scope(allocationType);
    _Unreferenced_scope(allocationScope);
}

void TbbAux::AllocationCallbacks::Initialize(void * pUserData)
{
    AllocCallbacks.pfnAllocation         = AllocationFunction;
    AllocCallbacks.pfnReallocation       = ReallocationFunction;
    AllocCallbacks.pfnFree               = FreeFunction;
    AllocCallbacks.pfnInternalAllocation = InternalAllocationNotification;
    AllocCallbacks.pfnInternalFree       = InternalFreeNotification;
    AllocCallbacks.pUserData             = pUserData;
}
