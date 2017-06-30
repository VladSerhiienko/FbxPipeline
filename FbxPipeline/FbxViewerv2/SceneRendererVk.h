#pragma once

#include <ISceneRenderer.h>

namespace apemode {
    class SceneRendererVk : public ISceneRenderer {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {};
        struct SceneRenderParametersVk : SceneRenderParametersBase {};

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}