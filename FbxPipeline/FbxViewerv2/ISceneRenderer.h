#pragma once

namespace apemode {

    class Scene;

    /* Scene renderer will have a simple interface to allow the best flexibility for its implementations */
    class ISceneRenderer {
    public:
        /* Base structure that will contain common for all platforms scene update params */
        struct SceneUpdateParametersBase {};

        /* Base update function that will perform common for all platforms scene update actions */
        virtual void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) {
            (void) pScene;
            (void) pParams;
        }

        /* Base structure that will contain common for all platforms scene render params */
        struct SceneRenderParametersBase {};

        /* Base render function that will perform platform independent preparations before rendering */
        virtual void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
            (void) pScene;
            (void) pParams;
        }
    };

} // namespace apemode