#pragma once

#include <math.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>

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
            float           dims[ 2 ]  = {};             /* Required */
            float           scale[ 2 ] = {};             /* Required */
            VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
            uint32_t        FrameIndex = 0;              /* Required */
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
