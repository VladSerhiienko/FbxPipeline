#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {
            apemodevk::GraphicsDevice* pNode = nullptr;
        };

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;

        struct SceneRenderParametersVk : SceneRenderParametersBase {};
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}