#include <fbxvpch.h>

#include <QueuePools.Vulkan.h>
#include <BufferPools.Vulkan.h>
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

    struct SceneDeviceAssetVk {
        GraphicsDevice*                                         pNode = nullptr;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;

        static uint32_t const kMaxFrameCount = 3;

        apemodevk::TDescriptorSets< kMaxFrameCount > DescSets;
        apemodevk::HostBufferPool                    BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                 DescSetPools[ kMaxFrameCount ];



        struct RecreateResourcesParameters {
            GraphicsDevice*  pNode       = nullptr;
            VkDescriptorPool pDescPool   = VK_NULL_HANDLE;
            VkRenderPass     pRenderPass = VK_NULL_HANDLE;
            uint32_t         FrameCount  = 0;
        };

        bool RecreateResources( apemode::SceneRendererVk::SceneUpdateParametersVk* pParams ) {
            if ( nullptr == pParams->pNode )
                return false;

            const char* vertexShader =
                "#version 450\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "layout(std140, binding = 0) uniform FrameUniformBuffer {\n"
                "    mat4 worldMatrix;\n"
                "    mat4 viewMatrix;\n"
                "    mat4 projectionMatrix;\n"
                "    vec4 color;\n"
                "    vec4 positionOffset;\n"
                "    vec4 positionScale;\n"
                "} frameInfo;\n"
                "layout (location = 0) in vec3 inPosition;\n"
                "layout (location = 1) in vec3 inNormal;\n"
                "layout (location = 2) in vec4 inTangent;\n"
                "layout (location = 3) in vec2 inTexcoords;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main( ) {\n"
                "outColor    = frameInfo.color;\n"
                "gl_Position = frameInfo.projectionMatrix * frameInfo.viewMatrix * frameInfo.worldMatrix * \n"
                //"              vec4( inPosition.xyz, 1.0 );\n"
                "              vec4( inPosition.xyz * frameInfo.positionScale.xyz + frameInfo.positionOffset.xyz, 1.0 );\n"
                "}";

            const char* fragmentShader =
                "#version 450\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "layout (location = 0) in vec4 inColor;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "void main() {\n"
                "    outColor = inColor;\n"
                "}\n";

            shaderc::Compiler compiler;

            shaderc::CompileOptions options;
            options.SetSourceLanguage( shaderc_source_language_glsl );
            options.SetOptimizationLevel( shaderc_optimization_level_size );
            options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );

            shaderc::PreprocessedSourceCompilationResult cube_preprocessed[] = {
                compiler.PreprocessGlsl( vertexShader, shaderc_glsl_vertex_shader, "scene.vert", options ),
                compiler.PreprocessGlsl( fragmentShader, shaderc_glsl_fragment_shader, "scene.frag", options )};

            if ( shaderc_compilation_status_success != cube_preprocessed[ 0 ].GetCompilationStatus( ) ||
                shaderc_compilation_status_success != cube_preprocessed[ 1 ].GetCompilationStatus( ) ) {
                OutputDebugStringA( cube_preprocessed[ 0 ].GetErrorMessage( ).c_str( ) );
                OutputDebugStringA( cube_preprocessed[ 1 ].GetErrorMessage( ).c_str( ) );
                DebugBreak( );
                return false;
            }

#if 1
            shaderc::AssemblyCompilationResult cube_compiled_assembly[] = {
                compiler.CompileGlslToSpvAssembly(cube_preprocessed[0].begin(), shaderc_glsl_vertex_shader, "nuklear.vert.spv", options),
                compiler.CompileGlslToSpvAssembly(cube_preprocessed[1].begin(), shaderc_glsl_fragment_shader, "nuklear.frag.spv", options) };

            OutputDebugStringA("-------------------------------------------\n");
            OutputDebugStringA(cube_compiled_assembly[0].begin());
            OutputDebugStringA("-------------------------------------------\n");
            OutputDebugStringA(cube_compiled_assembly[1].begin());
            OutputDebugStringA("-------------------------------------------\n");

            if (shaderc_compilation_status_success != cube_compiled_assembly[0].GetCompilationStatus() ||
                shaderc_compilation_status_success != cube_compiled_assembly[1].GetCompilationStatus()) {

                OutputDebugStringA(cube_compiled_assembly[0].GetErrorMessage().c_str());
                OutputDebugStringA(cube_compiled_assembly[1].GetErrorMessage().c_str());

                DebugBreak();
                return false;
            }
#endif

            shaderc::SpvCompilationResult cube_compiled[] = {
                compiler.CompileGlslToSpv( cube_preprocessed[ 0 ].begin( ), shaderc_glsl_default_vertex_shader, "nuklear.vert.spv", options ),
                compiler.CompileGlslToSpv( cube_preprocessed[ 1 ].begin( ), shaderc_glsl_default_fragment_shader, "nuklear.frag.spv", options )};

            if ( shaderc_compilation_status_success != cube_compiled[ 0 ].GetCompilationStatus( ) ||
                shaderc_compilation_status_success != cube_compiled[ 1 ].GetCompilationStatus( ) ) {
                DebugBreak( );
                return false;
            }

            VkShaderModuleCreateInfo vertexShaderCreateInfo;
            InitializeStruct( vertexShaderCreateInfo );
            vertexShaderCreateInfo.pCode    = (const uint32_t*)       cube_compiled[ 0 ].begin( );
            vertexShaderCreateInfo.codeSize = (size_t) std::distance( cube_compiled[ 0 ].begin( ), cube_compiled[ 0 ].end( ) ) * sizeof( uint32_t );

            VkShaderModuleCreateInfo fragmentShaderCreateInfo;
            InitializeStruct( fragmentShaderCreateInfo );
            fragmentShaderCreateInfo.pCode    = (const uint32_t*)       cube_compiled[ 1 ].begin( );
            fragmentShaderCreateInfo.codeSize = (size_t) std::distance( cube_compiled[ 1 ].begin( ), cube_compiled[ 1 ].end( ) ) * sizeof( uint32_t );

            TDispatchableHandle< VkShaderModule > hVertexShaderModule;
            TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
            if ( false == hVertexShaderModule.Recreate( *pParams->pNode, vertexShaderCreateInfo ) ||
                false == hFragmentShaderModule.Recreate( *pParams->pNode, fragmentShaderCreateInfo ) ) {
                DebugBreak( );
                return false;
            }


            VkDescriptorSetLayoutBinding bindings[ 1 ];
            InitializeStruct( bindings );

            bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
            bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            bindings[ 0 ].descriptorCount = 1;

            VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
            InitializeStruct( descSetLayoutCreateInfo );

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
            InitializeStruct( pipelineLayoutCreateInfo );
            pipelineLayoutCreateInfo.setLayoutCount = GetArraySizeU( descriptorSetLayouts );
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

            InitializeStruct( graphicsPipelineCreateInfo );
            InitializeStruct( pipelineCacheCreateInfo );
            InitializeStruct( vertexInputStateCreateInfo );
            InitializeStruct( vertexInputAttributeDescription );
            InitializeStruct( vertexInputBindingDescription );
            InitializeStruct( inputAssemblyStateCreateInfo );
            InitializeStruct( rasterizationStateCreateInfo );
            InitializeStruct( colorBlendStateCreateInfo );
            InitializeStruct( colorBlendAttachmentState );
            InitializeStruct( depthStencilStateCreateInfo );
            InitializeStruct( viewportStateCreateInfo );
            InitializeStruct( multisampleStateCreateInfo );
            InitializeStruct( dynamicStateCreateInfo );
            InitializeStruct( shaderStageCreateInfo );

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

            graphicsPipelineCreateInfo.stageCount = GetArraySizeU( shaderStageCreateInfo );
            graphicsPipelineCreateInfo.pStages    = shaderStageCreateInfo;

            //
#if 1

            vertexInputBindingDescription[ 0 ].stride    = sizeof( PackedVertex );
            vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            vertexInputAttributeDescription[ 0 ].location = 0;
            vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
            vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->position );

            vertexInputAttributeDescription[ 1 ].location = 1;
            vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
            vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->normal );

            vertexInputAttributeDescription[ 2 ].location = 2;
            vertexInputAttributeDescription[ 2 ].binding  = vertexInputBindingDescription[ 0 ].binding;
            vertexInputAttributeDescription[ 2 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            vertexInputAttributeDescription[ 2 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->tangent );

            vertexInputAttributeDescription[ 3 ].location = 3;
            vertexInputAttributeDescription[ 3 ].binding  = vertexInputBindingDescription[ 0 ].binding;
            vertexInputAttributeDescription[ 3 ].format   = VK_FORMAT_R16G16_UNORM;
            vertexInputAttributeDescription[ 3 ].offset   = ( size_t )( &( (PackedVertex*) 0 )->texcoords );

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

            vertexInputStateCreateInfo.vertexBindingDescriptionCount   = GetArraySizeU( vertexInputBindingDescription );
            vertexInputStateCreateInfo.pVertexBindingDescriptions      = vertexInputBindingDescription;
            vertexInputStateCreateInfo.vertexAttributeDescriptionCount = GetArraySizeU( vertexInputAttributeDescription );
            vertexInputStateCreateInfo.pVertexAttributeDescriptions    = vertexInputAttributeDescription;
            graphicsPipelineCreateInfo.pVertexInputState               = &vertexInputStateCreateInfo;

            //

            inputAssemblyStateCreateInfo.topology          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;

            //

            dynamicStateEnables[ 0 ]                 = VK_DYNAMIC_STATE_SCISSOR;
            dynamicStateEnables[ 1 ]                 = VK_DYNAMIC_STATE_VIEWPORT;
            dynamicStateCreateInfo.pDynamicStates    = dynamicStateEnables;
            dynamicStateCreateInfo.dynamicStateCount = GetArraySizeU( dynamicStateEnables );
            graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

            //

            rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
            rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_NONE;
            //rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
            rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizationStateCreateInfo.depthClampEnable        = VK_FALSE;
            rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
            rasterizationStateCreateInfo.depthBiasEnable         = VK_FALSE;
            rasterizationStateCreateInfo.lineWidth               = 1.0f;
            graphicsPipelineCreateInfo.pRasterizationState       = &rasterizationStateCreateInfo;

            //

            colorBlendAttachmentState[ 0 ].colorWriteMask = 0xf;
            colorBlendAttachmentState[ 0 ].blendEnable    = VK_FALSE;
            colorBlendStateCreateInfo.attachmentCount     = GetArraySizeU( colorBlendAttachmentState );
            colorBlendStateCreateInfo.pAttachments        = colorBlendAttachmentState;
            graphicsPipelineCreateInfo.pColorBlendState   = &colorBlendStateCreateInfo;

            //

            depthStencilStateCreateInfo.depthTestEnable       = VK_TRUE;
            depthStencilStateCreateInfo.depthWriteEnable      = VK_TRUE;
            depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
            depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
            depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
            depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
            depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_ALWAYS;
            depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
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
    };

    struct SceneMeshDeviceAssetVk {
        SceneDeviceAssetVk*                   pSceneDeviceAsset = nullptr;
        TDispatchableHandle< VkBuffer >       hBuffer;
        TDispatchableHandle< VkDeviceMemory > hMemory;
        uint32_t                              VertexCount = 0;
        uint32_t                              IndexOffset = 0;
        VkIndexType                           IndexType   = VK_INDEX_TYPE_UINT16;
        apemodem::vec4                        positionOffset;
        apemodem::vec4                        positionScale;
    };
}

void apemode::SceneRendererVk::UpdateScene( Scene* pScene, const SceneUpdateParametersBase* pParamsBase ) {
    if ( nullptr == pScene ) {
        return;
    }

    bool deviceChanged = false;
    auto pParams       = (SceneUpdateParametersVk*) pParamsBase;

    auto pDeviceAsset = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset;
    if ( nullptr == pScene->deviceAsset ) {
        pDeviceAsset = new apemodevk::SceneDeviceAssetVk( );
        pScene->deviceAsset = pDeviceAsset;
        deviceChanged |= true;
    }

    if ( pDeviceAsset->pNode != pParams->pNode ) {
        pDeviceAsset->pNode = pParams->pNode;
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
            auto& mesh = pScene->meshes[ meshIndex++ ];

            /* Create mesh device asset if needed. */
            auto pMeshDeviceAsset = (apemodevk::SceneMeshDeviceAssetVk*) mesh.deviceAsset;
            if ( nullptr == pMeshDeviceAsset ) {
                pMeshDeviceAsset  = new apemodevk::SceneMeshDeviceAssetVk( );
                pMeshDeviceAsset->pSceneDeviceAsset = pDeviceAsset;
                mesh.deviceAsset = pMeshDeviceAsset;
            }

            const uint32_t verticesByteSize = (uint32_t) meshFb->vertices( )->size( );
            const uint32_t indicesByteSize  = (uint32_t) meshFb->indices( )->size( );

            const uint32_t storageAlignment = (uint32_t) pDeviceAsset->pNode->AdapterProps.limits.minStorageBufferOffsetAlignment;
            const uint32_t verticesStorageSize = apemodem::AlignedOffset( verticesByteSize, storageAlignment );
            const uint32_t totalMeshSize = verticesStorageSize + indicesByteSize;

            pMeshDeviceAsset->IndexOffset = verticesStorageSize;
            if ( meshFb->index_type( ) == apemodefb::EIndexTypeFb_UInt32 ) {
                pMeshDeviceAsset->IndexType = VK_INDEX_TYPE_UINT32;
            }

            auto offset = meshFb->submeshes( )->begin( )->position_offset( );
            pMeshDeviceAsset->positionOffset.x = offset.x();
            pMeshDeviceAsset->positionOffset.y = offset.y();
            pMeshDeviceAsset->positionOffset.z = offset.z();

            auto scale = meshFb->submeshes( )->begin( )->position_scale( );
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

    if ( pDeviceAsset->hPipeline.IsNull( ) && nullptr != pParams && nullptr != pParams->pRenderPass && nullptr != pParams->pDescPool ) {
        if ( false == pDeviceAsset->RecreateResources( pParams ) ) {
            return;
        }
    }
}

// -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45p.fbxp" -p
// -i "F:\Dev\AutodeskMaya\Mercedes+Benz+A45+AMG+Centered.FBX" -o "$(SolutionDir)assets\A45.fbxp"
// -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4p.fbxp" -p
// -i "E:\Media\Models\mech-m-6k\source\93d43cf18ad5406ba0176c9fae7d4927.fbx" -o "$(SolutionDir)assets\Mech6kv4.fbxp"
// -i "E:\Media\Models\carambit\source\Knife.fbx" -o "$(SolutionDir)assets\Knifep.fbxp" -p
// -i "E:\Media\Models\pontiac-firebird-formula-1974\source\carz.obj 2.zip\carz.obj\mesh.obj" -o "$(SolutionDir)assets\pontiacp.fbxp" -p

void apemode::SceneRendererVk::RenderScene( const Scene* pScene, const SceneRenderParametersBase* pParamsBase ) {
    if ( nullptr == pScene || nullptr == pParamsBase ) {
        return;
    }

    auto pParams = (const SceneRenderParametersVk*) pParamsBase;
    auto pDeviceAsset = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset; /* TODO: const_cast */

    /* Device change was not handled, cannot render the scene. */
    if ( nullptr == pScene->deviceAsset || pDeviceAsset->pNode != pParams->pNode ) {
        return;
    }

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

    auto FrameIndex = ( pParams->FrameIndex ) % apemodevk::SceneDeviceAssetVk::kMaxFrameCount;

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

                auto suballocResult = pDeviceAsset->BufferPools[ FrameIndex ].TSuballocate( frameData );
                assert( VK_NULL_HANDLE != suballocResult.descBufferInfo.buffer );
                suballocResult.descBufferInfo.range = sizeof( apemodevk::FrameUniformBuffer );
                VkDescriptorSet descriptorSet[ 1 ]  = {pDeviceAsset->DescSetPools[ FrameIndex ].GetDescSet(
                    suballocResult.descBufferInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC )};

                vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pDeviceAsset->hPipeline );
                vkCmdBindDescriptorSets( pParams->pCmdBuffer,
                                         VK_PIPELINE_BIND_POINT_GRAPHICS,
                                         pDeviceAsset->hPipelineLayout,
                                         0,
                                         1,
                                         descriptorSet,
                                         1,
                                         &suballocResult.dynamicOffset );

                VkBuffer     vertexBuffers[ 1 ] = {pMeshDeviceAsset->hBuffer};
                VkDeviceSize vertexOffsets[ 1 ] = {0};
                vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

#if 0
                vkCmdDraw( pParams->pCmdBuffer, pMeshDeviceAsset->VertexCount, 1, 0, 0 );
#elif 0
                vkCmdDraw( pParams->pCmdBuffer, subset.indexCount, 1, subset.baseIndex, 0 );
#else

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
#endif
            }
        }
    }
}

void apemode::SceneRendererVk::Reset( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        if ( auto pDeviceAsset = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset ) { /* TODO: const_cast */
            pDeviceAsset->BufferPools[ FrameIndex ].Reset( );
        }
    }
}

void apemode::SceneRendererVk::Flush( const Scene* pScene, uint32_t FrameIndex ) {
    if ( nullptr != pScene ) {
        if ( auto pDeviceAsset = (apemodevk::SceneDeviceAssetVk*) pScene->deviceAsset ) { /* TODO: const_cast */
            pDeviceAsset->BufferPools[ FrameIndex ].Flush( );
        }
    }
}
