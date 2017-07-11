#include <fbxvpch.h>

#include <QueuePools.Vulkan.h>
#include <SceneRendererVk.h>
#include <Scene.h>

namespace apemodevk {
    struct SceneDeviceAssetVk {
        apemodevk::GraphicsDevice* pNode = nullptr;
    };
    struct SceneMeshDeviceAssetVk {
        apemodevk::GraphicsDevice*                       pNode = nullptr;
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

    if ( ( nullptr == pScene->deviceAsset && nullptr != pParams->pSceneSrc ) /* First call. */ ||
         ( nullptr != deviceAsset && pParams->pNode != deviceAsset->pNode ) /* Device lost / changed. */ ) {
        deviceChanged |= true;

        delete deviceAsset;
        deviceAsset = new apemodevk::SceneDeviceAssetVk();
        deviceAsset->pNode = pParams->pNode;

        auto pQueuePool    = pParams->pNode->GetQueuePool( );
        auto acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );

        auto pCmdBufferPool    = pParams->pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );

        auto meshesFb = pParamsBase->pSceneSrc->meshes( );
        for ( int32_t i = 0; i < pScene->meshes.size( ); ++i ) {
            auto meshFb = ( (const apemodefb::MeshFb*) meshesFb->Data( ) ) + i;

            /* Create device resources ... */
            /* Create host resources ... */
            /* Populate host resources ... */
            /* Barriers ... */
            /* Copy host resources to device resources ... */
            /* Barriers ... */

            /*
            mesh.vertexBufferHandle = bgfx::createVertexBuffer(
                bgfxUtils::makeReleasableCopy( meshFb->vertices( )->Data( ), meshFb->vertices( )->size( ) ),
                PackedVertex::vertexDecl );

            if ( meshFb->subsets( ) && meshFb->subsets( )->size( ) && meshFb->subset_indices( ) ) {
                mesh.indexBufferHandle = bgfx::createIndexBuffer(
                    bgfxUtils::makeReleasableCopy( meshFb->subset_indices( )->Data( ),
                                                   meshFb->subset_indices( )->size( ) ),
                    meshFb->subset_index_type( ) == apemodefb::EIndexTypeFb_UInt32 ? BGFX_BUFFER_INDEX32 : 0 );
            }
            */

        }

        /* Submit ... */

        pCmdBufferPool->Release( acquiredCmdBuffer );
        pQueuePool->Release( acquiredQueue );
    }
}

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
    if ( nullptr == pScene ) {
        return;
    }
}
