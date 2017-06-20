
#include <NuklearSdlVk.h>
#include <shaderc/shaderc.hpp>

namespace apemode {
    using namespace apemodevk;
}

bool apemode::NuklearSdlVk::Render( RenderParametersBase* p ) {
    auto renderParams = (RenderParametersVk*) p;
    auto FrameIndex   = ( renderParams->FrameIndex ) % kMaxFrameCount;

    if ( hVertexBuffer[ FrameIndex ].IsNull( ) || ( VertexBufferSize[ FrameIndex ] < p->max_vertex_buffer ) ) {
        hVertexBuffer[ FrameIndex ].Destroy( );
        hVertexBufferMemory[ FrameIndex ].Destroy( );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.size        = p->max_vertex_buffer;
        bufferCreateInfo.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ( false == hVertexBuffer[ FrameIndex ].Recreate( pDevice, pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return false;
        }

        auto memoryAllocateInfo = hVertexBuffer[ FrameIndex ].GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        if ( false == hVertexBufferMemory[ FrameIndex ].Recreate( pDevice, memoryAllocateInfo ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == hVertexBuffer[ FrameIndex ].BindMemory( hVertexBufferMemory[ FrameIndex ], 0 ) ) {
            DebugBreak( );
            return false;
        }

        VertexBufferSize[ FrameIndex ] = p->max_vertex_buffer;
    }

    if ( hIndexBuffer[ FrameIndex ].IsNull( ) || IndexBufferSize[ FrameIndex ] < p->max_element_buffer ) {
        hIndexBuffer[ FrameIndex ].Destroy( );
        hIndexBufferMemory[ FrameIndex ].Destroy( );

        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.size        = p->max_element_buffer;
        bufferCreateInfo.usage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ( false == hIndexBuffer[ FrameIndex ].Recreate( pDevice, pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return false;
        }

        auto memoryAllocateInfo = hIndexBuffer[ FrameIndex ].GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        if ( false == hIndexBufferMemory[ FrameIndex ].Recreate( pDevice, memoryAllocateInfo ) ) {
            DebugBreak( );
            return false;
        }

        if ( false == hIndexBuffer[ FrameIndex ].BindMemory( hIndexBufferMemory[ FrameIndex ], 0 ) ) {
            DebugBreak( );
            return false;
        }

        IndexBufferSize[ FrameIndex ] = p->max_element_buffer;
    }

    {
        /* Convert from command queue into draw list and draw to screen */
        void* vertices = nullptr;
        void* elements = nullptr;

        /* Load vertices/elements directly into vertex/element buffer */
        vertices = hVertexBufferMemory[ FrameIndex ].Map( 0, VertexBufferSize[ FrameIndex ], 0 );
        elements = hIndexBufferMemory[ FrameIndex ].Map( 0, IndexBufferSize[ FrameIndex ], 0 );
        {
            /* Fill convert configuration */
            struct nk_convert_config                          config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, pos )},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, uv )},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF( Vertex, col )},
                {NK_VERTEX_LAYOUT_END}};

            memset( &config, 0, sizeof( config ) );
            config.vertex_layout        = vertex_layout;
            config.vertex_size          = sizeof( Vertex );
            config.vertex_alignment     = NK_ALIGNOF( Vertex );
            config.null                 = NullTexture;
            config.circle_segment_count = 22;
            config.curve_segment_count  = 22;
            config.arc_segment_count    = 22;
            config.global_alpha         = 1.0f;
            config.shape_AA             = p->aa;
            config.line_AA              = p->aa;

            /* Setup buffers to load vertices and elements */
            nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed( &vbuf, vertices, (nk_size) p->max_vertex_buffer );
            nk_buffer_init_fixed( &ebuf, elements, (nk_size) p->max_element_buffer );
            nk_convert( &Context, &RenderCmds, &vbuf, &ebuf, &config );
        }

        VkMappedMemoryRange ranges[ 2 ];
        InitializeStruct( ranges );

        ranges[ 0 ].memory = hVertexBufferMemory[ FrameIndex ];
        ranges[ 0 ].size   = VK_WHOLE_SIZE;

        ranges[ 1 ].memory = hIndexBufferMemory[ FrameIndex ];
        ranges[ 1 ].size   = VK_WHOLE_SIZE;

        if ( ResultHandle::Failed( vkFlushMappedMemoryRanges( pDevice, GetArraySizeU( ranges ), ranges ) ) ) {
            DebugBreak( );
            return false;
        }

        hVertexBufferMemory[ FrameIndex ].Unmap( );
        hIndexBufferMemory[ FrameIndex ].Unmap( );
    }

    /* Bind pipeline and descriptor sets */
    {
        VkDescriptorSet descSets[ 1 ] = {DescSet.hSets[ 0 ]};
        vkCmdBindPipeline( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
        vkCmdBindDescriptorSets( renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipelineLayout, 0, 1, descSets, 0, NULL );
    }

    /* Bind Vertex And Index Buffer */
    {
        VkBuffer     vertexBuffers[ 1 ] = {hVertexBuffer[ FrameIndex ]};
        VkDeviceSize vertexOffsets[ 1 ] = {0};
        vkCmdBindVertexBuffers( renderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );
        vkCmdBindIndexBuffer( renderParams->pCmdBuffer, hIndexBuffer[ FrameIndex ], 0, VK_INDEX_TYPE_UINT16 );
    }

    VkViewport viewport;
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = p->dims[ 0 ] * p->scale[ 0 ];
    viewport.height   = p->dims[ 1 ] * p->scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( renderParams->pCmdBuffer, 0, 1, &viewport );

    /* Setup scale and translation */
    {
        float offsetScale[ 4 ] = {
            -1.0f,               /* Translation X */
            -1.0f,               /* Translation Y */
            2.0f / p->dims[ 0 ], /* Scaling X */
            2.0f / p->dims[ 1 ], /* Scaling Y */
        };

        vkCmdPushConstants(
            renderParams->pCmdBuffer, hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( float[ 4 ] ), offsetScale );
    }

    const nk_draw_command* cmd        = nullptr;
    uint32_t               offset     = 0;
    uint32_t               vtx_offset = 0;

    /* Iterate over and execute each draw command */
    nk_draw_foreach( cmd, &Context, &RenderCmds ) {
        if ( !cmd->elem_count )
            continue;

        VkRect2D scissor;

        scissor.offset.x      = (  int32_t )( cmd->clip_rect.x * p->scale[ 0 ] );
        scissor.offset.y      = (  int32_t )( cmd->clip_rect.y * p->scale[ 1 ] );
        scissor.extent.width  = ( uint32_t )( cmd->clip_rect.w * p->scale[ 0 ] );
        scissor.extent.height = ( uint32_t )( cmd->clip_rect.h * p->scale[ 1 ] );

        scissor.offset.x      = std::max< int32_t >( 0, scissor.offset.x );
        scissor.offset.y      = std::max< int32_t >( 0, scissor.offset.y );
        scissor.extent.width  = std::min< uint32_t >( (uint32_t) viewport.width, scissor.extent.width );
        scissor.extent.height = std::min< uint32_t >( (uint32_t) viewport.height, scissor.extent.height );

        vkCmdSetScissor( renderParams->pCmdBuffer, 0, 1, &scissor );
        vkCmdDrawIndexed( renderParams->pCmdBuffer, cmd->elem_count, 1, offset, vtx_offset, 0 );

        offset += cmd->elem_count;
    }

    nk_clear( &Context );
    return true;
}

void apemode::NuklearSdlVk::DeviceDestroy( ) {
}

bool apemode::NuklearSdlVk::DeviceCreate( InitParametersBase* init_params ) {
    const char* vertex_shader =
        "#version 450\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "layout(push_constant) uniform PushConstants { vec4 offsetScale; } pushConstants;\n"
        //"layout(push_constant) uniform PushConstants { mat4 modelViewProjection; } pushConstants;\n"
        "layout(location=0) in vec2 inPosition;\n"
        "layout(location=1) in vec2 inTexcoords;\n"
        "layout(location=2) in vec4 inColor;\n"
        "layout(location=0) out vec2 outTexcoords;\n"
        "layout(location=1) out vec4 outColor;\n"
        "void main() {\n"
        "   outTexcoords = inTexcoords;\n"
        "   outColor = inColor;\n"
        "   gl_Position = vec4(inPosition.xy * pushConstants.offsetScale.zw + pushConstants.offsetScale.xy, 0, 1);\n"
        //"   gl_Position = pushConstants.modelViewProjection * vec4(inPosition.xy, 0, 1);\n"
        "}\n";

    const char* fragment_shader =
        "#version 450\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "layout(set=0, binding=0) uniform sampler2D samplerFont;\n"
        "layout(location=0) in vec2 inTexcoords;\n"
        "layout(location=1) in vec4 inColor;\n"
        "layout(location=0) out vec4 outColor;\n"
        "void main(){\n"
        "   outColor = inColor * texture(samplerFont, inTexcoords.st);\n"
        "}\n";

    shaderc::Compiler compiler;

    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level_size );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );

    shaderc::PreprocessedSourceCompilationResult nuklear_preprocessed[] = {
        compiler.PreprocessGlsl( vertex_shader, shaderc_glsl_vertex_shader, "nuklear.vert", options ),
        compiler.PreprocessGlsl( fragment_shader, shaderc_glsl_fragment_shader, "nuklear.frag", options )};

    if ( shaderc_compilation_status_success != nuklear_preprocessed[ 0 ].GetCompilationStatus( ) ||
         shaderc_compilation_status_success != nuklear_preprocessed[ 1 ].GetCompilationStatus( ) ) {
        OutputDebugStringA( nuklear_preprocessed[ 0 ].GetErrorMessage( ).c_str( ) );
        OutputDebugStringA( nuklear_preprocessed[ 1 ].GetErrorMessage( ).c_str( ) );
        DebugBreak( );
        return false;
    }

#if 0

    shaderc::AssemblyCompilationResult nuklear_compiled_assembly[] = {
        compiler.CompileGlslToSpvAssembly(nuklear_preprocessed[0].begin(), shaderc_glsl_vertex_shader, "nuklear.vert.spv", options),
        compiler.CompileGlslToSpvAssembly(nuklear_preprocessed[1].begin(), shaderc_glsl_fragment_shader, "nuklear.frag.spv", options) };

    OutputDebugStringA("-------------------------------------------\n");
    OutputDebugStringA(nuklear_compiled_assembly[0].begin());
    OutputDebugStringA("-------------------------------------------\n");
    OutputDebugStringA(nuklear_compiled_assembly[1].begin());
    OutputDebugStringA("-------------------------------------------\n");

    if (shaderc_compilation_status_success != nuklear_compiled_assembly[0].GetCompilationStatus() ||
        shaderc_compilation_status_success != nuklear_compiled_assembly[1].GetCompilationStatus()) {

        OutputDebugStringA(nuklear_compiled_assembly[0].GetErrorMessage().c_str());
        OutputDebugStringA(nuklear_compiled_assembly[1].GetErrorMessage().c_str());

        DebugBreak();
        return;
    }

#endif

    shaderc::SpvCompilationResult nuklear_compiled[] = {
        compiler.CompileGlslToSpv(
            nuklear_preprocessed[ 0 ].begin( ), shaderc_glsl_default_vertex_shader, "nuklear.vert.spv", options ),
        compiler.CompileGlslToSpv(
            nuklear_preprocessed[ 1 ].begin( ), shaderc_glsl_default_fragment_shader, "nuklear.frag.spv", options )};

    if ( shaderc_compilation_status_success != nuklear_compiled[ 0 ].GetCompilationStatus( ) ||
         shaderc_compilation_status_success != nuklear_compiled[ 1 ].GetCompilationStatus( ) ) {
        DebugBreak( );
        return false;
    }

    auto initParametersVk = (InitParametersVk*) init_params;
    if ( nullptr == initParametersVk ) {
        DebugBreak( );
        return false;
    }

    pAlloc          = initParametersVk->pAlloc;
    pDevice         = initParametersVk->pDevice;
    pPhysicalDevice = initParametersVk->pPhysicalDevice;
    pDescPool       = initParametersVk->pDescPool;
    pRenderPass     = initParametersVk->pRenderPass;

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    InitializeStruct( vertexShaderCreateInfo );
    vertexShaderCreateInfo.pCode = (const uint32_t*) nuklear_compiled[ 0 ].begin( );
    vertexShaderCreateInfo.codeSize =
        (size_t) std::distance( nuklear_compiled[ 0 ].begin( ), nuklear_compiled[ 0 ].end( ) ) * sizeof( uint32_t );

    VkShaderModuleCreateInfo fragmentShaderCreateInfo;
    InitializeStruct( fragmentShaderCreateInfo );
    fragmentShaderCreateInfo.pCode = (const uint32_t*) nuklear_compiled[ 1 ].begin( );
    fragmentShaderCreateInfo.codeSize =
        (size_t) std::distance( nuklear_compiled[ 1 ].begin( ), nuklear_compiled[ 1 ].end( ) ) * sizeof( uint32_t );

    TDispatchableHandle< VkShaderModule > hVertexShaderModule;
    TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( pDevice, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( pDevice, fragmentShaderCreateInfo ) ) {
        DebugBreak( );
        return false;
    }

    if ( nullptr != pDevice && false == hFontSampler.IsNotNull( ) ) {
        VkSamplerCreateInfo fontSamplerCreateInfo;
        InitializeStruct( fontSamplerCreateInfo );
        fontSamplerCreateInfo.magFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo.minFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        fontSamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo.minLod       = -1000;
        fontSamplerCreateInfo.maxLod       = +1000;

        if ( false == hFontSampler.Recreate( pDevice, fontSamplerCreateInfo ) ) {
            DebugBreak( );
            return false;
        }
    }
    if ( nullptr != pDevice && false == hDescSetLayout.IsNotNull( ) ) {
        VkDescriptorSetLayoutBinding bindings[ 1 ];
        InitializeStruct( bindings );

        bindings[ 0 ].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[ 0 ].descriptorCount    = 1;
        bindings[ 0 ].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[ 0 ].pImmutableSamplers = hFontSampler;

        VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
        InitializeStruct( descSetLayoutCreateInfo );
        descSetLayoutCreateInfo.bindingCount = 1;
        descSetLayoutCreateInfo.pBindings    = bindings;

        if ( false == hDescSetLayout.Recreate( pDevice, descSetLayoutCreateInfo ) ) {
            DebugBreak( );
            return false;
        }
    }

    if ( nullptr != pDevice && false == hPipelineLayout.IsNotNull( ) ) {
        VkDescriptorSetLayout descSetLayouts[] = {hDescSetLayout};
        if ( false == DescSet.RecreateResourcesFor( pDevice, pDescPool, descSetLayouts ) ) {
            DebugBreak( );
            return false;
        }

        VkPushConstantRange pushConstant;
        InitializeStruct( pushConstant );
        ZeroMemory( pushConstant );
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.size       = sizeof( float[ 4 ] );

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
        InitializeStruct( pipelineLayoutCreateInfo );
        pipelineLayoutCreateInfo.setLayoutCount         = 1;
        pipelineLayoutCreateInfo.pSetLayouts            = descSetLayouts;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

        if ( false == hPipelineLayout.Recreate( pDevice, pipelineLayoutCreateInfo ) ) {
            DebugBreak( );
            return false;
        }
    }

    VkPipelineShaderStageCreateInfo pipelineStages[ 2 ];
    InitializeStruct( pipelineStages );

    pipelineStages[ 0 ].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineStages[ 0 ].module = hVertexShaderModule;
    pipelineStages[ 0 ].pName  = "main";

    pipelineStages[ 1 ].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineStages[ 1 ].module = hFragmentShaderModule;
    pipelineStages[ 1 ].pName  = "main";

    VkVertexInputBindingDescription bindingDescs[ 1 ];
    InitializeStruct( bindingDescs );
    bindingDescs[ 0 ].stride    = sizeof( Vertex );
    bindingDescs[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttributeDesc[ 3 ];
    InitializeStruct( inputAttributeDesc );

    inputAttributeDesc[ 0 ].location = 0;
    inputAttributeDesc[ 0 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 0 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 0 ].offset   = ( size_t )( &( (Vertex*) 0 )->pos );

    inputAttributeDesc[ 1 ].location = 1;
    inputAttributeDesc[ 1 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 1 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 1 ].offset   = ( size_t )( &( (Vertex*) 0 )->uv );

    inputAttributeDesc[ 2 ].location = 2;
    inputAttributeDesc[ 2 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 2 ].format   = VK_FORMAT_R8G8B8A8_UNORM;
    inputAttributeDesc[ 2 ].offset   = ( size_t )( &( (Vertex*) 0 )->col );

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputState;
    InitializeStruct( pipelineVertexInputState );
    pipelineVertexInputState.vertexBindingDescriptionCount   = GetArraySizeU( bindingDescs );
    pipelineVertexInputState.pVertexBindingDescriptions      = bindingDescs;
    pipelineVertexInputState.vertexAttributeDescriptionCount = GetArraySizeU( inputAttributeDesc );
    pipelineVertexInputState.pVertexAttributeDescriptions    = inputAttributeDesc;

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyState;
    InitializeStruct( pipelineInputAssemblyState );
    pipelineInputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo pipelineViewportState;
    InitializeStruct( pipelineViewportState );
    pipelineViewportState.viewportCount = 1;
    pipelineViewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationState;
    InitializeStruct( pipelineRasterizationState );
    pipelineRasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationState.cullMode    = VK_CULL_MODE_NONE;
    pipelineRasterizationState.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationState.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleState;
    InitializeStruct( pipelineMultisampleState );
    pipelineMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
    InitializeStruct( pipelineColorBlendAttachmentState );
    pipelineColorBlendAttachmentState.blendEnable         = VK_TRUE;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilState;
    InitializeStruct( pipelineDepthStencilState );

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendState;
    InitializeStruct( pipelineColorBlendState );
    pipelineColorBlendState.attachmentCount = 1;
    pipelineColorBlendState.pAttachments    = &pipelineColorBlendAttachmentState;

    VkDynamicState dynamicStates[ 2 ] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo pipelineDynamicState;
    InitializeStruct( pipelineDynamicState );
    pipelineDynamicState.dynamicStateCount = GetArraySizeU( dynamicStates );
    pipelineDynamicState.pDynamicStates    = dynamicStates;

    VkGraphicsPipelineCreateInfo graphicsPipeline;
    InitializeStruct( graphicsPipeline );
    graphicsPipeline.stageCount          = GetArraySizeU( pipelineStages );
    graphicsPipeline.pStages             = pipelineStages;
    graphicsPipeline.pVertexInputState   = &pipelineVertexInputState;
    graphicsPipeline.pInputAssemblyState = &pipelineInputAssemblyState;
    graphicsPipeline.pViewportState      = &pipelineViewportState;
    graphicsPipeline.pRasterizationState = &pipelineRasterizationState;
    graphicsPipeline.pMultisampleState   = &pipelineMultisampleState;
    graphicsPipeline.pDepthStencilState  = &pipelineDepthStencilState;
    graphicsPipeline.pColorBlendState    = &pipelineColorBlendState;
    graphicsPipeline.pDynamicState       = &pipelineDynamicState;
    graphicsPipeline.layout              = hPipelineLayout;
    graphicsPipeline.renderPass          = pRenderPass;

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
    InitializeStruct( pipelineCacheCreateInfo );
    hPipelineCache.Recreate( pDevice, pipelineCacheCreateInfo );

    if ( false == hPipeline.Recreate( pDevice, hPipelineCache, graphicsPipeline ) ) {
        DebugBreak( );
        return false;
    }

    return true;
}

void* apemode::NuklearSdlVk::DeviceUploadAtlas( InitParametersBase* init_params, const void* image, int width, int height ) {
    auto           initParametersVk = reinterpret_cast< InitParametersVk* >( init_params );
    const uint8_t* fontImgPixels    = reinterpret_cast< const uint8_t* >( image );
    uint32_t       uploadSize       = width * height * 4 * sizeof( uint8_t );

    // Create the Image:
    {
        VkImageCreateInfo imageCreateInfo;
        InitializeStruct( imageCreateInfo );
        imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo.extent.width  = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth  = 1;
        imageCreateInfo.mipLevels     = 1;
        imageCreateInfo.arrayLayers   = 1;
        imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if ( false == hFontImg.Recreate( pDevice, pPhysicalDevice, imageCreateInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        auto fontImgAllocInfo = hFontImg.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        if ( false == hFontImgMemory.Recreate( pDevice, fontImgAllocInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( false == hFontImg.BindMemory( hFontImgMemory, 0 ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    // Create the Image View:
    {
        VkImageViewCreateInfo fontImgView;
        InitializeStruct( fontImgView );
        fontImgView.image                       = hFontImg;
        fontImgView.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        fontImgView.format                      = VK_FORMAT_R8G8B8A8_UNORM;
        fontImgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        fontImgView.subresourceRange.levelCount = 1;
        fontImgView.subresourceRange.layerCount = 1;

        if ( false == hFontImgView.Recreate( pDevice, fontImgView ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo fontImgDescriptor;
        InitializeStruct( fontImgDescriptor );
        fontImgDescriptor.sampler     = hFontSampler;
        fontImgDescriptor.imageView   = hFontImgView;
        fontImgDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeDescriptorSet;
        InitializeStruct( writeDescriptorSet );
        writeDescriptorSet.dstSet          = DescSet.hSets[ 0 ];
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.pImageInfo      = &fontImgDescriptor;
        vkUpdateDescriptorSets( pDevice, 1, &writeDescriptorSet, 0, nullptr );
    }

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo bufferCreateInfo;
        InitializeStruct( bufferCreateInfo );
        bufferCreateInfo.size        = uploadSize;
        bufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ( false == hUploadBuffer.Recreate( pDevice, pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        auto uploadBufferAllocInfo = hUploadBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        if ( false == hUploadBufferMemory.Recreate( pDevice, uploadBufferAllocInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( false == hUploadBuffer.BindMemory( hUploadBufferMemory, 0 ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    // Upload to Buffer:
    if ( auto mappedUploadBufferData = hUploadBufferMemory.Map( 0, uploadSize, 0 ) ) {
        memcpy( mappedUploadBufferData, fontImgPixels, uploadSize );

        VkMappedMemoryRange range;
        InitializeStruct( range );
        range.memory = hUploadBufferMemory;
        range.size   = uploadSize;
        if ( ResultHandle::Failed( vkFlushMappedMemoryRanges( pDevice, 1, &range ) ) ) {
            hUploadBufferMemory.Unmap( );
            DebugBreak( );
            return nullptr;
        }

        hUploadBufferMemory.Unmap( );
    }

    TDispatchableHandle< VkCommandPool >   cmdPool;
    TDispatchableHandle< VkCommandBuffer > cmdBuffer;
    VkCommandBuffer                        finalCmdBuffer = initParametersVk->pCmdBuffer;

    if ( VK_NULL_HANDLE == finalCmdBuffer ) {
        VkCommandPoolCreateInfo cmdPoolCreateInfo;
        InitializeStruct( cmdPoolCreateInfo );
        cmdPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolCreateInfo.queueFamilyIndex = initParametersVk->QueueFamilyId;
        if ( false == cmdPool.Recreate( initParametersVk->pDevice, cmdPoolCreateInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        VkCommandBufferAllocateInfo cmdBufferAllocInfo;
        InitializeStruct( cmdBufferAllocInfo );
        cmdBufferAllocInfo.commandPool        = cmdPool;
        cmdBufferAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferAllocInfo.commandBufferCount = 1;
        if ( false == cmdBuffer.Recreate( initParametersVk->pDevice, cmdBufferAllocInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        finalCmdBuffer = cmdBuffer;

        if ( false == cmdPool.Reset( false ) ) {
            DebugBreak( );
            return nullptr;
        }

        VkCommandBufferBeginInfo cmdBufferBeginInfo;
        InitializeStruct( cmdBufferBeginInfo );
        cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if ( ResultHandle::Failed( vkBeginCommandBuffer( cmdBuffer, &cmdBufferBeginInfo ) ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    // Copy to Image:
    {
        VkImageMemoryBarrier copyBarrier;
        InitializeStruct( copyBarrier );
        copyBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        copyBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        copyBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.image                       = hFontImg;
        copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyBarrier.subresourceRange.levelCount = 1;
        copyBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            finalCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &copyBarrier );

        VkBufferImageCopy region;
        InitializeStruct( region );
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width           = width;
        region.imageExtent.height          = height;
        region.imageExtent.depth           = 1;

        vkCmdCopyBufferToImage( finalCmdBuffer, hUploadBuffer, hFontImg, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

        VkImageMemoryBarrier useBarrier;
        InitializeStruct( useBarrier );
        useBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        useBarrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
        useBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        useBarrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        useBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        useBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        useBarrier.image                       = hFontImg;
        useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        useBarrier.subresourceRange.levelCount = 1;
        useBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier( cmdBuffer,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              &useBarrier );
    }

    if ( cmdBuffer.IsNotNull( ) ) {
        VkSubmitInfo submitInfo;
        InitializeStruct( submitInfo );
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = cmdBuffer;

        if ( ResultHandle::Failed( vkEndCommandBuffer( cmdBuffer ) ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( ResultHandle::Failed( vkQueueSubmit( initParametersVk->pQueue, 1, &submitInfo, VK_NULL_HANDLE ) ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( ResultHandle::Failed( vkDeviceWaitIdle( initParametersVk->pDevice ) ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    return hFontImg.Handle;
}
