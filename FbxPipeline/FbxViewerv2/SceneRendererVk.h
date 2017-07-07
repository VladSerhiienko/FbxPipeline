#pragma once

#include <SceneRendererBase.h>
#include <GraphicsDevice.Vulkan.h>

namespace apemodevk {
    struct QueueFamilyPool {
        VkQueueFamilyProperties QueueFamilyProperties;

        template < typename TScheduleCallbackFn >
        void Schedule( TScheduleCallbackFn scheduleCallback ) {
        }
    };

    struct QueuePool {
        std::vector< QueueFamilyPool > Pools;

    };

}

namespace apemode {
    class SceneRendererVk : public SceneRendererBase {
    public:
        struct SceneUpdateParametersVk : SceneUpdateParametersBase {};
        struct SceneRenderParametersVk : SceneRenderParametersBase {};

        void UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) override;
        void RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) override;
    };
}