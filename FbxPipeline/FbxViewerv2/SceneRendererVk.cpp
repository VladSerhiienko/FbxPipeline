#include <fbxvpch.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>

#include <SceneRendererVk.h>
#include <Scene.h>
#include <ArrayUtils.h>
#include <shaderc/shaderc.hpp>

namespace apemodevk {

    struct StaticVertex {
        float position[ 3 ];
        float normal[ 3 ];
        float tangent[ 4 ];
        float texcoords[ 2 ];
        /*apemodem::vec3 position;
        apemodem::vec3 normal;
        apemodem::vec4 tangent;
        apemodem::vec2 texcoords;*/
    };

    struct PackedVertex {
        uint32_t position;
        uint32_t normal;
        uint32_t tangent;
        uint32_t texcoords;
    };

    struct FrameUniformBuffer {
        apemodem::mat4 worldMatrix;
        apemodem::mat4 viewMatrix;
        apemodem::mat4 projectionMatrix;
        apemodem::vec4 color;
        apemodem::vec4 positionOffset;
        apemodem::vec4 positionScale;
    };

    struct SceneMeshDeviceAssetVk {
        TDispatchableHandle< VkBuffer >       hBuffer;
        TDispatchableHandle< VkDeviceMemory > hMemory;
        uint32_t                              VertexCount = 0;
        uint32_t                              IndexOffset = 0;
        VkIndexType                           IndexType   = VK_INDEX_TYPE_UINT16;
        apemodem::vec4                        positionOffset;
        apemodem::vec4                        positionScale;
    };
}

bool apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParamsBase ) {
    if ( nullptr == pScene ) {
        return false;
    }

    bool deviceChanged = false;
    auto pParams = (SceneUpdateParametersVk*) pParamsBase;

    if ( pNode != pParams->pNode ) {
        pNode = pParams->pNode;
        deviceChanged |= true;
    }

    if ( deviceChanged ) {

        /* Get queue from pool (only copying) */
        auto pQueuePool = pParams->pNode->GetQueuePool( );

        auto acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );
        while ( acquiredQueue.pQueue == nullptr ) {
            acquiredQueue = pQueuePool->Acquire( false, VK_QUEUE_TRANSFER_BIT, false );
        }

        /* Get command buffer from pool (only copying) */
        auto pCmdBufferPool = pParams->pNode->GetCommandBufferPool( );
        auto acquiredCmdBuffer = pCmdBufferPool->Acquire( false, acquiredQueue.queueFamilyId );

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
            auto& mesh = pScene->meshes[ meshIndex++ ];

            /* Create mesh device asset if needed. */
            auto pMeshDeviceAsset = (apemodevk::SceneMeshDeviceAssetVk*) mesh.deviceAsset;
            if ( nullptr == pMeshDeviceAsset ) {
                pMeshDeviceAsset  = new apemodevk::SceneMeshDeviceAssetVk( );
                mesh.deviceAsset = pMeshDeviceAsset;
            }

            const uint32_t verticesByteSize = (uint32_t) meshFb->vertices( )->size( );
            const uint32_t indicesByteSize  = (uint32_t) meshFb->indices( )->size( );

            const uint32_t storageAlignment = (uint32_t) pNode->AdapterProps.limits.minStorageBufferOffsetAlignment;
            const uint32_t verticesStorageSize = apemodem::AlignedOffset( verticesByteSize, storageAlignment );
            const uint32_t totalMeshSize = verticesStorageSize + indicesByteSize;

            pMeshDeviceAsset->IndexOffset = verticesStorageSize;
            if ( meshFb->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ) {
                pMeshDeviceAsset->IndexType = VK_INDEX_TYPE_UINT32;
            }

            auto & offset = meshFb->submeshes( )->begin( )->position_offset( );
            pMeshDeviceAsset->positionOffset.x = offset.x();
            pMeshDeviceAsset->positionOffset.y = offset.y();
            pMeshDeviceAsset->positionOffset.z = offset.z();

            auto & scale = meshFb->submeshes( )->begin( )->position_scale( );
            pMeshDeviceAsset->positionScale.x = scale.x();
            pMeshDeviceAsset->positionScale.y = scale.y();
            pMeshDeviceAsset->positionScale.z = scale.z();

            pMeshDeviceAsset->VertexCount = meshFb->submeshes( )->begin( )->vertex_count( );


            auto verticesSuballocResult = bufferPool.Suballocate( meshFb->vertices( )->Data( ), verticesByteSize );
            auto indicesSuballocResult = bufferPool.Suballocate( meshFb->indices( )->Data( ), indicesByteSize );

            static VkBufferUsageFlags eBufferUsage
                = VK_BUFFER_USAGE_TRANSFER_DST_BIT  /* Copy data from staging buffers */
                | VK_BUFFER_USAGE_INDEX_BUFFER_BIT  /* Use it as index buffer */
                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT /* Use it as vertex buffer */;

            VkBufferCreateInfo bufferCreateInfo;
            apemodevk::InitializeStruct( bufferCreateInfo );
            bufferCreateInfo.usage = eBufferUsage;
            bufferCreateInfo.size  = totalMeshSize;

            if ( false == pMeshDeviceAsset->hBuffer.Recreate( *pParams->pNode, *pParams->pNode, bufferCreateInfo ) ) {
                DebugBreak( );
            }

            auto memoryAllocateInfo = pMeshDeviceAsset->hBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
            if ( false == pMeshDeviceAsset->hMemory.Recreate( *pParams->pNode, memoryAllocateInfo ) ) {
                DebugBreak( );
            }

            if ( false == pMeshDeviceAsset->hBuffer.BindMemory( pMeshDeviceAsset->hMemory, 0 ) ) {
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
            bufferMemoryBarrier[ 2 ].buffer              = pMeshDeviceAsset->hBuffer;
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
                             pMeshDeviceAsset->hBuffer,                    /* Dst */
                             1,
                             &bufferCopy[ 0 ] );

            vkCmdCopyBuffer( acquiredCmdBuffer.pCmdBuffer,                /* Cmd */
                             indicesSuballocResult.descBufferInfo.buffer, /* Src */
                             pMeshDeviceAsset->hBuffer,                   /* Dst */
                             1,
                             &bufferCopy[ 1 ] );
        }

        vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

        VkSubmitInfo submitInfo;
        apemodevk::InitializeStruct( submitInfo );
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;

        bufferPool.Flush( );

        vkResetFences( *pParams->pNode, 1, &acquiredQueue.pFence );
        vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence );
        vkWaitForFences( *pParams->pNode, 1, &acquiredQueue.pFence, true, UINT_MAX );

        acquiredCmdBuffer.pFence = acquiredQueue.pFence;
        pCmdBufferPool->Release( acquiredCmdBuffer );
        pQueuePool->Release( acquiredQueue );
    }

    return true;
}

bool apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {
    if ( nullptr == pScene || nullptr == pParamsBase ) {
        return false;
    }

    auto pParams = (const SceneRenderParametersVk*) pParamsBase;

    /* Device change was not handled, cannot render the scene. */
    if ( pNode != pParams->pNode ) {
        return false;
    }

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );

    VkViewport viewport;
    apemodevk::InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pParams->dims[ 0 ] * pParams->scale[ 0 ];
    viewport.height   = pParams->dims[ 1 ] * pParams->scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( pParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    apemodevk::InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = ( uint32_t )( pParams->dims[ 0 ] * pParams->scale[ 0 ] );
    scissor.extent.height = ( uint32_t )( pParams->dims[ 1 ] * pParams->scale[ 1 ] );

    vkCmdSetScissor(pParams->pCmdBuffer, 0, 1, &scissor);

    auto FrameIndex = ( pParams->FrameIndex ) % kMaxFrameCount;

    apemodevk::FrameUniformBuffer frameData;
    frameData.projectionMatrix = pParams->ProjMatrix;
    frameData.viewMatrix       = pParams->ViewMatrix;

    for ( auto& node : pScene->nodes ) {
        if ( node.meshId >= pScene->meshes.size( ) )
            continue;

        auto& mesh = pScene->meshes[ node.meshId ];

        if ( auto pMeshDeviceAsset = (const apemodevk::SceneMeshDeviceAssetVk*) mesh.deviceAsset ) {
            for (auto& subset : mesh.subsets) {
                auto& mat = pScene->materials[ node.materialIds[ subset.materialId ] ];

                frameData.color          = mat.albedo;
                frameData.positionOffset = pMeshDeviceAsset->positionOffset;
                frameData.positionScale  = pMeshDeviceAsset->positionScale;
                frameData.worldMatrix    = pScene->worldMatrices[ node.id ];

                auto suballocResult = BufferPools[ FrameIndex ].TSuballocate( frameData );
                assert( VK_NULL_HANDLE != suballocResult.descBufferInfo.buffer );
                suballocResult.descBufferInfo.range = sizeof( apemodevk::FrameUniformBuffer );

                VkDescriptorSet descriptorSet[ 1 ]  = {nullptr};

                apemodevk::TDescriptorSet< 1 > descSet;
                descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                descSet.pBinding[ 0 ].BufferInfo      = suballocResult.descBufferInfo;

                descriptorSet[ 0 ]  = DescSetPools[ FrameIndex ].GetDescSet( &descSet );

                vkCmdBindDescriptorSets( pParams->pCmdBuffer,
                                         VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         hPipelineLayout,
                                         0,
                                         1,
                                         descriptorSet,
                                         1,
                                         &suballocResult.dynamicOffset );

                VkBuffer     vertexBuffers[ 1 ] = {pMeshDeviceAsset->hBuffer};
                VkDeviceSize vertexOffsets[ 1 ] = {0};
                vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

                vkCmdBindIndexBuffer( pParams->pCmdBuffer,
                                      pMeshDeviceAsset->hBuffer,
                                      pMeshDeviceAsset->IndexOffset,
                                      pMeshDeviceAsset->IndexType );

                vkCmdDrawIndexed( pParams->pCmdBuffer,
                                  subset.indexCount, /* IndexCount */
                                  1,                 /* InstanceCount */
                                  subset.baseIndex,  /* FirstIndex */
                                  0,                 /* VertexOffset */
                                  0 );               /* FirstInstance */
            }
        }
    }

    return true;
}

bool apemode::SceneRendererVk::Recreate(const RecreateParametersBase * pParamsBase)
{
    auto pParams = (RecreateParametersVk*) pParamsBase;

    if ( nullptr == pParams->pNode )
        return false;

    std::set<std::string> includedFiles;
    std::vector<uint8_t> compiledShaders[2];

    if ( false == pParams->pShaderCompiler->Compile( "shaders/apemode/Scene.vert",
                                                     {},
                                                     apemodevk::ShaderCompiler::eShaderType_GLSL_VertexShader,
                                                     includedFiles,
                                                     compiledShaders[ 0 ] ) ||
         false == pParams->pShaderCompiler->Compile( "shaders/apemode/Scene.frag",
                                                     {},
                                                     apemodevk::ShaderCompiler::eShaderType_GLSL_FragmentShader,
                                                     includedFiles,
                                                     compiledShaders[ 1 ] ) ) {
        DebugBreak( );
        return false;
    }

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    apemodevk::InitializeStruct( vertexShaderCreateInfo );
    vertexShaderCreateInfo.pCode    = (const uint32_t*) compiledShaders[ 0 ].data( );
    vertexShaderCreateInfo.codeSize = compiledShaders[ 0 ].size( );

    VkShaderModuleCreateInfo fragmentShaderCreateInfo;
    apemodevk::InitializeStruct( fragmentShaderCreateInfo );
    fragmentShaderCreateInfo.pCode    = (const uint32_t*) compiledShaders[ 1 ].data( );
    fragmentShaderCreateInfo.codeSize = compiledShaders[ 1 ].size( );

    apemodevk::TDispatchableHandle< VkShaderModule > hVertexShaderModule;
    apemodevk::TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( *pParams->pNode, vertexShaderCreateInfo ) ||
        false == hFragmentShaderModule.Recreate( *pParams->pNode, fragmentShaderCreateInfo ) ) {
        DebugBreak( );
        return false;
    }


    VkDescriptorSetLayoutBinding bindings[ 1 ];
    apemodevk::InitializeStruct( bindings );

    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[ 0 ].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    apemodevk::InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = 1;
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( *pParams->pNode, descSetLayoutCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    if ( false == DescSets.RecreateResourcesFor( *pParams->pNode, pParams->pDescPool, descriptorSetLayouts ) ) {
        DebugBreak( );
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    apemodevk::InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = apemodevk::GetArraySizeU( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( *pParams->pNode, pipelineLayoutCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 4 ];
    VkVertexInputBindingDescription        vertexInputBindingDescription[ 1 ];
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    colorBlendStateCreateInfo;
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState[ 1 ];
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo;
    VkPipelineViewportStateCreateInfo      viewportStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   multisampleStateCreateInfo;
    VkDynamicState                         dynamicStateEnables[ 2 ];
    VkPipelineDynamicStateCreateInfo       dynamicStateCreateInfo ;
    VkPipelineShaderStageCreateInfo        shaderStageCreateInfo[ 2 ];

    apemodevk::InitializeStruct( graphicsPipelineCreateInfo );
    apemodevk::InitializeStruct( pipelineCacheCreateInfo );
    apemodevk::InitializeStruct( vertexInputStateCreateInfo );
    apemodevk::InitializeStruct( vertexInputAttributeDescription );
    apemodevk::InitializeStruct( vertexInputBindingDescription );
    apemodevk::InitializeStruct( inputAssemblyStateCreateInfo );
    apemodevk::InitializeStruct( rasterizationStateCreateInfo );
    apemodevk::InitializeStruct( colorBlendStateCreateInfo );
    apemodevk::InitializeStruct( colorBlendAttachmentState );
    apemodevk::InitializeStruct( depthStencilStateCreateInfo );
    apemodevk::InitializeStruct( viewportStateCreateInfo );
    apemodevk::InitializeStruct( multisampleStateCreateInfo );
    apemodevk::InitializeStruct( dynamicStateCreateInfo );
    apemodevk::InitializeStruct( shaderStageCreateInfo );

    //

    graphicsPipelineCreateInfo.layout     = hPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = pParams->pRenderPass;

    //

    shaderStageCreateInfo[ 0 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[ 0 ].module = hVertexShaderModule;
    shaderStageCreateInfo[ 0 ].pName  = "main";

    shaderStageCreateInfo[ 1 ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[ 1 ].module = hFragmentShaderModule;
    shaderStageCreateInfo[ 1 ].pName  = "main";

    graphicsPipelineCreateInfo.stageCount = apemodevk::GetArraySizeU( shaderStageCreateInfo );
    graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

    //
#if 1

    vertexInputBindingDescription[ 0 ].stride    = sizeof( apemodevk::PackedVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R16G16_UNORM;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (apemodevk::PackedVertex*) 0 )->texcoords );

#else

    vertexInputBindingDescription[ 0 ].stride    = sizeof( StaticVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->normal );

    vertexInputAttributeDescription[ 2 ].location = 2;
    vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->tangent );

    vertexInputAttributeDescription[ 3 ].location = 3;
    vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (StaticVertex*) 0 )->texcoords );

#endif

    vertexInputStateCreateInfo.vertexBindingDescriptionCount   = apemodevk::GetArraySizeU( vertexInputBindingDescription );
    vertexInputStateCreateInfo.pVertexBindingDescriptions      = vertexInputBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = apemodevk::GetArraySizeU( vertexInputAttributeDescription );
    vertexInputStateCreateInfo.pVertexAttributeDescriptions    = vertexInputAttributeDescription;
    graphicsPipelineCreateInfo.pVertexInputState               = &vertexInputStateCreateInfo;

    //

    inputAssemblyStateCreateInfo.topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

    //

    dynamicStateEnables[ 0 ]                 = VK_DYNAMIC_STATE_SCISSOR;
    dynamicStateEnables[ 1 ]                 = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
    dynamicStateCreateInfo.dynamicStateCount = apemodevk::GetArraySizeU( dynamicStateEnables );
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    //

    // rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_NONE;
    // rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE; /* CW */
    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
    rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
    rasterizationStateCreateInfo.lineWidth               = 1.0f;
    graphicsPipelineCreateInfo.pRasterizationState       = &rasterizationStateCreateInfo;

    //

    colorBlendAttachmentState[ 0 ].colorWriteMask = 0xf;
    colorBlendAttachmentState[ 0 ].blendEnable    = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount     = apemodevk::GetArraySizeU( colorBlendAttachmentState );
    colorBlendStateCreateInfo.pAttachments        = colorBlendAttachmentState;
    graphicsPipelineCreateInfo.pColorBlendState   = &colorBlendStateCreateInfo;

    //

    depthStencilStateCreateInfo.depthTestEnable       = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_ALWAYS;
    depthStencilStateCreateInfo.front                 = depthStencilStateCreateInfo.back;
    graphicsPipelineCreateInfo.pDepthStencilState     = &depthStencilStateCreateInfo;

    //

    viewportStateCreateInfo.scissorCount      = 1;
    viewportStateCreateInfo.viewportCount     = 1;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;

    //

    multisampleStateCreateInfo.pSampleMask          = NULL;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    graphicsPipelineCreateInfo.pMultisampleState    = &multisampleStateCreateInfo;

    //

    if ( false == hPipelineCache.Recreate( *pParams->pNode, pipelineCacheCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    if ( false == hPipeline.Recreate( *pParams->pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    VkPhysicalDeviceProperties adapterProps;
    vkGetPhysicalDeviceProperties( *pParams->pNode, &adapterProps );

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        BufferPools[ i ].Recreate( *pParams->pNode, *pParams->pNode, &adapterProps.limits, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
        DescSetPools[ i ].Recreate( *pParams->pNode, pParams->pDescPool, hDescSetLayout );
    }

    return true;
}

bool apemode::SceneRendererVk::Reset( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        BufferPools[ FrameIndex ].Reset( );
    }

    return true;
}

bool apemode::SceneRendererVk::Flush( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        BufferPools[ FrameIndex ].Flush( );
    }

    return true;
}
