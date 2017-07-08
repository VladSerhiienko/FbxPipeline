#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            VkDevice         pDevice         = VK_NULL_HANDLE;
            VkPhysicalDevice pPhysicalDevice = VK_NULL_HANDLE;
        };

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {};
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}