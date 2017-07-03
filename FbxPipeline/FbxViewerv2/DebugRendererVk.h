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
            VkDevice                              pDevice;
            TDispatchableHandle< VkBuffer >       hBuffer;
            TDispatchableHandle< VkDeviceMemory > hMemory;
            uint8_t *                             pMapped;
            uint32_t                              Alignment;
            uint32_t                              Range;
            uint32_t                              OffsetIndex;
            uint32_t                              OffsetCount;
            uint32_t                              RangeIndex;
            VkMappedMemoryRange                   Ranges[ kMaxOffsets ];
            bool                                  bHostCoherent;

            bool Recreate( VkDevice           pInLogicalDevice,
                           VkPhysicalDevice   pInPhysicalDevice,
                           uint32_t           alignment,
                           uint32_t           bufferRange,
                           VkBufferUsageFlags bufferUsageFlags,
                           bool               bInHostCoherent ) {
                pMapped = nullptr;
                pDevice       = pInLogicalDevice;
                bHostCoherent = bInHostCoherent;

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
                
                return Reset();
            }

            /* NOTE: Does not handle space requirements (aborts in debug mode only). */
            template < typename TDataStructure >
            uint32_t TPush( const TDataStructure &dataStructure ) {
                const uint32_t coveredOffsetCount   = sizeof( TDataStructure ) / Alignment + 1;
                const uint32_t availableOffsetCount = OffsetCount - OffsetIndex + 1;

                assert( nullptr != pMapped );
                assert( coveredOffsetCount <= availableOffsetCount );

                const uint32_t currentMappedOffset = OffsetIndex * Alignment;

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

            bool Reset( ) {
                if ( nullptr == pMapped ) {
                    RangeIndex  = 0;
                    OffsetIndex = 0;

                    pMapped = hMemory.Map( 0, Range, 0 );
                    if ( nullptr == pMapped ) {
                        DebugBreak( );
                        return false;
                    }
                }

                /* TODO: Create descriptor set. */
                return true;
            }

            /* NOTE: Called from pool. */
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
            VkDescriptorBufferInfo descBufferInfo;
            uint32_t               dynamicOffset;
        };

        VkDevice              pLogicalDevice    = VK_NULL_HANDLE;
        VkPhysicalDevice      pPhysicalDevice   = VK_NULL_HANDLE;
        VkDescriptorPool      pDescPool         = VK_NULL_HANDLE;
        VkBufferUsageFlags    eBufferUsageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bool                  bHostCoherent     = false;
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
                       bool                          bInHostCoherent ) {
            Destroy( );

            pLogicalDevice  = pInLogicalDevice;
            pPhysicalDevice = pInPhysicalDevice;
            pDescPool       = pInDescPool;
            bHostCoherent   = bInHostCoherent;

            Pages.reserve( 16 );

            if ( 0 != bufferUsageFlags )
                eBufferUsageFlags = bufferUsageFlags;

            /* Use defaults otherwise. */
            if ( nullptr != pInLimits ) {
                MaxPageRange = pInLimits->maxUniformBufferRange;
                MinAlignment = (uint32_t) pInLimits->minUniformBufferOffsetAlignment;

                /* Catch incorrectly initialized device limits. */
                assert( 0 != MaxPageRange );
                assert( 0 != MinAlignment );

                /* Ensure we fit into 256 items. */
                if ( ( MaxPageRange / MinAlignment ) > kMaxOffsets ) {
                    MaxPageRange = MinAlignment * kMaxOffsets;
                }
            }
        }

        Page *FindPage( uint32_t dataStructureSize ) {
            /* Ensure it is possible to allocate that space. */
            if ( dataStructureSize > MaxPageRange ) {
                DebugBreak( );
                return nullptr;
            }

            /* Calculate how many chunks the data covers. */
            const uint32_t coveredOffsetCount = dataStructureSize / MinAlignment + 1;

            /* Try to find an existing free page. */
            auto pageIt = std::find_if( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                const uint32_t availableOffsets = pPage->OffsetCount - pPage->OffsetIndex + 1;
                return availableOffsets >= coveredOffsetCount;
            } );

            /* Return free page. */
            if ( pageIt != Pages.end( ) ) {
                return *pageIt;
            }

            /* Allocate new page and return. */
            Pages.push_back( new Page( ) );
            Pages.back( )->Recreate( pLogicalDevice, pPhysicalDevice, MinAlignment, MaxPageRange, eBufferUsageFlags, bHostCoherent );
            return Pages.back( );
        }

        template < typename TDataStructure >
        SuballocResult TSuballocate( const TDataStructure &dataStructure ) {
            SuballocResult suballocResult;
            InitializeStruct( suballocResult.descBufferInfo );

            if ( auto pPage = FindPage( sizeof( TDataStructure ) ) ) {
                suballocResult.descBufferInfo.buffer = pPage->hBuffer;
                suballocResult.descBufferInfo.range  = pPage->Range;
                suballocResult.dynamicOffset         = pPage->TPush( dataStructure );
            }

            return suballocResult;
        }

        /* On command buffer submission */
        void Flush( ) {
            std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                pPage->Flush( );
            } );
        }

        void Reset( ) {
            std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                pPage->Reset( );
            } );
        }

        void Destroy( ) {
            std::for_each( Pages.begin( ), Pages.end( ), [&]( Page *pPage ) {
                assert( nullptr != pPage );
                delete pPage;
            } );

            Pages.clear( );
        }
    };

    struct DescriptorSetPool {
        VkDevice              pLogicalDevice;
        VkDescriptorPool      pDescPool;
        VkDescriptorSetLayout pLayout;
        std::vector< std::pair<VkBuffer, VkDescriptorSet> > Sets;

        bool Recreate( VkDevice pInLogicalDevice, VkDescriptorPool pInDescPool, VkDescriptorSetLayout pInLayout ) {
            pLogicalDevice = pInLogicalDevice;
            pDescPool      = pInDescPool;
            pLayout        = pInLayout;
            return true;
        }

        VkDescriptorSet GetDescSet( const VkDescriptorBufferInfo &descriptorBufferInfo ) {
            auto descSetIt = std::find_if( Sets.begin( ), Sets.end( ), [&]( const std::pair< VkBuffer, VkDescriptorSet > &pair ) {
                return pair.first == descriptorBufferInfo.buffer;
            } );

            if ( descSetIt != Sets.end( ) )
                return descSetIt->second;

            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            InitializeStruct( descriptorSetAllocateInfo );
            descriptorSetAllocateInfo.descriptorPool     = pDescPool;
            descriptorSetAllocateInfo.pSetLayouts        = &pLayout;
            descriptorSetAllocateInfo.descriptorSetCount = 1;

            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            if ( VK_SUCCESS != vkAllocateDescriptorSets( pLogicalDevice, &descriptorSetAllocateInfo, &descriptorSet ) ) {
                DebugBreak( );
                return nullptr;
            }

            Sets.emplace_back( descriptorBufferInfo.buffer, descriptorSet );

            VkWriteDescriptorSet writeDescriptorSet;
            InitializeStruct( writeDescriptorSet );
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            writeDescriptorSet.pBufferInfo     = &descriptorBufferInfo;
            writeDescriptorSet.dstSet          = descriptorSet;
            vkUpdateDescriptorSets( pLogicalDevice, 1, &writeDescriptorSet, 0, nullptr );

            return descriptorSet;
        }
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

#if 1
        apemodevk::UniformBufferPool                            BufferPools[kMaxFrameCount];
        apemodevk::DescriptorSetPool                            DescSetPools[kMaxFrameCount];
#else
        apemodevk::TDispatchableHandle< VkBuffer >              hUniformBuffers[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hUniformBufferMemory[ kMaxFrameCount ];
#endif


        bool RecreateResources( InitParametersVk *initParams );

        void Reset( uint32_t FrameIndex );
        bool Render( RenderParametersVk *renderParams );
        void Flush( uint32_t FrameIndex );
    };
} // namespace apemode
