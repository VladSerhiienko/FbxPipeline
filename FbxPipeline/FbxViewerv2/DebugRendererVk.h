#pragma once

#include <math.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>

namespace apemodevk {

    struct UniformBufferPool {
        static const uint32_t kMaxOffsets = 256;

        struct Page {
            VkDevice                                         pDevice;
            apemodevk::TDescriptorSets< 1 >                  DescSet;
            apemodevk::TDispatchableHandle< VkBuffer >       hBuffer;
            apemodevk::TDispatchableHandle< VkDeviceMemory > hMemory;
            uint8_t *                                        pMapped;
            uint32_t                                         Alignment;
            uint32_t                                         Range;
            uint32_t                                         OffsetIndex;
            uint32_t                                         OffsetCount;
            uint32_t                                         RangeIndex;
            VkMappedMemoryRange                              Ranges[ kMaxOffsets ];
            bool                                             bHostCoherent;

            bool Recreate( VkDevice           pInLogicalDevice,
                           VkPhysicalDevice   pInPhysicalDevice,
                           uint32_t           alignment,
                           uint32_t           bufferRange,
                           VkBufferUsageFlags bufferUsageFlags,
                           bool               bInHostCoherent ) {
                pDevice       = pInLogicalDevice;
                bHostCoherent = bInHostCoherent;

                if ( false == bHostCoherent ) {
                    RangeIndex = 0;
                }

                OffsetIndex = 0;
                OffsetCount = bufferRange / alignment;
                assert( OffsetCount <= kMaxOffsets );

                Range     = bufferRange;
                Alignment = alignment;

                VkBufferCreateInfo bufferCreateInfo;
                InitializeStruct( bufferCreateInfo );
                bufferCreateInfo.size  = bufferRange;
                bufferCreateInfo.usage = bufferUsageFlags;

                if ( false == hBuffer.Recreate( pInLogicalDevice, pInPhysicalDevice, bufferCreateInfo ) ) {
                    DebugBreak( );
                    return false;
                }

                auto memoryPropertyFlags
                    = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | ( bHostCoherent ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0 );

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

                /* TODO: Create descriptor set. */
                return true;
            }

            template < typename TDataStructure >
            uint32_t Push( const TDataStructure &dataStructure ) {
                assert( nullptr != pMapped );

                const uint32_t coveredOffsetCount = sizeof( TDataStructure ) / Alignment;
                const uint32_t availableOffsetCount = OffsetCount - OffsetIndex + 1;

                assert( coveredOffsetCount <= availableOffsetCount );

                if (coveredOffsetCount > availableOffsetCount)
                    /* Zero offset means suballocation failed */
                    return 0;

                /* Creation failed. */
                assert( nullptr != pMapped );

                const uint32_t currentMappedOffset = OffsetIndex * Alignment;

                /* Ensure the memory range related members are not accessed if the memory is host coherent. */
                if ( false == bHostCoherent ) {
                    /* Fill current range for flush. */
                    assert( RangeIndex < kMaxOffsets );
                    Ranges[ RangeIndex ].offset = currentMappedOffset;
                    Ranges[ RangeIndex ].size   = sizeof( TDataStructure );
                    ++RangeIndex;
                }

                /* Get current memory pointer and copy there. */
                auto mappedData = pMapped + currentMappedOffset;
                memcpy( mappedData, &dataStructure, sizeof( TDataStructure ) );
                OffsetIndex += coveredOffsetCount;

                return currentMappedOffset;
            }

            bool Flush( ) {
                if ( pMapped ) {
                    if ( true == bHostCoherent ) {
                        if ( VK_SUCCESS != vkFlushMappedMemoryRanges( pDevice, RangeIndex, Ranges ) ) {
                            DebugBreak( );
                            return false;
                        }
                    }

                    hMemory.Unmap( );
                    pMapped = nullptr;
                }

                return true;
            }
        };

        struct SuballocResult {
            VkDescriptorSet pDescSet;
            VkDeviceSize    Offset;
        };

        VkDevice              pLogicalDevice    = VK_NULL_HANDLE;
        VkPhysicalDevice      pPhysicalDevice   = VK_NULL_HANDLE;
        VkDescriptorPool      pDescPool         = VK_NULL_HANDLE;
        VkBufferUsageFlags    eBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        uint32_t              MinAlignment      = 256;
        uint32_t              MaxPageRange      = 65536; /* 256 * 256 */
        uint32_t              PageIndex         = 0;
        std::vector< Page * > Pages;

        /**
         * @param pInLogicalDevice Logical device.
         * @param pInPhysicalDevice Physical device.
         * @param pInDescPool Descriptor pool.
         * @param pInLimits Device limits (null is ok).
         * @param usageFlags Usually UNIFORM.
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

            Pages.reserve( 16 );

            if ( 0 != bufferUsageFlags )
                eBufferUsageFlags = bufferUsageFlags;

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
            std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                delete pPage;
            } );

            Pages.clear( );
        }

        Page *FindPage( size_t dataStructureSize ) {
            if ( dataStructureSize > MaxPageRange ) {
                DebugBreak( );
                return nullptr;
            }

            const uint32_t coveredOffsetCount = dataStructureSize / MinAlignment;
            auto pageIt = std::find_if( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                const uint32_t availableOffsets = pPage->OffsetCount - pPage->OffsetIndex + 1;
                return availableOffsets >= coveredOffsetCount;
            } );

            if ( pageIt != Pages.end( ) ) {
                return *pageIt;
            }

            Pages.push_back( new Page( ) );
            Pages.back( )->Recreate( pLogicalDevice, pPhysicalDevice, MinAlignment, MaxPageRange, eBufferUsageFlags, );
            return Pages.back( );
        }

        template < typename TDataStructure >
        SuballocResult TSuballocate( const TDataStructure &dataStructure ) {
            SuballocResult suballocResult;
            return suballocResult;
        }

        void Flush() {
            std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                pPage->Flush( );
            } );
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
