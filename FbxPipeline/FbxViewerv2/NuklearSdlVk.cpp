
#include <NuklearSdlVk.h>

#include <shaderc/shaderc.hpp>

static uint32_t MemoryType( VkPhysicalDevice gpu, VkMemoryPropertyFlags properties, uint32_t type_bits ) {

    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties( gpu, &prop );
    for ( uint32_t i = 0; i < prop.memoryTypeCount; i++ )
        if ( ( prop.memoryTypes[ i ].propertyFlags & properties ) == properties && type_bits & ( 1 << i ) )
            return i;

    // Unable to find memoryType
    return 0xffffffff;
}

void apemode::NuklearSdlVk::Render( RenderParametersBase* p ) {

    auto renderParams = (RenderParametersVk*)p;

    if (hVertexBuffer[FrameIndex].IsNull() || VertexBufferSize[FrameIndex] < p->max_vertex_buffer)
    {
        hVertexBuffer[FrameIndex].Destroy();
        hVertexBufferMemory[FrameIndex].Destroy();

        apemodevk::TInfoStruct<VkBufferCreateInfo> bufferCreateInfo;
        bufferCreateInfo->size = p->max_vertex_buffer;
        bufferCreateInfo->usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (false == hVertexBuffer[FrameIndex].Recreate(pDevice, pPhysicalDevice, bufferCreateInfo)) {
            DebugBreak();
            return;
        }

        if (false == hVertexBufferMemory[FrameIndex].Recreate(pDevice, hVertexBuffer[FrameIndex].GetMemoryAllocateInfo(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
            DebugBreak();
            return;
        }

        if (false == hVertexBuffer[FrameIndex].BindMemory(hVertexBufferMemory[FrameIndex], 0)) {
            DebugBreak();
            return;
        }

        VertexBufferSize[FrameIndex] = p->max_vertex_buffer;
    }

    if (hIndexBuffer[FrameIndex].IsNull() || IndexBufferSize[FrameIndex] < p->max_element_buffer)
    {
        hIndexBuffer[FrameIndex].Destroy();
        hIndexBufferMemory[FrameIndex].Destroy();

        apemodevk::TInfoStruct<VkBufferCreateInfo> bufferCreateInfo;
        bufferCreateInfo->size = p->max_element_buffer;
        bufferCreateInfo->usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (false == hIndexBuffer[FrameIndex].Recreate(pDevice, pPhysicalDevice, bufferCreateInfo)) {
            DebugBreak();
            return;
        }

        if (false == hIndexBufferMemory[FrameIndex].Recreate(pDevice, hIndexBuffer[FrameIndex].GetMemoryAllocateInfo(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
            DebugBreak();
            return;
        }

        if (false == hIndexBuffer[FrameIndex].BindMemory(hIndexBufferMemory[FrameIndex], 0)) {
            DebugBreak();
            return;
        }

        IndexBufferSize[FrameIndex] = p->max_element_buffer;
    }

    {
        /* convert from command queue into draw list and draw to screen */
        void * vertices = nullptr;
        void * elements = nullptr;

        /* load vertices/elements directly into vertex/element buffer */
        vertices = hVertexBufferMemory[FrameIndex].Map(0, VertexBufferSize[FrameIndex], 0);
        elements = hIndexBufferMemory[FrameIndex].Map(0, IndexBufferSize[FrameIndex], 0);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                { NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, position) },
                { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(Vertex, uv) },
                { NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(Vertex, col) },
                { NK_VERTEX_LAYOUT_END } };

            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(Vertex);
            config.vertex_alignment = NK_ALIGNOF(Vertex);
            config.null = NullTexture;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = p->aa;
            config.line_AA = p->aa;

            /* setup buffers to load vertices and elements */
            {
                nk_buffer vbuf, ebuf;
                nk_buffer_init_fixed(&vbuf, vertices, (nk_size)p->max_vertex_buffer);
                nk_buffer_init_fixed(&ebuf, elements, (nk_size)p->max_element_buffer);
                nk_convert(&Context, &RenderCmds, &vbuf, &ebuf, &config);
            }
        }

        VkMappedMemoryRange range[2] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = hVertexBufferMemory[FrameIndex];
        range[0].size = VK_WHOLE_SIZE;
        range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[1].memory = hIndexBufferMemory[FrameIndex];
        range[1].size = VK_WHOLE_SIZE;

        if (apemodevk::ResultHandle::Failed(vkFlushMappedMemoryRanges(pDevice, 2, range))) {
            DebugBreak();
            return;
        }

        hVertexBufferMemory[FrameIndex].Unmap();
        hIndexBufferMemory[FrameIndex].Unmap();
    }

    // Bind pipeline and descriptor sets:
    {
        VkDescriptorSet descSets[1] = { DescSet.hSets[0] };
        vkCmdBindPipeline(renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline);
        vkCmdBindDescriptorSets(renderParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipelineLayout, 0, 1, descSets, 0, NULL);
    }

    // Bind Vertex And Index Buffer:
    {
        VkBuffer vertexBuffers[1] = { hVertexBuffer[FrameIndex] };
        VkDeviceSize vertexOffsets[1] = { 0 };
        vkCmdBindVertexBuffers(renderParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets);
        vkCmdBindIndexBuffer(renderParams->pCmdBuffer, hIndexBuffer[FrameIndex], 0, VK_INDEX_TYPE_UINT16);
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = p->dims[0] * p->scale[0];
        viewport.height = p->dims[1] * p->scale[1];
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(renderParams->pCmdBuffer, 0, 1, &viewport);
    }


    // Setup scale and translation:
    {
        const float ortho[4][4] = {
            { 2.0f / p->dims[0], 0.0f, 0.0f, 0.0f },
            { 0.0f, -2.0f / p->dims[1], 0.0f, 0.0f },
            { 0.0f, 0.0f, -1.0f, 0.0f },
            { -1.0f, 1.0f, 0.0f, 1.0f },
        };

        vkCmdPushConstants(renderParams->pCmdBuffer, hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float [4][4]), ortho);
    }

    const nk_draw_command *cmd = nullptr;
    uint32_t  offset = 0;
    uint32_t vtx_offset = 0;

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, &Context, &RenderCmds) {
        if (!cmd->elem_count)
            continue;

        VkRect2D scissor;
        scissor.offset.x = (cmd->clip_rect.x * p->scale[0]);
        scissor.offset.y = ((p->dims[1] - (cmd->clip_rect.y + cmd->clip_rect.h)) * p->scale[1]);
        scissor.extent.width = (cmd->clip_rect.w * p->scale[0]);
        scissor.extent.height = (cmd->clip_rect.h * p->scale[1]);

        vkCmdSetScissor(renderParams->pCmdBuffer, 0, 1, &scissor);
        vkCmdDrawIndexed(renderParams->pCmdBuffer, cmd->elem_count, 1, offset, vtx_offset, 0);

        offset += cmd->elem_count;
    }

    nk_clear(&Context);
}

void apemode::NuklearSdlVk::DeviceDestroy( ) {
}

void apemode::NuklearSdlVk::DeviceCreate( InitParametersBase* init_params ) {
    const char* vertex_shader =
        "#version 450\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "layout(push_constant) uniform PushConstants { mat4 modelViewProjection; } pushConstants;\n"
        "layout(location=0) in vec2 inPosition;\n"
        "layout(location=1) in vec2 inTexcoords;\n"
        "layout(location=2) in vec4 inColor;\n"
        "layout(location=0) out vec2 outTexcoords;\n"
        "layout(location=1) out vec4 outColor;\n"
        "void main() {\n"
        "   outTexcoords = inTexcoords;\n"
        "   outColor = inColor;\n"
        "   gl_Position = pushConstants.modelViewProjection * vec4(inPosition.xy, 0, 1);\n"
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
        OutputDebugStringA(nuklear_preprocessed[0].GetErrorMessage().c_str());
        OutputDebugStringA(nuklear_preprocessed[1].GetErrorMessage().c_str());
        DebugBreak( );
        return;
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
        compiler.CompileGlslToSpv( nuklear_preprocessed[ 0 ].begin(), shaderc_glsl_default_vertex_shader, "nuklear.vert.spv", options ),
        compiler.CompileGlslToSpv( nuklear_preprocessed[ 1 ].begin(), shaderc_glsl_default_fragment_shader, "nuklear.frag.spv", options )};

    if ( shaderc_compilation_status_success != nuklear_compiled[ 0 ].GetCompilationStatus( ) ||
         shaderc_compilation_status_success != nuklear_compiled[ 1 ].GetCompilationStatus( ) ) {
        DebugBreak( );
        return;
    }

    auto initParametersVk = (InitParametersVk*) init_params;
    if ( nullptr == initParametersVk )
        return;

    pAlloc      = initParametersVk->pAlloc;
    pDevice = initParametersVk->pDevice;
    pPhysicalDevice = initParametersVk->pPhysicalDevice;
    pDescPool   = initParametersVk->pDescPool;
    pRenderPass = initParametersVk->pRenderPass;

#if 0
    apemodevk::TInfoStruct< VkShaderModuleCreateInfo > vertexShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*)nuklear_compiled_assembly[ 0 ].begin( );
    vertexShaderCreateInfo->codeSize = (size_t) std::distance(nuklear_compiled_assembly[ 0 ].begin( ), nuklear_compiled_assembly[ 0 ].end( ) );

    apemodevk::TInfoStruct< VkShaderModuleCreateInfo > fragmentShaderCreateInfo;
    fragmentShaderCreateInfo->pCode = (const uint32_t*)nuklear_compiled_assembly[ 1 ].begin( );
    fragmentShaderCreateInfo->codeSize = (size_t) std::distance(nuklear_compiled_assembly[ 1 ].begin( ), nuklear_compiled_assembly[ 1 ].end( ) );

#else

    apemodevk::TInfoStruct< VkShaderModuleCreateInfo > vertexShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*)nuklear_compiled[0].begin();
    vertexShaderCreateInfo->codeSize = (size_t)std::distance(nuklear_compiled[0].begin(), nuklear_compiled[0].end()) * sizeof(uint32_t);

    apemodevk::TInfoStruct< VkShaderModuleCreateInfo > fragmentShaderCreateInfo;
    fragmentShaderCreateInfo->pCode = (const uint32_t*)nuklear_compiled[1].begin();
    fragmentShaderCreateInfo->codeSize = (size_t)std::distance(nuklear_compiled[1].begin(), nuklear_compiled[1].end()) * sizeof(uint32_t);

#endif

    apemodevk::TDispatchableHandle< VkShaderModule > hVertexShaderModule;
    apemodevk::TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( pDevice, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( pDevice, fragmentShaderCreateInfo ) ) {
        DebugBreak( );
        return;
    }

    if ( nullptr != pDevice && false == hFontSampler.IsNotNull( ) ) {
        apemodevk::TInfoStruct< VkSamplerCreateInfo > fontSamplerCreateInfo;
        fontSamplerCreateInfo->magFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo->minFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo->mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        fontSamplerCreateInfo->addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->minLod       = -1000;
        fontSamplerCreateInfo->maxLod       = +1000;

        if ( false == hFontSampler.Recreate( pDevice, fontSamplerCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }
    if ( nullptr != pDevice && false == hDescSetLayout.IsNotNull( ) ) {
        VkDescriptorSetLayoutBinding binding[ 1 ];
        apemodevk::ZeroMemory( binding );

        binding[ 0 ].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[ 0 ].descriptorCount    = 1;
        binding[ 0 ].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[ 0 ].pImmutableSamplers = hFontSampler;

        apemodevk::TInfoStruct< VkDescriptorSetLayoutCreateInfo > descSetLayoutCreateInfo;
        descSetLayoutCreateInfo->bindingCount = 1;
        descSetLayoutCreateInfo->pBindings    = binding;

        if ( false == hDescSetLayout.Recreate( pDevice, descSetLayoutCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }

    if ( nullptr != pDevice && false == hPipelineLayout.IsNotNull( ) ) {
        VkDescriptorSetLayout descSetLayouts[] = {hDescSetLayout};
        if ( false == DescSet.RecreateResourcesFor( pDevice, pDescPool, descSetLayouts ) ) {
            DebugBreak( );
            return;
        }

        VkPushConstantRange pushConstants[ 1 ];
        apemodevk::ZeroMemory(pushConstants);
        pushConstants[ 0 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstants[ 0 ].size       = sizeof( float [4][4] );

        apemodevk::TInfoStruct< VkPipelineLayoutCreateInfo > pipelineLayoutCreateInfo;
        pipelineLayoutCreateInfo->setLayoutCount         = 1;
        pipelineLayoutCreateInfo->pSetLayouts            = descSetLayouts;
        pipelineLayoutCreateInfo->pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo->pPushConstantRanges    = pushConstants;

        if ( false == hPipelineLayout.Recreate( pDevice, pipelineLayoutCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }

    apemodevk::TInfoStruct< VkPipelineShaderStageCreateInfo > pipelineStages[ 2 ];

    pipelineStages[ 0 ]->stage  = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineStages[ 0 ]->module = hVertexShaderModule;
    pipelineStages[ 0 ]->pName  = "main";

    pipelineStages[ 1 ]->stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineStages[ 1 ]->module = hFragmentShaderModule;
    pipelineStages[ 1 ]->pName  = "main";

    VkVertexInputBindingDescription bindingDescs[ 1 ];
    apemodevk::ZeroMemory( bindingDescs );
    bindingDescs[ 0 ].stride    = sizeof( Vertex );
    bindingDescs[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttributeDesc[ 3 ];
    apemodevk::ZeroMemory( inputAttributeDesc );

    inputAttributeDesc[ 0 ].location = 0;
    inputAttributeDesc[ 0 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 0 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 0 ].offset   = ( size_t )( &( (Vertex*) 0 )->position );

    inputAttributeDesc[ 1 ].location = 1;
    inputAttributeDesc[ 1 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 1 ].format   = VK_FORMAT_R32G32_SFLOAT;
    inputAttributeDesc[ 1 ].offset   = ( size_t )( &( (Vertex*) 0 )->uv );

    inputAttributeDesc[ 2 ].location = 2;
    inputAttributeDesc[ 2 ].binding  = bindingDescs[ 0 ].binding;
    inputAttributeDesc[ 2 ].format   = VK_FORMAT_R8G8B8A8_UNORM;
    inputAttributeDesc[ 2 ].offset   = ( size_t )( &( (Vertex*) 0 )->col );

    apemodevk::TInfoStruct< VkPipelineVertexInputStateCreateInfo > pipelineVertexInputState;
    pipelineVertexInputState->vertexBindingDescriptionCount   = _Get_array_length_u( bindingDescs );
    pipelineVertexInputState->pVertexBindingDescriptions      = bindingDescs;
    pipelineVertexInputState->vertexAttributeDescriptionCount = _Get_array_length_u( inputAttributeDesc );
    pipelineVertexInputState->pVertexAttributeDescriptions    = inputAttributeDesc;

    apemodevk::TInfoStruct< VkPipelineInputAssemblyStateCreateInfo > pipelineInputAssemblyState;
    pipelineInputAssemblyState->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    apemodevk::TInfoStruct< VkPipelineViewportStateCreateInfo > pipelineViewportState;
    pipelineViewportState->viewportCount = 1;
    pipelineViewportState->scissorCount  = 1;

    apemodevk::TInfoStruct< VkPipelineRasterizationStateCreateInfo > pipelineRasterizationState;
    pipelineRasterizationState->polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationState->cullMode    = VK_CULL_MODE_NONE;
    pipelineRasterizationState->frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationState->lineWidth   = 1.0f;

    apemodevk::TInfoStruct< VkPipelineMultisampleStateCreateInfo > pipelineMultisampleState;
    pipelineMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState[ 1 ];
    apemodevk::ZeroMemory( pipelineColorBlendAttachmentState );
    pipelineColorBlendAttachmentState[ 0 ].blendEnable         = VK_TRUE;
    pipelineColorBlendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].colorBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState[ 0 ].alphaBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState[ 0 ].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    apemodevk::TInfoStruct< VkPipelineDepthStencilStateCreateInfo > pipelineDepthStencilState;

    apemodevk::TInfoStruct< VkPipelineColorBlendStateCreateInfo > pipelineColorBlendState;
    pipelineColorBlendState->attachmentCount = 1;
    pipelineColorBlendState->pAttachments    = pipelineColorBlendAttachmentState;

    VkDynamicState dynamicStates[ 2 ] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    apemodevk::TInfoStruct< VkPipelineDynamicStateCreateInfo > pipelineDynamicState;
    pipelineDynamicState->dynamicStateCount = _Get_array_length_u( dynamicStates );
    pipelineDynamicState->pDynamicStates    = dynamicStates;

    apemodevk::TInfoStruct< VkGraphicsPipelineCreateInfo > graphicsPipeline;
    graphicsPipeline->stageCount          = _Get_array_length_u( pipelineStages );
    graphicsPipeline->pStages             = reinterpret_cast< const VkPipelineShaderStageCreateInfo* >( &pipelineStages[0] );
    graphicsPipeline->pVertexInputState   = pipelineVertexInputState;
    graphicsPipeline->pInputAssemblyState = pipelineInputAssemblyState;
    graphicsPipeline->pViewportState      = pipelineViewportState;
    graphicsPipeline->pRasterizationState = pipelineRasterizationState;
    graphicsPipeline->pMultisampleState   = pipelineMultisampleState;
    graphicsPipeline->pDepthStencilState  = pipelineDepthStencilState;
    graphicsPipeline->pColorBlendState    = pipelineColorBlendState;
    graphicsPipeline->pDynamicState       = pipelineDynamicState;
    graphicsPipeline->layout              = hPipelineLayout;
    graphicsPipeline->renderPass          = pRenderPass;

    apemodevk::TInfoStruct<VkPipelineCacheCreateInfo> pipelineCacheCreateInfo;
    pipelineCacheCreateInfo->pInitialData = nullptr;
    pipelineCacheCreateInfo->initialDataSize = 0;
    hPipelineCache.Recreate( pDevice, pipelineCacheCreateInfo );

    if ( false == hPipeline.Recreate( pDevice, hPipelineCache, graphicsPipeline ) ) {
        DebugBreak( );
        return;
    }
}

void* apemode::NuklearSdlVk::DeviceUploadAtlas( InitParametersBase* init_params, const void* image, int width, int height ) {
    auto           initParametersVk = reinterpret_cast< InitParametersVk* >( init_params );
    const uint8_t* fontImgPixels    = reinterpret_cast< const uint8_t* >( image );
    size_t         uploadSize       = width * height * 4 * sizeof( uint8_t );

    // Create the Image:
    {
        apemodevk::TInfoStruct< VkImageCreateInfo > imageCreateInfo;
        imageCreateInfo->imageType     = VK_IMAGE_TYPE_2D;
        imageCreateInfo->format        = VK_FORMAT_R8G8B8A8_UNORM;
        imageCreateInfo->extent.width  = width;
        imageCreateInfo->extent.height = height;
        imageCreateInfo->extent.depth  = 1;
        imageCreateInfo->mipLevels     = 1;
        imageCreateInfo->arrayLayers   = 1;
        imageCreateInfo->samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo->tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo->usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCreateInfo->sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if ( false == hFontImg.Recreate( pDevice, pPhysicalDevice, imageCreateInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( false ==
             hFontImgMemory.Recreate( pDevice, hFontImg.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) ) {
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
        apemodevk::TInfoStruct< VkImageViewCreateInfo > fontImgView;
        fontImgView->image                       = hFontImg;
        fontImgView->viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        fontImgView->format                      = VK_FORMAT_R8G8B8A8_UNORM;
        fontImgView->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        fontImgView->subresourceRange.levelCount = 1;
        fontImgView->subresourceRange.layerCount = 1;

        if ( false == hFontImgView.Recreate( pDevice, fontImgView ) ) {
            DebugBreak( );
            return nullptr;
        }
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo fontImgDescriptor[ 1 ];
        apemodevk::ZeroMemory( fontImgDescriptor );
        fontImgDescriptor[ 0 ].sampler     = hFontSampler;
        fontImgDescriptor[ 0 ].imageView   = hFontImgView;
        fontImgDescriptor[ 0 ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        apemodevk::TInfoStruct< VkWriteDescriptorSet > writeDescriptorSet[ 1 ];
        writeDescriptorSet[ 0 ]->dstSet          = DescSet.hSets[ 0 ];
        writeDescriptorSet[ 0 ]->descriptorCount = 1;
        writeDescriptorSet[ 0 ]->descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet[ 0 ]->pImageInfo      = fontImgDescriptor;
        vkUpdateDescriptorSets( pDevice, 1, writeDescriptorSet[ 0 ], 0, nullptr );
    }

    // Create the Upload Buffer:
    {
        apemodevk::TInfoStruct< VkBufferCreateInfo > bufferCreateInfo;
        bufferCreateInfo->size        = uploadSize;
        bufferCreateInfo->usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ( false == hUploadBuffer.Recreate( pDevice, pPhysicalDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return nullptr;
        }

        if ( false ==
             hUploadBufferMemory.Recreate( pDevice, hUploadBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ) ) {
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

        apemodevk::TInfoStruct< VkMappedMemoryRange > range[ 1 ];
        range[ 0 ]->memory = hUploadBufferMemory;
        range[ 0 ]->size   = uploadSize;
        if ( apemodevk::ResultHandle::Failed( vkFlushMappedMemoryRanges( pDevice, 1, range[ 0 ] ) ) ) {
            hUploadBufferMemory.Unmap( );
            DebugBreak( );
            return nullptr;
        }

        hUploadBufferMemory.Unmap( );
    }

    // Copy to Image:
    {
        VkImageMemoryBarrier copy_barrier[ 1 ]        = {};
        copy_barrier[ 0 ].sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copy_barrier[ 0 ].dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        copy_barrier[ 0 ].oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
        copy_barrier[ 0 ].newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copy_barrier[ 0 ].srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[ 0 ].dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        copy_barrier[ 0 ].image                       = hFontImg;
        copy_barrier[ 0 ].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_barrier[ 0 ].subresourceRange.levelCount = 1;
        copy_barrier[ 0 ].subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier( initParametersVk->pCmdBuffer,
                              VK_PIPELINE_STAGE_HOST_BIT,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              copy_barrier );

        VkBufferImageCopy region           = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width           = width;
        region.imageExtent.height          = height;
        region.imageExtent.depth           = 1;

        vkCmdCopyBufferToImage( initParametersVk->pCmdBuffer,
                                hUploadBuffer,
                                hFontImg,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1,
                                &region );

        VkImageMemoryBarrier use_barrier[ 1 ]        = {};
        use_barrier[ 0 ].sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        use_barrier[ 0 ].srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
        use_barrier[ 0 ].dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
        use_barrier[ 0 ].oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        use_barrier[ 0 ].newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        use_barrier[ 0 ].srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[ 0 ].dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
        use_barrier[ 0 ].image                       = hFontImg;
        use_barrier[ 0 ].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        use_barrier[ 0 ].subresourceRange.levelCount = 1;
        use_barrier[ 0 ].subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier( initParametersVk->pCmdBuffer,
                              VK_PIPELINE_STAGE_TRANSFER_BIT,
                              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                              0,
                              0,
                              NULL,
                              0,
                              NULL,
                              1,
                              use_barrier );
    }

    return hFontImg.Handle;
}
