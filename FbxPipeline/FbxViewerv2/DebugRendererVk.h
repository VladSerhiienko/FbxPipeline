#pragma once

#include <MathfuInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>

namespace apemode {

    struct DebugRendererVk {
        struct PositionVertex {
            float pos[ 3 ];
        };

        struct FrameUniformBuffer {
            apemodem::mat4 worldMatrix;
            apemodem::mat4 viewMatrix;
            apemodem::mat4 projectionMatrix;
            apemodem::vec4 color;
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
        apemodevk::DescriptorSetPool                            DescSetPools[ kMaxFrameCount ];
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
