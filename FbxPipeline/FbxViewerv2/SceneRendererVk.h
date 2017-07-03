#pragma once

#include <SceneRendererBase.h>

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {};
        struct SceneRenderParametersVk : SceneRenderParametersBase {};

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}