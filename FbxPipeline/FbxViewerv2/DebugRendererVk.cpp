#include "DebugRendererVk.h"

#include <TInfoStruct.Vulkan.h>
#include <shaderc/shaderc.hpp>

namespace apemode {
    using namespace apemodevk;
}

bool apemode::DebugRendererVk::RecreateResources( InitParametersVk* initParams ) {
    if ( nullptr == initParams )
        return false;

    const char* vertexShader =
        "#version 450\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "layout(std140, binding = 0) uniform FrameUniformBuffer {\n"
        "    mat4 worldMatrix;\n"
        "    mat4 viewMatrix;\n"
        "    mat4 projectionMatrix;\n"
        "    vec4 color;\n"
        "} frameInfo;\n"
        "layout (location = 0) in vec3 inPosition;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "void main( ) {\n"
        "outColor    = frameInfo.color;\n"
        "gl_Position = frameInfo.projectionMatrix * frameInfo.viewMatrix * frameInfo.worldMatrix * vec4( inPosition, 1.0 );\n"
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
        compiler.PreprocessGlsl( vertexShader, shaderc_glsl_vertex_shader, "cube.vert", options ),
        compiler.PreprocessGlsl( fragmentShader, shaderc_glsl_fragment_shader, "cube.frag", options )};

    if ( shaderc_compilation_status_success != cube_preprocessed[ 0 ].GetCompilationStatus( ) ||
        shaderc_compilation_status_success != cube_preprocessed[ 1 ].GetCompilationStatus( ) ) {
        OutputDebugStringA( cube_preprocessed[ 0 ].GetErrorMessage( ).c_str( ) );
        OutputDebugStringA( cube_preprocessed[ 1 ].GetErrorMessage( ).c_str( ) );
        DebugBreak( );
        return false;
    }

#if 0

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
        return;
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
    if ( false == hVertexShaderModule.Recreate( initParams->pDevice, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( initParams->pDevice, fragmentShaderCreateInfo ) ) {
        DebugBreak( );
        return false;
    }


    VkDescriptorSetLayoutBinding bindings[ 1 ];
    InitializeStruct( bindings );

#if 1
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
#else
    bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
#endif
    bindings[ 0 ].descriptorCount = 1;
    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = 1;
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( initParams->pDevice, descSetLayoutCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    if ( false == DescSets.RecreateResourcesFor( initParams->pDevice, initParams->pDescPool, descriptorSetLayouts ) ) {
        DebugBreak( );
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = GetArraySizeU( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( initParams->pDevice, pipelineLayoutCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 1 ];
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
    graphicsPipelineCreateInfo.renderPass = initParams->pRenderPass;

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

    vertexInputBindingDescription[ 0 ].stride    = sizeof( PositionVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (PositionVertex*) 0 )->pos );

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
    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
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

    if ( false == hPipelineCache.Recreate( initParams->pDevice, pipelineCacheCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    if ( false == hPipeline.Recreate( initParams->pDevice, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    if ( hVertexBuffer.IsNull( ) ) {

        // clang-format off
        const float g_vertex_buffer_data[] = {
            // -X side
            -1.0f,-1.0f,-1.0f,  
            -1.0f,-1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,

            // -Z side
            -1.0f,-1.0f,-1.0f,
             1.0f, 1.0f,-1.0f,
             1.0f,-1.0f,-1.0f,
            -1.0f,-1.0f,-1.0f,
            -1.0f, 1.0f,-1.0f,
             1.0f, 1.0f,-1.0f,

             // -Y side
             -1.0f,-1.0f,-1.0f,
             1.0f,-1.0f,-1.0f,
             1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f,-1.0f,
             1.0f,-1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,

            // +Y side
            -1.0f, 1.0f,-1.0f,
            -1.0f, 1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f,-1.0f,
             1.0f, 1.0f, 1.0f,
             1.0f, 1.0f,-1.0f,

             // +X side
             1.0f, 1.0f,-1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f, 1.0f,
            1.0f,-1.0f,-1.0f,
            1.0f, 1.0f,-1.0f,

            // +Z side
            -1.0f, 1.0f, 1.0f,  
            -1.0f,-1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
            -1.0f,-1.0f, 1.0f,
             1.0f,-1.0f, 1.0f,
             1.0f, 1.0f, 1.0f,
        };
        // clang-format on

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.size  = sizeof( g_vertex_buffer_data );

        if ( false == hVertexBuffer.Recreate( initParams->pDevice, initParams->pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return false;
        }

        auto memoryAllocateInfo = hVertexBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        if ( false == hVertexBufferMemory.Recreate( initParams->pDevice, memoryAllocateInfo ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == hVertexBuffer.BindMemory( hVertexBufferMemory, 0 ) ) {
            DebugBreak( );
            return false;
        }

        if ( auto mappedData = hVertexBufferMemory.Map( 0, bufferCreateInfo.size, 0 ) ) {
            memcpy( mappedData, g_vertex_buffer_data, sizeof( g_vertex_buffer_data ) );

            VkMappedMemoryRange range;
            InitializeStruct( range );
            range.memory = hVertexBufferMemory;
            range.size   = VK_WHOLE_SIZE;

            if ( ResultHandle::Failed( vkFlushMappedMemoryRanges( initParams->pDevice, 1, &range ) ) ) {
                DebugBreak( );
                return false;
            }

            hVertexBufferMemory.Unmap();
        }
    }

#if 1
    for (uint32_t i = 0; i < initParams->FrameCount; ++i) {
        BufferPools[i].Recreate(initParams->pDevice, initParams->pPhysicalDevice, initParams->pDescPool, nullptr, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false);
        DescSetPools[i].Recreate(initParams->pDevice, initParams->pDescPool, hDescSetLayout);
    }
#else
    for ( uint32_t i = 0; i < initParams->FrameCount; ++i ) {
        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferCreateInfo.size  = sizeof( FrameUniformBuffer );

        if ( false == hUniformBuffers[ i ].Recreate( initParams->pDevice, initParams->pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return false;
        }

        auto memoryAllocateInfo = hUniformBuffers[ i ].GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

        if ( false == hUniformBufferMemory[ i ].Recreate( initParams->pDevice, memoryAllocateInfo ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == hUniformBuffers[ i ].BindMemory( hUniformBufferMemory[ i ], 0 ) ) {
            DebugBreak( );
            return false;
        }
    }

    for ( uint32_t i = 0; i < initParams->FrameCount; ++i ) {
        VkDescriptorBufferInfo descriptorBufferInfo;
        InitializeStruct( descriptorBufferInfo );
        descriptorBufferInfo.buffer = hUniformBuffers[ i ];
        descriptorBufferInfo.range  = sizeof( FrameUniformBuffer );

        VkWriteDescriptorSet writeDescriptorSet;
        InitializeStruct( writeDescriptorSet );
        writeDescriptorSet.dstSet          = DescSets.hSets[ i ];
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo     = &descriptorBufferInfo;
        vkUpdateDescriptorSets( initParams->pDevice, 1, &writeDescriptorSet, 0, nullptr );
    }
#endif

    pDevice = initParams->pDevice;
    return true;
}

void apemode::DebugRendererVk::Reset( uint32_t FrameIndex ) {
    BufferPools[ FrameIndex ].Reset( );
}

bool apemode::DebugRendererVk::Render( RenderParametersVk* renderParams ) {
    auto FrameIndex = ( renderParams->FrameIndex ) % kMaxFrameCount;

#if 1

    auto suballocResult = BufferPools[ FrameIndex ].TSuballocate( *renderParams->pFrameData );
    assert( VK_NULL_HANDLE != suballocResult.descBufferInfo.buffer );
    suballocResult.descBufferInfo.range = sizeof(FrameUniformBuffer);
    VkDescriptorSet descriptorSet[1] = { DescSetPools[FrameIndex].GetDescSet(suballocResult.descBufferInfo) };

    vkCmdBindPipeline( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
    vkCmdBindDescriptorSets( renderParams->pCmdBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             hPipelineLayout,
                             0,
                             1,
                             descriptorSet,
                             1,
                             &suballocResult.dynamicOffset );

#else
    if ( auto mappedData = hUniformBufferMemory[ FrameIndex ].Map( 0, sizeof( FrameUniformBuffer ), 0 ) ) {
        memcpy( mappedData, renderParams->pFrameData, sizeof( FrameUniformBuffer ) );

        /**
         * Note: if propertyFlags has the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit set,
         *       host cache management commands vkFlushMappedMemoryRanges and vkInvalidateMappedMemoryRanges
         *       are not needed to make host writes visible to the device or device writes visible to the host,
         *       respectively.
         * See: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch10s02.html
         */

        hUniformBufferMemory[ FrameIndex ].Unmap( );
    } else {
        DebugBreak( );
        return false;
    }

    VkDescriptorSet descSets[ 1 ] = {DescSets.hSets[ FrameIndex ]};
    vkCmdBindPipeline( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
    vkCmdBindDescriptorSets( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipelineLayout, 0, 1, descSets, 0, NULL );
#endif

    VkBuffer     vertexBuffers[ 1 ] = {hVertexBuffer};
    VkDeviceSize vertexOffsets[ 1 ] = {0};
    vkCmdBindVertexBuffers( renderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );

    VkViewport viewport;
    InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = renderParams->dims[ 0 ] * renderParams->scale[ 0 ];
    viewport.height   = renderParams->dims[ 1 ] * renderParams->scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( renderParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = renderParams->dims[ 0 ] * renderParams->scale[ 0 ];
    scissor.extent.height = renderParams->dims[ 1 ] * renderParams->scale[ 1 ];

    vkCmdSetScissor( renderParams->pCmdBuffer, 0, 1, &scissor );
    vkCmdDraw( renderParams->pCmdBuffer, 12 * 3, 1, 0, 0 );

    return true;
}

void apemode::DebugRendererVk::Flush( uint32_t FrameIndex ) {
    BufferPools[ FrameIndex ].Flush( );
}
