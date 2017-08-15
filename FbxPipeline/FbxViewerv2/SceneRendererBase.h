#pragma once
#include <stdint.h>

namespace apemodefb
{
    struct SceneFb;
}

namespace apemode {

    class Scene;

    /* Scene renderer will have a simple interface to allow the best flexibility for its implementations */
    class SceneRendererBase {
    public:

        /* Base structure that will contain common for all platforms scene update params */
        struct RecreateParametersBase {};

        /* Base update function that will perform common for all platforms scene update actions */
        virtual bool Recreate( const RecreateParametersBase* pParams ) {
            (void) pParams;
            return true;
        }

        /* Base structure that will contain common for all platforms scene update params */
        struct SceneUpdateParametersBase {
            const apemodefb::SceneFb* pSceneSrc = nullptr;
        };

        /* Base update function that will perform common for all platforms scene update actions */
        virtual bool UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) {
            (void) pScene;
            (void) pParams;
            return true;
        }

        /* Base structure that will contain common for all platforms scene render params */
        struct SceneRenderParametersBase {};

        /* Base render function that will perform platform independent preparations before rendering */
        virtual bool RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
            (void) pScene;
            (void) pParams;
            return true;
        }

        virtual bool Reset( const Scene* pScene, uint32_t FrameIndex ) {
            (void) pScene;
            (void) FrameIndex;
            return true;
        }

        virtual bool Flush( const Scene* pScene, uint32_t FrameIndex ) {
            (void) pScene;
            (void) FrameIndex;
            return true;
        }
    };

} // namespace apemode