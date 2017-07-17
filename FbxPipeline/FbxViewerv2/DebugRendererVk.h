#pragma once

#include <math.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>

namespace apemodevk {

    struct DescriptorSetPool {
        VkDevice                                            pLogicalDevice;
        VkDescriptorPool                                    pDescPool;
        VkDescriptorSetLayout                               pLayout;
        std::vector< std::pair<VkBuffer, VkDescriptorSet> > Sets;

        bool Recreate( VkDevice pInLogicalDevice, VkDescriptorPool pInDescPool, VkDescriptorSetLayout pInLayout ) {
            pLogicalDevice = pInLogicalDevice;
            pDescPool      = pInDescPool;
            pLayout        = pInLayout;
            return true;
        }

        VkDescriptorSet GetDescSet( const VkDescriptorBufferInfo &descriptorBufferInfo ) {
            auto descSetIt = std::find_if( Sets.begin( ), Sets.end( ),
                [&]( const std::pair< VkBuffer, VkDescriptorSet > &pair ) {
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
        apemodevk::HostBufferPool                               BufferPools[ kMaxFrameCount ];
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
