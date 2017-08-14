#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {
            apemodevk::GraphicsDevice* pNode       = nullptr;        /* Required */
            float                      dims[ 2 ]   = {};             /* Required */
            float                      scale[ 2 ]  = {};             /* Required */
            uint32_t                   FrameIndex  = 0;              /* Required */
            VkCommandBuffer            pCmdBuffer  = VK_NULL_HANDLE; /* Required */
            apemodem::mat4             ViewMatrix;                   /* Required */
            apemodem::mat4             ProjMatrix;                   /* Required */
        };

        void Reset( const Scene* pScene, uint32_t FrameIndex ) override;
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
        void Flush( const Scene* pScene, uint32_t FrameIndex ) override;
    };
}