#pragma once

#include <math.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>

namespace apemodevk {

    template < uint32_t TMaxPages = 16 >
    struct BufferPool {
        static const uint32_t kMaxOffsets = 256;

        struct Page {
            apemodevk::TDescriptorSets< 1 >                  DescSet;
            apemodevk::TDispatchableHandle< VkBuffer >       hBuffer;
            apemodevk::TDispatchableHandle< VkDeviceMemory > hMemory;
            uint8_t *                                        pMapped;
            uint32_t                                         Alignment;
            uint32_t                                         Range;
            uint32_t                                         OffsetIndex;
            uint32_t                                         RangeIndex;
            uint32_t                                         OffsetCount;
            VkMappedMemoryRange                              Ranges[ kMaxOffsets ];

            bool Recreate( VkDevice              pInLogicalDevice,
                           VkPhysicalDevice      pInPhysicalDevice,
                           uint32_t              alignment,
                           uint32_t              bufferRange,
                           VkBufferUsageFlags    bufferUsageFlags,
                           VkMemoryPropertyFlags memoryPropertyFlags,
                           uint32_t              offsetCount ) {
                assert( offsetCount <= kMaxOffsets );

                RangeIndex  = 0;
                OffsetIndex = 0;
                OffsetCount = offsetCount;
                Range       = bufferRange;
                Alignment   = alignment;

                VkBufferCreateInfo bufferCreateInfo;
                InitializeStruct( bufferCreateInfo );
                bufferCreateInfo.size  = bufferRange;
                bufferCreateInfo.usage = bufferUsageFlags;

                if ( false == hBuffer.Recreate( pInLogicalDevice, pInPhysicalDevice, bufferCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                auto memoryAllocateInfo = hBuffer.GetMemoryAllocateInfo( memoryPropertyFlags );
                if ( false == hMemory.Recreate( pInLogicalDevice, memoryAllocateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                if ( false == hBuffer.BindMemory( hMemory, 0 ) ) {
                    DebugBreak( );
                    return false;
                }

                pMapped = hMemory.Map( 0, bufferRange, 0 );
                if ( nullptr == pMapped ) {
                    DebugBreak( );
                    return false;
                }

                return true;
            }

            template < typename TDataStructure >
            uint32_t Push( const TDataStructure &dataStructure ) {
                assert( nullptr != pMapped );

                const uint32_t coveredOffsetCount = sizeof( TDataStructure ) / Alignment;
                const uint32_t availableOffsetCount = OffsetCount - OffsetIndex + 1;

                /* Cannot fit all the data in this page. */
                if (coveredOffsetCount > availableOffsetCount)
                    /* Zero offset means suballocation failed */
                    return 0;

                /* Creation failed. */
                assert( nullptr != pMapped );

                const uint32_t currentMappedOffset = OffsetIndex * Alignment;

                /* Fill current range for flush. */
                assert( RangeIndex < kMaxOffsets );
                Ranges[ RangeIndex ].offset = currentMappedOffset;
                Ranges[ RangeIndex ].size   = sizeof( TDataStructure );
                ++RangeIndex;

                /* Get current memory pointer and copy there. */
                auto mappedData = pMapped + currentMappedOffset;
                memcpy( mappedData, &dataStructure, sizeof( TDataStructure ) );
                OffsetIndex += coveredOffsetCount;

                return currentMappedOffset;
            }

            void Flush( ) {
                if ( pMapped ) {
                    hMemory.Unmap( );
                    pMapped = nullptr;
                }

            }
        };

        struct SuballocResult {
            VkDescriptorSet pDescSet;
            VkDeviceSize    Offset;
        };

        VkDevice                pLogicalDevice  = VK_NULL_HANDLE;
        VkPhysicalDevice        pPhysicalDevice = VK_NULL_HANDLE;
        VkDescriptorPool        pDescPool       = VK_NULL_HANDLE;
        uint32_t                MinAlignment    = 256;
        uint32_t                MaxPageRange    = 65536; /* 256 * 256 */
        uint32_t                PageIndex       = 0;
        uint32_t                PageCount       = 0;
        std::unique_ptr< Page > Pages[ TMaxPages ];

        /**
         * @param pInLogicalDevice Logical device.
         * @param pInPhysicalDevice Physical device.
         * @param pInDescPool Descriptor pool.
         * @param pInLimits Device limits (null is ok).
         * @param usageFlags Usually VERTEX + FRAGMENT.
         */
        void Recreate( VkDevice                      pInLogicalDevice,
                       VkPhysicalDevice              pInPhysicalDevice,
                       VkDescriptorPool              pInDescPool,
                       const VkPhysicalDeviceLimits *pInLimits,
                       VkBufferUsageFlags            bufferUsageFlags,
                       VkMemoryPropertyFlags         memoryPropertyFlags ) {
            Destroy( );

            pLogicalDevice  = pInLogicalDevice;
            pPhysicalDevice = pInPhysicalDevice;
            pDescPool       = pInDescPool;

            /* Use defaults otherwise. */
            if ( nullptr != pInLimits ) {
                MaxPageRange = pInLimits->maxUniformBufferRange;
                MinAlignment = pInLimits->minUniformBufferOffsetAlignment;

                /* Catch incorrectly initialized device limits. */
                assert( 0 != MaxPageRange );
                assert( 0 != MinAlignment );

                /* Ensure we fit into 256 items. */
                if ( ( MaxPageRange / MinAlignment ) > kMaxOffsets ) {
                    MaxPageRange = MinAlignment * kMaxOffsets;
                }
            }
        }

        void Destroy() {
            Pages.clear( );
        }

        template < typename TDataStructure >
        SuballocResult TSuballocate( const TDataStructure &dataStructure ) {

            SuballocResult suballocResult;
            return suballocResult;
        }

        void Flush() {

        }
    };

    template <typename TData>
    struct TUniformBufferHandle {
        TData Data;
    };

}

namespace apemode {

    struct DebugRendererVk {
        struct PositionVertex {
            float pos[ 3 ];
        };

        struct FrameUniformBuffer {
            mathfu::mat4 worldMatrix;
            mathfu::mat4 viewMatrix;
            mathfu::mat4 projectionMatrix;
            mathfu::vec4 color;
        };

        struct InitParametersVk {
            VkAllocationCallbacks *pAlloc          = nullptr;        /* Null is ok */
            VkDevice               pDevice         = VK_NULL_HANDLE; /* Required */
            VkPhysicalDevice       pPhysicalDevice = VK_NULL_HANDLE; /* Required */
            VkDescriptorPool       pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass           pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t               FrameCount      = 0;              /* Required, swapchain img count typically */
        };

        struct RenderParametersVk {
            float               dims[ 2 ]  = {};             /* Required */
            float               scale[ 2 ] = {};             /* Required */
            VkCommandBuffer     pCmdBuffer = VK_NULL_HANDLE; /* Required */
            uint32_t            FrameIndex = 0;              /* Required */
            FrameUniformBuffer *pFrameData = nullptr;        /* Required */
        };

        static uint32_t const kMaxFrameCount = 3;

        VkDevice pDevice;
        apemodevk::TDescriptorSets< kMaxFrameCount >            DescSets;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDispatchableHandle< VkBuffer >              hVertexBuffer;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hVertexBufferMemory;
        apemodevk::TDispatchableHandle< VkBuffer >              hUniformBuffers[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hUniformBufferMemory[ kMaxFrameCount ];

        bool RecreateResources( InitParametersVk *initParams );
        bool Render( RenderParametersVk *renderParams );
    };
} // namespace apemode
