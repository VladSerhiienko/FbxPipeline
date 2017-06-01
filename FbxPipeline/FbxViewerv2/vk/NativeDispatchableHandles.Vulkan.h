#pragma once

#include <TDispatchableHandle.Vulkan.h>

namespace apemodevk
{
    template <>
    struct TDispatchableHandleDeleter<VkInstance> : public TDispatchableHandleHandleTypeResolver<VkInstance>
    {
        void operator()(VkInstance & Handle)
        {
            if (Handle == nullptr) return;
            vkDestroyInstance(Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDevice> : public TDispatchableHandleHandleTypeResolver<VkDevice>
    {
        void operator()(VkDevice & Handle)
        {
            if (Handle == nullptr) return;
            vkDestroyDevice(Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkQueue> : public TDispatchableHandleHandleTypeResolver<VkQueue>
    {
        void operator()(VkQueue & Handle)
        {
            // Queues are created along with a logical device creation during vkCreateDevice. Because of this, no
            // explicit deletion of queues is required. All queues associated with a logical device are destroyed when
            // vkDestroyDevice is called on that device.
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkSurfaceKHR> : public TDispatchableHandleHandleTypeResolver<VkSurfaceKHR>
    {
        VkInstance InstanceHandle;

        TDispatchableHandleDeleter() : InstanceHandle(VK_NULL_HANDLE) {}

        void operator()(VkSurfaceKHR & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(InstanceHandle != nullptr, "Instance is required.");
            vkDestroySurfaceKHR(InstanceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkSwapchainKHR> : public TDispatchableHandleHandleTypeResolver<VkSwapchainKHR>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkSwapchainKHR & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroySwapchainKHR(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkCommandPool> : public TDispatchableHandleHandleTypeResolver<VkCommandPool>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkCommandPool & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyCommandPool(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkCommandBuffer> : public TDispatchableHandleHandleTypeResolver<VkCommandBuffer>
    {
        VkDevice LogicalDeviceHandle;
        VkCommandPool CmdPool;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE), CmdPool(VK_NULL_HANDLE) {}

        void operator()(VkCommandBuffer & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkFreeCommandBuffers(LogicalDeviceHandle, CmdPool, 1, &Handle);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkFence> : public TDispatchableHandleHandleTypeResolver<VkFence>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkFence & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyFence(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkEvent> : public TDispatchableHandleHandleTypeResolver<VkEvent>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkEvent & Handle)
        {
            if (Handle == nullptr) return;
            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyEvent(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkImage> : public TDispatchableHandleHandleTypeResolver<VkImage>
    {
        VkDevice LogicalDeviceHandle;
        VkPhysicalDevice PhysicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE)
                                     , PhysicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkImage & Handle)
        {
            if (Handle == nullptr ||
                LogicalDeviceHandle == nullptr) 
                return;

            // Image can be assigned without ownership (by swapchain, for example).
            //_Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyImage(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkImageView> : public TDispatchableHandleHandleTypeResolver<VkImageView>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkImageView & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyImageView(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDeviceMemory> : public TDispatchableHandleHandleTypeResolver<VkDeviceMemory>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkDeviceMemory & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkFreeMemory(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkBuffer> : public TDispatchableHandleHandleTypeResolver<VkBuffer>
    {
        VkDevice LogicalDeviceHandle;
        VkPhysicalDevice PhysicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE)
                                     , PhysicalDeviceHandle(VK_NULL_HANDLE){}

        void operator()(VkBuffer & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyBuffer(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkBufferView> : public TDispatchableHandleHandleTypeResolver<VkBuffer>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkBufferView & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyBufferView(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkFramebuffer> : public TDispatchableHandleHandleTypeResolver<VkFramebuffer>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkFramebuffer & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyFramebuffer(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkRenderPass> : public TDispatchableHandleHandleTypeResolver<VkRenderPass>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkRenderPass & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyRenderPass(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipelineCache> : public TDispatchableHandleHandleTypeResolver<VkPipelineCache>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkPipelineCache & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyPipelineCache(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipeline> : public TDispatchableHandleHandleTypeResolver<VkPipeline>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkPipeline & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyPipeline(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDescriptorSetLayout> : public TDispatchableHandleHandleTypeResolver<VkDescriptorSetLayout>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkDescriptorSetLayout & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyDescriptorSetLayout(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDescriptorSet> : public TDispatchableHandleHandleTypeResolver<VkDescriptorSet>
    {
        VkDevice LogicalDeviceHandle;
        VkDescriptorPool hDescPool;

        TDispatchableHandleDeleter() 
            : LogicalDeviceHandle(VK_NULL_HANDLE)
            , hDescPool(VK_NULL_HANDLE)
        {
        }

        void operator()(VkDescriptorSet & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, 
                                "Device is required.");

            const ResultHandle eIsFreed
                = vkFreeDescriptorSets(LogicalDeviceHandle, hDescPool, 1, &Handle);

            _Game_engine_Assert(eIsFreed.Succeeded(),
                                "Failed to free descriptor set (vkFreeDescriptorSets).");

            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDescriptorPool> : public TDispatchableHandleHandleTypeResolver<VkDescriptorPool>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() 
            : LogicalDeviceHandle(VK_NULL_HANDLE)
        {
        }

        void operator()(VkDescriptorPool & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyDescriptorPool(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipelineLayout> : public TDispatchableHandleHandleTypeResolver<VkPipelineLayout>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkPipelineLayout & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyPipelineLayout(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkShaderModule> : public TDispatchableHandleHandleTypeResolver<VkShaderModule>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkShaderModule & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroyShaderModule(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkSemaphore> : public TDispatchableHandleHandleTypeResolver<VkSemaphore>
    {
        VkDevice LogicalDeviceHandle;

        TDispatchableHandleDeleter() : LogicalDeviceHandle(VK_NULL_HANDLE) {}

        void operator()(VkSemaphore & Handle)
        {
            if (Handle == nullptr) return;

            _Game_engine_Assert(LogicalDeviceHandle != nullptr, "Device is required.");
            vkDestroySemaphore(LogicalDeviceHandle, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <typename TNativeHandle>
    struct TDispatchableHandle : public TDispatchableHandleBase<TNativeHandle>
    {
    };

    template <>
    struct TDispatchableHandle<VkInstance> : public TDispatchableHandleBase<VkInstance>
    {
        bool Recreate (VkInstanceCreateInfo const & CreateInfo)
        {
            Deleter(Handle);

            const ResultHandle ErrorHandle = vkCreateInstance(&CreateInfo, *this, *this);
            _Game_engine_Assert(ErrorHandle, "vkCreateInstance failed.");
            return ErrorHandle.Succeeded();
        }
    };

    template <>
    struct TDispatchableHandle<VkDevice> : public TDispatchableHandleBase<VkDevice>
    {
       bool Recreate(VkPhysicalDevice AdapterHandle, VkDeviceCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(AdapterHandle != VK_NULL_HANDLE, "Adapter must be valid");

            Deleter(Handle);
            if (AdapterHandle != nullptr)
            {
                const ResultHandle ErrorHandle = vkCreateDevice(AdapterHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateDevice failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkQueue> : public TDispatchableHandleBase<VkQueue>
    {
        void Recreate(VkDevice InLogicalDeviceHandle, uint32_t QueueFamilyId, uint32_t QueueId)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device must be valid");

            if (InLogicalDeviceHandle != nullptr)
            {
                vkGetDeviceQueue(InLogicalDeviceHandle, QueueFamilyId, QueueId, *this);
            }
        }
    };

    template <>
    struct TDispatchableHandle<VkSurfaceKHR> : public TDispatchableHandleBase<VkSurfaceKHR>
    {
        bool Recreate(VkInstance InInstanceHandle, VkWin32SurfaceCreateInfoKHR const &CreateInfo)
        {
            _Game_engine_Assert(InInstanceHandle != VK_NULL_HANDLE, "Instance is required.");

            Deleter(Handle);
            if (InInstanceHandle)
            {
                Deleter.InstanceHandle = InInstanceHandle;
                ResultHandle ErrorHandle = vkCreateWin32SurfaceKHR(InInstanceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateWin32SurfaceKHR failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkSwapchainKHR> : public TDispatchableHandleBase<VkSwapchainKHR>
    {
        /**
         * Creates swapchain.
         * Handles deleting of the old swapchain.
         */
       bool Recreate(VkDevice InLogicalDeviceHandle, VkSwapchainCreateInfoKHR const & CreateInfo)
        {
            auto PrevHandle = Handle;
            TDeleter PrevDeleter = Deleter;

            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Instance is required.");

            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateSwapchainKHR(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateSwapchainKHR failed.");
                if (ErrorHandle.Succeeded())
                {
                    // If we just re-created an existing swapchain, we should destroy the old
                    // swapchain at this point.

                    // NOTE: 
                    //      Destroying the swapchain also cleans up all its associated
                    //      presentable images once the platform is done with them.

                    PrevDeleter(PrevHandle);
                    return true;
                }
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkCommandPool> : public TDispatchableHandleBase<VkCommandPool>
    {
       bool Recreate(VkDevice InLogicalDeviceHandle, VkCommandPoolCreateInfo const &CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;
                ResultHandle ErrorHandle = vkCreateCommandPool(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateCommandPool failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }

       bool Reset(bool bRecycleResources)
        {
            if (IsNotNull())
            {
                const auto ResetFlags = bRecycleResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0u;
                const ResultHandle ErrorHandle = vkResetCommandPool(Deleter.LogicalDeviceHandle, *this, ResetFlags);
                _Game_engine_Assert(ErrorHandle, "vkResetCommandPool failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkCommandBuffer> : public TDispatchableHandleBase<VkCommandBuffer>
    {
       bool Recreate(VkDevice InLogicalDeviceHandle, VkCommandBufferAllocateInfo const &AllocInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");
            _Game_engine_Assert(AllocInfo.commandPool != VK_NULL_HANDLE, "No default pools available.");
            _Game_engine_Assert(AllocInfo.commandBufferCount == 1, "This method handles single command list.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.CmdPool = AllocInfo.commandPool;
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkAllocateCommandBuffers(InLogicalDeviceHandle, &AllocInfo, *this);
                _Game_engine_Assert(ErrorHandle, "vkAllocateCommandBuffers failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkFence> : public TDispatchableHandleBase<VkFence>
    {
       bool Recreate(VkDevice InLogicalDeviceHandle, VkFenceCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateFence(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateFence failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }

       bool Await() const
        {
            const ResultHandle ErrorHandle = vkWaitForFences(Deleter.LogicalDeviceHandle, 1, &Handle, true, UINT64_MAX);
            _Game_engine_Assert(ErrorHandle, "vkWaitForFences failed.");
            return ErrorHandle.Succeeded();
        }

        _Force_inline_function VkResult Status() const { return vkGetFenceStatus(Deleter.LogicalDeviceHandle, Handle); }
        _Force_inline_function bool     IsSignalled() const { return Status() == ResultHandle::Success; }
        _Force_inline_function bool     IsInProgress() const { return Status() == ResultHandle::NotReady; }
        _Force_inline_function bool     Failed() const { return ResultHandle(Status()).Failed(); }
    };

    template <>
    struct TDispatchableHandle<VkEvent> : public TDispatchableHandleBase<VkEvent>
    {
       bool Recreate(VkDevice InLogicalDeviceHandle, VkEventCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateEvent(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateFence failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }

       bool Set() const
        {
            const ResultHandle ErrorHandle = vkSetEvent(Deleter.LogicalDeviceHandle, Handle);
            _Game_engine_Assert(ErrorHandle, "vkSetEvent failed.");
            return ErrorHandle.Succeeded();
        }

       bool Reset() const
        {
            const ResultHandle ErrorHandle = vkResetEvent(Deleter.LogicalDeviceHandle, Handle);
            _Game_engine_Assert(ErrorHandle, "vkResetEvent failed.");
            return ErrorHandle.Succeeded();
        }

        _Force_inline_function VkResult Status() const { return vkGetEventStatus(Deleter.LogicalDeviceHandle, Handle); }
        _Force_inline_function bool     IsSet() const { return Status() == ResultHandle::EventSet; }
        _Force_inline_function bool     IsReset() const { return Status() == ResultHandle::EventReset; }
        _Force_inline_function bool     Failed() const { return ResultHandle(Status()).Failed(); }
    };

    inline uint32_t ResolveMemoryType(VkPhysicalDevice gpu, VkMemoryPropertyFlags properties, uint32_t type_bits)
    {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(gpu, &physicalDeviceMemoryProperties);
        for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
            if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
                return i;

        // Unable to find memoryType
        return uint32_t(-1);
    }

    template <>
    struct TDispatchableHandle<VkImage> : public TDispatchableHandleBase<VkImage>
    {
        bool Assign(VkDevice InLogicalDeviceHandle, VkPhysicalDevice InPhysicalDeviceHandle, VkImage Img, bool bTakeOwnership)
        {
            Deleter(Handle);

            const bool bCaseOk = InLogicalDeviceHandle != VK_NULL_HANDLE && Img != VK_NULL_HANDLE;
            const bool bCaseNull = InLogicalDeviceHandle == VK_NULL_HANDLE && Img == VK_NULL_HANDLE;
            _Game_engine_Assert(bCaseOk || bCaseNull, "Both handles should be null or valid.");

            if (bCaseOk || bCaseNull)
            {
                Handle = Img;
                Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
                Deleter.LogicalDeviceHandle = bTakeOwnership 
                    ? InLogicalDeviceHandle 
                    : nullptr;
                return true;
            }

            return false;
        }

        bool Recreate(VkDevice InLogicalDeviceHandle, VkPhysicalDevice InPhysicalDeviceHandle, VkImageCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;
                Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateImage(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateImage failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }

        VkMemoryRequirements GetMemoryRequirements() {
            _Game_engine_Assert(IsNotNull(), "Null.");

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(Deleter.LogicalDeviceHandle, *this, &memoryRequirements);
            return memoryRequirements;
        }

        VkMemoryAllocateInfo GetMemoryAllocateInfo(VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {

            VkMemoryRequirements memoryRequirements = GetMemoryRequirements();

            VkMemoryAllocateInfo memoryAllocInfo;
            apemodevk::ZeroMemory(memoryAllocInfo);
            memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = ResolveMemoryType(Deleter.PhysicalDeviceHandle, memoryPropertyFlags, memoryRequirements.memoryTypeBits);

            return memoryAllocInfo;
        }

        bool BindMemory(VkDeviceMemory hMemory, uint32_t Offset = 0) {
            const ResultHandle ErrorHandle = vkBindImageMemory(Deleter.LogicalDeviceHandle, *this, hMemory, Offset);
            return ErrorHandle.Succeeded();
        }
    };

    template <>
    struct TDispatchableHandle<VkImageView> : public TDispatchableHandleBase<VkImageView>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkImageViewCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateImageView(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateImageView failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkDeviceMemory> : public TDispatchableHandleBase<VkDeviceMemory>
    {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkMemoryAllocateInfo const &AllocInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkAllocateMemory( InLogicalDeviceHandle, &AllocInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkAllocateMemory failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }

        uint8_t *Map( uint32_t mappedMemoryOffset, uint32_t mappedMemorySize, VkMemoryHeapFlags mapFlags ) {
            _Game_engine_Assert( Deleter.LogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            uint8_t *mappedData = nullptr;
            if ( ResultHandle::Failed( vkMapMemory( Deleter.LogicalDeviceHandle,
                                                    *this,
                                                    mappedMemoryOffset,
                                                    mappedMemorySize,
                                                    mapFlags,
                                                    reinterpret_cast< void ** >( &mappedData ) ) ) )
                return nullptr;

            return mappedData;
        }

        void Unmap( ) {
            _Game_engine_Assert( Deleter.LogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );
            vkUnmapMemory( Deleter.LogicalDeviceHandle, *this );
        }
    };

    template <>
    struct TDispatchableHandle<VkBuffer> : public TDispatchableHandleBase<VkBuffer>
    {
        bool Recreate( VkDevice                  InLogicalDeviceHandle,
                       VkPhysicalDevice          InPhysicalDeviceHandle,
                       VkBufferCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle  = InLogicalDeviceHandle;
                Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateBuffer( InLogicalDeviceHandle, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateBuffer failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            _Game_engine_Assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements( Deleter.LogicalDeviceHandle, *this, &memoryRequirements );
            return memoryRequirements;
        }

        VkMemoryAllocateInfo GetMemoryAllocateInfo( VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) {
            VkMemoryRequirements memoryRequirements = GetMemoryRequirements( );

            VkMemoryAllocateInfo memoryAllocInfo;
            apemodevk::ZeroMemory(memoryAllocInfo);
            memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocInfo.allocationSize = memoryRequirements.size;
            memoryAllocInfo.memoryTypeIndex = ResolveMemoryType( Deleter.PhysicalDeviceHandle, memoryPropertyFlags, memoryRequirements.memoryTypeBits );

            return memoryAllocInfo;
        }

        bool BindMemory( VkDeviceMemory hMemory, uint32_t Offset = 0 ) {
            const ResultHandle ErrorHandle = vkBindBufferMemory( Deleter.LogicalDeviceHandle, *this, hMemory, Offset );
            return ErrorHandle.Succeeded( );
        }
    };

    template <>
    struct TDispatchableHandle<VkBufferView> : public TDispatchableHandleBase<VkBufferView>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkBufferViewCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateBufferView(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateBufferView failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkFramebuffer> : public TDispatchableHandleBase<VkFramebuffer>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkFramebufferCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateFramebuffer(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateFramebuffer failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkRenderPass> : public TDispatchableHandleBase<VkRenderPass>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkRenderPassCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateRenderPass(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateRenderPass failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkPipelineCache> : public TDispatchableHandleBase<VkPipelineCache>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkPipelineCacheCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreatePipelineCache(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreatePipelineCache failed.");
                return ErrorHandle.Succeeded();
            }
        }
    };

    template <>
    struct TDispatchableHandle<VkPipelineLayout> : public TDispatchableHandleBase<VkPipelineLayout>
    {
        bool Recreate(VkDevice InLogicalDeviceHandle, VkPipelineLayoutCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreatePipelineLayout(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreatePipelineLayout failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle<VkDescriptorSetLayout> : public TDispatchableHandleBase<VkDescriptorSetLayout>
    {
        using Vector = std::vector<TDispatchableHandle<VkDescriptorSetLayout>>;
        using NativeHandleVector = std::vector<VkDescriptorSetLayout>;

        TDispatchableHandle()
        {
            Handle                      = VK_NULL_HANDLE;
            Deleter.LogicalDeviceHandle = VK_NULL_HANDLE;
        }

        TDispatchableHandle(TDispatchableHandle<VkDescriptorSetLayout> && Other)
        {
            Handle = Other.Release();
            Deleter.LogicalDeviceHandle = Other.Deleter.LogicalDeviceHandle;
        }

        TDispatchableHandle(VkDevice InLogicalDeviceHandle, VkDescriptorSetLayout SetLayoutHandle)
        {
            Handle                      = SetLayoutHandle;
            Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;
        }

        bool Recreate(VkDevice InLogicalDeviceHandle, VkDescriptorSetLayoutCreateInfo const & CreateInfo)
        {
            _Game_engine_Assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter(Handle);
            if (InLogicalDeviceHandle != nullptr)
            {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateDescriptorSetLayout(InLogicalDeviceHandle, &CreateInfo, *this, *this);
                _Game_engine_Assert(ErrorHandle, "vkCreateDescriptorSetLayout failed.");
                return ErrorHandle.Succeeded();
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkDescriptorSet > : public TDispatchableHandleBase< VkDescriptorSet > {
        bool Recreate( VkDevice InLogicalDeviceHandle,
                       VkDescriptorPool InDescPoolHandle,
                       VkDescriptorSetAllocateInfo const &AllocInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkAllocateDescriptorSets( InLogicalDeviceHandle, &AllocInfo, *this );
                _Game_engine_Assert( ErrorHandle, "vkAllocateDescriptorSets failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkDescriptorPool > : public TDispatchableHandleBase< VkDescriptorPool > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkDescriptorPoolCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateDescriptorPool( InLogicalDeviceHandle, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateDescriptorSetLayout failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkPipeline > : public TDispatchableHandleBase< VkPipeline > {
        bool Recreate( VkDevice InLogicalDeviceHandle,
                       VkPipelineCache pCache,
                       VkGraphicsPipelineCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateGraphicsPipelines( InLogicalDeviceHandle, pCache, 1, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateGraphicsPipelines failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }

        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineCache pCache, VkComputePipelineCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle =
                    vkCreateComputePipelines( InLogicalDeviceHandle, pCache, 1, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateGraphicsPipelines failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkShaderModule > : public TDispatchableHandleBase< VkShaderModule > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkShaderModuleCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateShaderModule( InLogicalDeviceHandle, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateShaderModule failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkSemaphore > : public TDispatchableHandleBase< VkSemaphore > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSemaphoreCreateInfo const &CreateInfo ) {
            _Game_engine_Assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.LogicalDeviceHandle = InLogicalDeviceHandle;

                const ResultHandle ErrorHandle = vkCreateSemaphore( InLogicalDeviceHandle, &CreateInfo, *this, *this );
                _Game_engine_Assert( ErrorHandle, "vkCreateSemaphore failed." );
                return ErrorHandle.Succeeded( );
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkSampler > : public TDispatchableHandleBase< VkSampler > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSamplerCreateInfo const &CreateInfo ) {
            Deleter( Handle );

            const ResultHandle ErrorHandle = vkCreateSampler( InLogicalDeviceHandle, &CreateInfo, *this, *this );
            _Game_engine_Assert( ErrorHandle, "vkCreateInstance failed." );
            return ErrorHandle.Succeeded( );
        }
    };
}
