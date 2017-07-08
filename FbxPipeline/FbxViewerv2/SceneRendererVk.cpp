#include <fbxvpch.h>

#include <SceneRendererVk.h>
#include <Scene.h>

namespace apemodevk {
    struct SceneDeviceAssetVk {
        VkDevice         pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice pPhysicalDevice = VK_NULL_HANDLE;
    };
    struct SceneMeshDeviceAssetVk {
        VkDevice                                         pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice                                 pPhysicalDevice = VK_NULL_HANDLE;
        apemodevk::TDispatchableHandle< VkBuffer >       VertexBuffer;
        apemodevk::TDispatchableHandle< VkBuffer >       IndexBuffer;
        apemodevk::TDispatchableHandle< VkDeviceMemory > VertexBufferMemory;
        apemodevk::TDispatchableHandle< VkDeviceMemory > IndexBufferMemory;
    };
}

void apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParamsBase ) {
    if ( nullptr == pScene ) {
        return;
    }

    bool deviceChanged = false;
    auto pParams       = (SceneUpdateParametersVk*) pParamsBase;
    auto deviceAsset   = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset;

    /* First call. */
    if ( nullptr == pScene->deviceAsset && nullptr != pParams->pSceneSrc ) {
        deviceChanged |= true;

        deviceAsset                  = new apemodevk::SceneDeviceAssetVk( );
        deviceAsset->pDevice         = pParams->pDevice;
        deviceAsset->pPhysicalDevice = pParams->pPhysicalDevice;
    }

    /* Device lost / changed. */
    if ( nullptr != deviceAsset && pParams->pDevice != deviceAsset->pDevice ) {
        deviceChanged |= true;

        delete deviceAsset;
        deviceAsset = new apemodevk::SceneDeviceAssetVk( );
        deviceAsset->pDevice = pParams->pDevice;
        deviceAsset->pPhysicalDevice = pParams->pPhysicalDevice;
    }

    auto meshesFb = pParamsBase->pSceneSrc->meshes( );
    for ( int32_t i = 0; i < pScene->meshes.size( ); ++i ) {
        auto meshFb = (const apemodefb::MeshFb*) meshesFb->Data( );
        

        /*mesh.vertexBufferHandle = bgfx::createVertexBuffer(
            bgfxUtils::makeReleasableCopy( meshFb->vertices( )->Data( ), meshFb->vertices( )->size( ) ),
            PackedVertex::vertexDecl );

        if ( meshFb->subsets( ) && meshFb->subsets( )->size( ) && meshFb->subset_indices( ) ) {
            mesh.indexBufferHandle = bgfx::createIndexBuffer(
                bgfxUtils::makeReleasableCopy( meshFb->subset_indices( )->Data( ),
                                                meshFb->subset_indices( )->size( ) ),
                meshFb->subset_index_type( ) == apemodefb::EIndexTypeFb_UInt32 ? BGFX_BUFFER_INDEX32 : 0 );
        }*/

    }
}

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
}
