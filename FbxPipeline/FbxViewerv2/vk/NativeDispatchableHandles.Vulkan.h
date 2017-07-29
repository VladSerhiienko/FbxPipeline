#pragma once

#include <TDispatchableHandle.Vulkan.h>

namespace apemodevk
{
    template <>
    struct TDispatchableHandleDeleter< VkInstance > : public TDispatchableHandleHandleTypeResolver< VkInstance > {
        void operator( )( VkInstance &Handle ) {
            if ( Handle == nullptr )
                return;
            vkDestroyInstance( Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkDevice > : public TDispatchableHandleHandleTypeResolver< VkDevice > {
        void operator( )( VkDevice &Handle ) {
            if ( Handle == nullptr )
                return;
            vkDestroyDevice( Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkQueue > : public TDispatchableHandleHandleTypeResolver< VkQueue > {
        void operator( )( VkQueue &Handle ) {
            // Queues are created along with a logical device creation during vkCreateDevice. Because of this, no
            // explicit deletion of queues is required. All queues associated with a logical device are destroyed when
            // vkDestroyDevice is called on that device.
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkSurfaceKHR > : public TDispatchableHandleHandleTypeResolver< VkSurfaceKHR > {
        VkInstance hInstance;

        TDispatchableHandleDeleter( ) : hInstance( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSurfaceKHR &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hInstance != nullptr, "Instance is required." );
            vkDestroySurfaceKHR( hInstance, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkSwapchainKHR > : public TDispatchableHandleHandleTypeResolver< VkSwapchainKHR > {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSwapchainKHR &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroySwapchainKHR( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkCommandPool > : public TDispatchableHandleHandleTypeResolver< VkCommandPool > {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkCommandPool &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyCommandPool( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkCommandBuffer > : public TDispatchableHandleHandleTypeResolver< VkCommandBuffer > {
        VkDevice      hLogicalDevice;
        VkCommandPool CmdPool;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), CmdPool( VK_NULL_HANDLE ) {
        }

        void operator( )( VkCommandBuffer &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkFreeCommandBuffers( hLogicalDevice, CmdPool, 1, &Handle );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkFence > : public TDispatchableHandleHandleTypeResolver< VkFence > {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkFence &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyFence( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkEvent > : public TDispatchableHandleHandleTypeResolver< VkEvent > {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ) {
        }

        void operator( )( VkEvent &Handle ) {
            if ( Handle == nullptr )
                return;
            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroyEvent( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkImage > : public TDispatchableHandleHandleTypeResolver< VkImage > {
        VkDevice         hLogicalDevice;
        VkPhysicalDevice PhysicalDeviceHandle;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), PhysicalDeviceHandle( VK_NULL_HANDLE ) {
        }

        void operator( )( VkImage &Handle ) {
            if ( Handle == nullptr || hLogicalDevice == nullptr )
                return;

            // Image can be assigned without ownership (by swapchain, for example).
            // apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyImage( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkSampler > : public TDispatchableHandleHandleTypeResolver< VkSampler > {
        VkDevice         hLogicalDevice;
        VkPhysicalDevice PhysicalDeviceHandle;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), PhysicalDeviceHandle( VK_NULL_HANDLE ) {
        }

        void operator( )( VkSampler &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            vkDestroySampler( hLogicalDevice, Handle, *this );
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkImageView> : public TDispatchableHandleHandleTypeResolver<VkImageView>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkImageView & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyImageView(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDeviceMemory> : public TDispatchableHandleHandleTypeResolver<VkDeviceMemory>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkDeviceMemory & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkFreeMemory(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkBuffer> : public TDispatchableHandleHandleTypeResolver<VkBuffer>
    {
        VkDevice hLogicalDevice;
        VkPhysicalDevice PhysicalDeviceHandle;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE)
                                     , PhysicalDeviceHandle(VK_NULL_HANDLE){}

        void operator()(VkBuffer & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyBuffer(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkBufferView> : public TDispatchableHandleHandleTypeResolver<VkBuffer>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkBufferView & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyBufferView(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkFramebuffer> : public TDispatchableHandleHandleTypeResolver<VkFramebuffer>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkFramebuffer & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyFramebuffer(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkRenderPass> : public TDispatchableHandleHandleTypeResolver<VkRenderPass>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkRenderPass & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyRenderPass(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipelineCache> : public TDispatchableHandleHandleTypeResolver<VkPipelineCache>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkPipelineCache & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyPipelineCache(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipeline> : public TDispatchableHandleHandleTypeResolver<VkPipeline>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkPipeline & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyPipeline(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDescriptorSetLayout> : public TDispatchableHandleHandleTypeResolver<VkDescriptorSetLayout>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkDescriptorSetLayout & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyDescriptorSetLayout(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter< VkDescriptorSet > : public TDispatchableHandleHandleTypeResolver< VkDescriptorSet > {
        VkDevice         hLogicalDevice;
        VkDescriptorPool hDescPool;

        TDispatchableHandleDeleter( ) : hLogicalDevice( VK_NULL_HANDLE ), hDescPool( VK_NULL_HANDLE ) {
        }

        void operator( )( VkDescriptorSet &Handle ) {
            if ( Handle == nullptr )
                return;

            apemode_assert( hLogicalDevice != nullptr, "Device is required." );
            const VkResult eIsFreed = vkFreeDescriptorSets( hLogicalDevice, hDescPool, 1, &Handle );
            apemode_assert( VK_SUCCESS == eIsFreed, "Failed to free descriptor set (vkFreeDescriptorSets)." );

            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkDescriptorPool> : public TDispatchableHandleHandleTypeResolver<VkDescriptorPool>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() 
            : hLogicalDevice(VK_NULL_HANDLE)
        {
        }

        void operator()(VkDescriptorPool & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyDescriptorPool(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkPipelineLayout> : public TDispatchableHandleHandleTypeResolver<VkPipelineLayout>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkPipelineLayout & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyPipelineLayout(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkShaderModule> : public TDispatchableHandleHandleTypeResolver<VkShaderModule>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkShaderModule & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroyShaderModule(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template <>
    struct TDispatchableHandleDeleter<VkSemaphore> : public TDispatchableHandleHandleTypeResolver<VkSemaphore>
    {
        VkDevice hLogicalDevice;

        TDispatchableHandleDeleter() : hLogicalDevice(VK_NULL_HANDLE) {}

        void operator()(VkSemaphore & Handle)
        {
            if (Handle == nullptr) return;

            apemode_assert(hLogicalDevice != nullptr, "Device is required.");
            vkDestroySemaphore(hLogicalDevice, Handle, *this);
            Handle = VK_NULL_HANDLE;
        }
    };

    template < typename TNativeHandle >
    struct TDispatchableHandle : public TDispatchableHandleBase< TNativeHandle > {};

    template <>
    struct TDispatchableHandle< VkInstance > : public TDispatchableHandleBase< VkInstance > {
        bool Recreate( VkInstanceCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            return VK_SUCCESS == CheckedCall( vkCreateInstance( &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkDevice > : public TDispatchableHandleBase< VkDevice > {
        bool Recreate( VkPhysicalDevice pPhysicalDevice, VkDeviceCreateInfo const &CreateInfo ) {
            apemode_assert( pPhysicalDevice != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            return VK_SUCCESS == CheckedCall( vkCreateDevice( pPhysicalDevice, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkQueue > : public TDispatchableHandleBase< VkQueue > {
        void Recreate( VkDevice InLogicalDeviceHandle, uint32_t QueueFamilyId, uint32_t QueueId ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            if ( InLogicalDeviceHandle != nullptr ) {
                vkGetDeviceQueue( InLogicalDeviceHandle, QueueFamilyId, QueueId, *this );
            }
        }

        void WaitIdle( ) {
            vkQueueWaitIdle( *this );
        }
    };

    template <>
    struct TDispatchableHandle< VkSurfaceKHR > : public TDispatchableHandleBase< VkSurfaceKHR > {
        bool Recreate( VkInstance InInstanceHandle, VkWin32SurfaceCreateInfoKHR const &CreateInfo ) {
            apemode_assert( InInstanceHandle != VK_NULL_HANDLE, "Instance is required." );

            Deleter( Handle );
            Deleter.hInstance = InInstanceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateWin32SurfaceKHR( InInstanceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkSwapchainKHR > : public TDispatchableHandleBase< VkSwapchainKHR > {
        /**
         * Creates swapchain.
         * Handles deleting of the old swapchain.
         **/
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSwapchainCreateInfoKHR const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            auto     PrevHandle  = Handle;
            TDeleter PrevDeleter = Deleter;

            if ( InLogicalDeviceHandle != nullptr ) {
                Deleter.hLogicalDevice = InLogicalDeviceHandle;
                if ( VK_SUCCESS == CheckedCall( vkCreateSwapchainKHR( InLogicalDeviceHandle, &CreateInfo, *this, *this ) ) ) {
                    /** 
                     * If we just re-created an existing swapchain, we should destroy the old
                     * swapchain at this point.
                     *
                     * @note Destroying the swapchain also cleans up all its associated
                     *       presentable images once the platform is done with them.
                     **/

                    PrevDeleter( PrevHandle );
                    return true;
                }
            }

            return false;
        }
    };

    template <>
    struct TDispatchableHandle< VkCommandPool > : public TDispatchableHandleBase< VkCommandPool > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkCommandPoolCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateCommandPool( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }

        bool Reset( bool bRecycleResources ) {
            assert( IsNotNull( ) );
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );
            const auto ResetFlags = bRecycleResources ? VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT : 0u;
            return VK_SUCCESS == CheckedCall( vkResetCommandPool( Deleter.hLogicalDevice, *this, ResetFlags ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkCommandBuffer > : public TDispatchableHandleBase< VkCommandBuffer > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkCommandBufferAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );
            apemode_assert( AllocInfo.commandPool != VK_NULL_HANDLE, "No default pools available." );
            apemode_assert( AllocInfo.commandBufferCount == 1, "This method handles single command list." );

            Deleter( Handle );
            Deleter.CmdPool        = AllocInfo.commandPool;
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkAllocateCommandBuffers( InLogicalDeviceHandle, &AllocInfo, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkFence > : public TDispatchableHandleBase< VkFence > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkFenceCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateFence( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }

        bool Wait( uint64_t Timeout = UINT64_MAX ) const {
            return VK_SUCCESS == CheckedCall( vkWaitForFences( Deleter.hLogicalDevice, 1, &Handle, true, Timeout ) );
        }

        inline VkResult Status() const       { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ); }
        inline bool     IsSignalled() const  { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) == VK_SUCCESS; }
        inline bool     IsInProgress() const { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) == VK_NOT_READY; }
        inline bool     Failed() const       { return vkGetFenceStatus( Deleter.hLogicalDevice, Handle ) < VK_SUCCESS; }
    };

    template <>
    struct TDispatchableHandle< VkEvent > : public TDispatchableHandleBase< VkEvent > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkEventCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateEvent( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }

        bool Set( ) const {
            return VK_SUCCESS == CheckedCall( vkSetEvent( Deleter.hLogicalDevice, Handle ) );
        }

        bool Reset( ) const {
            return VK_SUCCESS == CheckedCall( vkResetEvent( Deleter.hLogicalDevice, Handle ) );
        }

        inline VkResult Status( ) const  { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ); }
        inline bool     IsSet( ) const   { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) == VK_EVENT_SET; }
        inline bool     IsReset( ) const { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) == VK_EVENT_RESET; }
        inline bool     Failed( ) const  { return vkGetEventStatus( Deleter.hLogicalDevice, Handle ) < VK_SUCCESS; }
    };

    inline uint32_t ResolveMemoryType( VkPhysicalDevice gpu, VkMemoryPropertyFlags properties, uint32_t type_bits ) {
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties( gpu, &physicalDeviceMemoryProperties );
        for ( uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++ )
            if ( ( physicalDeviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties ) == properties && type_bits & ( 1 << i ) )
                return i;

        // Unable to find memoryType
        DebugBreak( );
        return uint32_t( -1 );
    }

    template <>
    struct TDispatchableHandle<VkImage> : public TDispatchableHandleBase<VkImage>
    {
        bool Assign( VkDevice         InLogicalDeviceHandle,
                     VkPhysicalDevice InPhysicalDeviceHandle,
                     VkImage          Img,
                     bool             bTakeOwnership ) {
            Deleter( Handle );

            const bool bCaseOk = InLogicalDeviceHandle != VK_NULL_HANDLE && Img != VK_NULL_HANDLE;
            const bool bCaseNull = InLogicalDeviceHandle == VK_NULL_HANDLE && Img == VK_NULL_HANDLE;
            apemode_assert( bCaseOk || bCaseNull, "Both handles should be null or valid." );

            if ( bCaseOk || bCaseNull ) {
                Handle = Img;
                Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
                Deleter.hLogicalDevice = bTakeOwnership ? InLogicalDeviceHandle : nullptr;
                return true;
            }

            return false;
        }

        bool Recreate( VkDevice                 InLogicalDeviceHandle,
                       VkPhysicalDevice         InPhysicalDeviceHandle,
                       VkImageCreateInfo const &CreateInfo ) {
            apemode_assert(InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required.");

            Deleter( Handle );
            Deleter.hLogicalDevice       = InLogicalDeviceHandle;
            Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateImage( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements( Deleter.hLogicalDevice, *this, &memoryRequirements );
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

        bool BindMemory( VkDeviceMemory hMemory, uint32_t Offset = 0 ) {
            return VK_SUCCESS == CheckedCall( vkBindImageMemory( Deleter.hLogicalDevice, *this, hMemory, Offset ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkImageView > : public TDispatchableHandleBase< VkImageView > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkImageViewCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateImageView( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle<VkDeviceMemory> : public TDispatchableHandleBase<VkDeviceMemory>
    {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkMemoryAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkAllocateMemory( InLogicalDeviceHandle, &AllocInfo, *this, *this ) );
        }

        uint8_t *Map( uint32_t mappedMemoryOffset, uint32_t mappedMemorySize, VkMemoryHeapFlags mapFlags ) {
            apemode_assert( Deleter.hLogicalDevice != VK_NULL_HANDLE, "Device is required." );

            uint8_t *mappedData = nullptr;
            if ( VK_SUCCESS == CheckedCall( vkMapMemory( Deleter.hLogicalDevice,
                                                         *this,
                                                         mappedMemoryOffset,
                                                         mappedMemorySize,
                                                         mapFlags,
                                                         reinterpret_cast< void ** >( &mappedData ) ) ) )
                return mappedData;
            return nullptr;
        }

        void Unmap( ) {
            apemode_assert( Deleter.hLogicalDevice != VK_NULL_HANDLE, "Device is required." );
            vkUnmapMemory( Deleter.hLogicalDevice, *this );
        }
    };

    template <>
    struct TDispatchableHandle< VkBuffer > : public TDispatchableHandleBase< VkBuffer > {
        bool Recreate( VkDevice                  InLogicalDeviceHandle,
                       VkPhysicalDevice          InPhysicalDeviceHandle,
                       VkBufferCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice       = InLogicalDeviceHandle;
            Deleter.PhysicalDeviceHandle = InPhysicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateBuffer( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }

        VkMemoryRequirements GetMemoryRequirements( ) {
            apemode_assert( IsNotNull( ), "Null." );

            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements( Deleter.hLogicalDevice, *this, &memoryRequirements );
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
            return VK_SUCCESS == CheckedCall( vkBindBufferMemory( Deleter.hLogicalDevice, *this, hMemory, Offset ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkBufferView > : public TDispatchableHandleBase< VkBufferView > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkBufferViewCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateBufferView( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkFramebuffer > : public TDispatchableHandleBase< VkFramebuffer > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkFramebufferCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateFramebuffer( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkRenderPass > : public TDispatchableHandleBase< VkRenderPass > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkRenderPassCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateRenderPass( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkPipelineCache > : public TDispatchableHandleBase< VkPipelineCache > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineCacheCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreatePipelineCache( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkPipelineLayout > : public TDispatchableHandleBase< VkPipelineLayout > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineLayoutCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreatePipelineLayout( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkDescriptorSetLayout > : public TDispatchableHandleBase< VkDescriptorSetLayout > {
        using Vector             = std::vector< TDispatchableHandle< VkDescriptorSetLayout > >;
        using NativeHandleVector = std::vector< VkDescriptorSetLayout >;

        TDispatchableHandle( ) {
            Handle                 = VK_NULL_HANDLE;
            Deleter.hLogicalDevice = VK_NULL_HANDLE;
        }

        TDispatchableHandle( TDispatchableHandle< VkDescriptorSetLayout > &&Other ) {
            Handle                 = Other.Release( );
            Deleter.hLogicalDevice = Other.Deleter.hLogicalDevice;
        }

        TDispatchableHandle( VkDevice InLogicalDeviceHandle, VkDescriptorSetLayout SetLayoutHandle ) {
            Handle                 = SetLayoutHandle;
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
        }

        bool Recreate( VkDevice InLogicalDeviceHandle, VkDescriptorSetLayoutCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateDescriptorSetLayout( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkDescriptorSet > : public TDispatchableHandleBase< VkDescriptorSet > {
        bool Recreate( VkDevice                           InLogicalDeviceHandle,
                       VkDescriptorPool                   InDescPoolHandle,
                       VkDescriptorSetAllocateInfo const &AllocInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkAllocateDescriptorSets( InLogicalDeviceHandle, &AllocInfo, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkDescriptorPool > : public TDispatchableHandleBase< VkDescriptorPool > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkDescriptorPoolCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateDescriptorPool( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkPipeline > : public TDispatchableHandleBase< VkPipeline > {
        bool Recreate( VkDevice                            InLogicalDeviceHandle,
                       VkPipelineCache                     pCache,
                       VkGraphicsPipelineCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateGraphicsPipelines( InLogicalDeviceHandle, pCache, 1, &CreateInfo, *this, *this ) );
        }

        bool Recreate( VkDevice InLogicalDeviceHandle, VkPipelineCache pCache, VkComputePipelineCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateComputePipelines( InLogicalDeviceHandle, pCache, 1, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkShaderModule > : public TDispatchableHandleBase< VkShaderModule > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkShaderModuleCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateShaderModule( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkSemaphore > : public TDispatchableHandleBase< VkSemaphore > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSemaphoreCreateInfo const &CreateInfo ) {
            apemode_assert( InLogicalDeviceHandle != VK_NULL_HANDLE, "Device is required." );

            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateSemaphore( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };

    template <>
    struct TDispatchableHandle< VkSampler > : public TDispatchableHandleBase< VkSampler > {
        bool Recreate( VkDevice InLogicalDeviceHandle, VkSamplerCreateInfo const &CreateInfo ) {
            Deleter( Handle );
            Deleter.hLogicalDevice = InLogicalDeviceHandle;
            return VK_SUCCESS == CheckedCall( vkCreateSampler( InLogicalDeviceHandle, &CreateInfo, *this, *this ) );
        }
    };
}
