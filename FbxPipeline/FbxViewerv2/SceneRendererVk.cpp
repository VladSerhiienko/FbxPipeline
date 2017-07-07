#include <fbxvpch.h>

#include <SceneRendererVk.h>
#include <Scene.h>

namespace apemodevk {
    struct SceneDeviceAsset {
    };
    struct SceneMeshDeviceAsset {
        apemodevk::TDispatchableHandle<VkBuffer> VertexBuffer, IndexBuffer;
        apemodevk::TDispatchableHandle<VkDeviceMemory> VertexBufferMemory, IndexBufferMemory;
    };
}

void apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParams ) {

    if ( nullptr == pScene ) {
        return;
    }

    // TODO: Recreate scene resources
    if ( nullptr == pScene->deviceAsset && nullptr != pParams->pSceneSrc ) {
        pScene->deviceAsset = new apemodevk::SceneDeviceAsset( );
    }
     


}

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
}
