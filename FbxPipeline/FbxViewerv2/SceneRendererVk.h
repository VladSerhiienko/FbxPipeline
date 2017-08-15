#pragma once

#include <SceneRendererBase.h>
#include <MathfuInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct RecreateParametersVk : RecreateParametersBase {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        bool Recreate( const RecreateParametersBase* pParams ) override;
        bool UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {
            apemodevk::GraphicsDevice* pNode       = nullptr;        /* Required */
            float                      dims[ 2 ]   = {};             /* Required */
            float                      scale[ 2 ]  = {};             /* Required */
            uint32_t                   FrameIndex  = 0;              /* Required */
            VkCommandBuffer            pCmdBuffer  = VK_NULL_HANDLE; /* Required */
            apemodem::mat4             ViewMatrix;                   /* Required */
            apemodem::mat4             ProjMatrix;                   /* Required */
        };

        bool Reset( const Scene* pScene, uint32_t FrameIndex ) override;
        bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
        bool Flush( const Scene* pScene, uint32_t FrameIndex ) override;

        static uint32_t const kMaxFrameCount = 3;

        apemodevk::GraphicsDevice*                              pNode = nullptr;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDescriptorSets< kMaxFrameCount >            DescSets;
        apemodevk::HostBufferPool                               BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                            DescSetPools[ kMaxFrameCount ];
    };
}