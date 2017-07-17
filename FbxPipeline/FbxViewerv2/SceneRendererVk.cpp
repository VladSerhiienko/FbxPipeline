#include <fbxvpch.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <SceneRendererVk.h>
#include <Scene.h>
#include <ArrayUtils.h>

namespace apemodevk {
    struct SceneDeviceAssetVk {
        GraphicsDevice* pNode = nullptr;
    };

    struct SceneMeshDeviceAssetVk {
        GraphicsDevice*                       pNode = nullptr;
        TDispatchableHandle< VkBufferView >   hVertexBuffer;
        TDispatchableHandle< VkBufferView >   hIndexBuffer;
        TDispatchableHandle< VkBuffer >       hBuffer;
        TDispatchableHandle< VkDeviceMemory > hMemory;
    };
}

void apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParamsBase ) {
    if ( nullptr == pScene ) {
        return;
    }

    bool deviceChanged = false;
    auto pParams       = (SceneUpdateParametersVk*) pParamsBase;

    auto deviceAsset = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset;
    if ( nullptr == pScene->deviceAsset ) {
        deviceAsset = new apemodevk::SceneDeviceAssetVk( );
        pScene->deviceAsset = deviceAsset;
        deviceChanged |= true;
    }

    if ( deviceAsset->pNode != pParams->pNode ) {
        deviceAsset->pNode = pParams->pNode;
        deviceChanged |= true;
    }

    if ( deviceChanged ) {

        /* Get queue from pool (only copying) */
        auto pQueuePool = pParams->pNode->GetQueuePool( );
        auto acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );

        /* Get command buffer from pool (only copying) */
        auto pCmdBufferPool = pParams->pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, acquiredQueue.QueueFamilyId );

        apemodevk::HostBufferPool bufferPool;
        bufferPool.Recreate( *pParams->pNode,
                             *pParams->pNode,
                             &pParams->pNode->AdapterProps.limits,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             false );

        uint32_t meshIndex = 0;
        auto & meshesFb = *pParamsBase->pSceneSrc->meshes( );

        if ( VK_SUCCESS != vkResetCommandPool( *pParams->pNode, acquiredCmdBuffer.pCmdPool, 0 ) ) {
            DebugBreak( );
        }

        VkCommandBufferBeginInfo commandBufferBeginInfo;
        apemodevk::InitializeStruct( commandBufferBeginInfo );
        commandBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if ( VK_SUCCESS != vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &commandBufferBeginInfo ) ) {
            DebugBreak( );
        }

        for ( auto meshFb : meshesFb ) {
            /* Scene mesh. */
            auto& mesh = pScene->meshes[ meshIndex ];

            /* Create mesh device asset if needed. */
            auto meshDeviceAsset = (apemodevk::SceneMeshDeviceAssetVk*) mesh.deviceAsset;
            if ( nullptr == meshDeviceAsset ) {
                meshDeviceAsset  = new apemodevk::SceneMeshDeviceAssetVk( );
                meshDeviceAsset->pNode = pParams->pNode;
                mesh.deviceAsset = meshDeviceAsset;
            }

            const uint32_t verticesByteSize = (uint32_t) meshFb->vertices( )->size( );
            const uint32_t indicesByteSize  = (uint32_t) meshFb->indices( )->size( );

            const uint32_t storageAlignment = (uint32_t) deviceAsset->pNode->AdapterProps.limits.minStorageBufferOffsetAlignment;
            const uint32_t verticesStorageSize = apemodem::AlignedOffset( verticesByteSize, storageAlignment );
            const uint32_t totalMeshSize = verticesStorageSize + indicesByteSize;

            auto verticesSuballocResult = bufferPool.Suballocate( meshFb->vertices( )->Data( ), verticesByteSize );
            auto indicesSuballocResult = bufferPool.Suballocate( meshFb->indices( )->Data( ), indicesByteSize );

            static VkBufferUsageFlags eBufferUsage
                = VK_BUFFER_USAGE_TRANSFER_DST_BIT  /* Copy data from staging buffers */
                | VK_BUFFER_USAGE_INDEX_BUFFER_BIT  /* Use it as index buffer */
                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT /* Use it as vertex buffer */;

            VkBufferCreateInfo bufferCreateInfo;
            apemodevk::InitializeStruct( bufferCreateInfo );
            bufferCreateInfo.usage = eBufferUsage;
            bufferCreateInfo.size  = meshFb->vertices( )->size( ) + meshFb->indices( )->size( );

            if ( false == meshDeviceAsset->hBuffer.Recreate( *pParams->pNode, *pParams->pNode, bufferCreateInfo ) ) {
                DebugBreak( );
            }

            auto memoryAllocateInfo = meshDeviceAsset->hBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            if ( false == meshDeviceAsset->hMemory.Recreate( *pParams->pNode, memoryAllocateInfo ) ) {
                DebugBreak( );
            }

            if ( false == meshDeviceAsset->hBuffer.BindMemory( meshDeviceAsset->hMemory, 0 ) ) {
                DebugBreak( );
            }

            VkBufferMemoryBarrier bufferMemoryBarrier[ 3 ];
            apemodevk::InitializeStruct( bufferMemoryBarrier );

            bufferMemoryBarrier[ 0 ].size                = verticesSuballocResult.descBufferInfo.range;
            bufferMemoryBarrier[ 0 ].offset              = verticesSuballocResult.descBufferInfo.offset;
            bufferMemoryBarrier[ 0 ].buffer              = verticesSuballocResult.descBufferInfo.buffer;
            bufferMemoryBarrier[ 0 ].srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
            bufferMemoryBarrier[ 0 ].dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            bufferMemoryBarrier[ 0 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier[ 0 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            bufferMemoryBarrier[ 1 ].size                = indicesSuballocResult.descBufferInfo.range;
            bufferMemoryBarrier[ 1 ].offset              = indicesSuballocResult.descBufferInfo.offset;
            bufferMemoryBarrier[ 1 ].buffer              = indicesSuballocResult.descBufferInfo.buffer;
            bufferMemoryBarrier[ 1 ].srcAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
            bufferMemoryBarrier[ 1 ].dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
            bufferMemoryBarrier[ 1 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier[ 1 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            bufferMemoryBarrier[ 2 ].size                = bufferCreateInfo.size;
            bufferMemoryBarrier[ 2 ].offset              = 0;
            bufferMemoryBarrier[ 2 ].buffer              = meshDeviceAsset->hBuffer;
            bufferMemoryBarrier[ 2 ].srcAccessMask       = VK_ACCESS_SHADER_READ_BIT;
            bufferMemoryBarrier[ 2 ].dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
            bufferMemoryBarrier[ 2 ].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            bufferMemoryBarrier[ 2 ].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                                  VK_PIPELINE_STAGE_HOST_BIT,
                                  VK_PIPELINE_STAGE_HOST_BIT,
                                  0,
                                  0,
                                  nullptr,
                                  apemode::GetArraySize( bufferMemoryBarrier ),
                                  bufferMemoryBarrier,
                                  0,
                                  nullptr );

            VkBufferCopy bufferCopy[ 2 ];
            apemodevk::InitializeStruct( bufferCopy );

            bufferCopy[ 0 ].srcOffset = verticesSuballocResult.descBufferInfo.offset;
            bufferCopy[ 0 ].size      = verticesSuballocResult.descBufferInfo.range;

            bufferCopy[ 1 ].srcOffset = indicesSuballocResult.descBufferInfo.offset;
            bufferCopy[ 1 ].dstOffset = verticesStorageSize;
            bufferCopy[ 1 ].size      = indicesSuballocResult.descBufferInfo.range;

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                 /* Cmd */
                             verticesSuballocResult.descBufferInfo.buffer, /* Src */
                             meshDeviceAsset->hBuffer,                     /* Dst */
                             1,
                             &bufferCopy[ 0 ] );

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                /* Cmd */
                             indicesSuballocResult.descBufferInfo.buffer, /* Src */
                             meshDeviceAsset->hBuffer,                    /* Dst */
                             1,
                             &bufferCopy[ 1 ] );

            ++meshIndex;
        }

        vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

        VkSubmitInfo submitInfo;
        apemodevk::InitializeStruct( submitInfo );
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;

        vkResetFences( *pParams->pNode, 1, &acquiredQueue.pFence );
        vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );
        vkWaitForFences( *pParams->pNode, 1, &acquiredQueue.pFence, true, UINT_MAX );

        acquiredCmdBuffer.pFence = acquiredQueue.pFence;
        pCmdBufferPool->Release( acquiredCmdBuffer );
        pQueuePool->Release( acquiredQueue );
    }
}

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParams ) {
    if ( nullptr == pScene ) {
        return;
    }
}
