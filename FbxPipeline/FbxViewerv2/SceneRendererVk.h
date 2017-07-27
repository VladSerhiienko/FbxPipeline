#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            apemodevk::GraphicsDevice* pNode       = nullptr;
            VkDescriptorPool           pDescPool   = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount  = 0;              /* Required */
        };

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {
            apemodevk::GraphicsDevice* pNode       = nullptr;
            float                      dims[ 2 ]   = {};             /* Required */
            float                      scale[ 2 ]  = {};             /* Required */
            VkCommandBuffer            pCmdBuffer  = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameIndex  = 0;              /* Required */
        };

        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}