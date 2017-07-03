#include <scene_generated.h>

#include <SceneRendererVk.h>
#include <Scene.h>

struct SceneDeviceAsset {
};
struct SceneMeshDeviceAsset {
};

void apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) {

    if ( nullptr == pScene ) {
        return;
    }

    // TODO: Recreate scene resources
    if ( nullptr == pScene->deviceAsset && nullptr != pParams->pSceneSrc ) {
        pScene->deviceAsset = new SceneDeviceAsset( );
    }
     


}

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
}
