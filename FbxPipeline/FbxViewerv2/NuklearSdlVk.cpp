
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
}

void apemode::NuklearSdlVk::DeviceDestroy( ) {
}

void apemode::NuklearSdlVk::DeviceCreate( InitParametersBase* init_params ) {
    const char* vertex_shader =
        "#version 450 core\n"
        "layout(push_constant) uniform PushConst {\n"
        "mat4 ProjMtx;\n"
        "} uPushConst;\n"
        "layout(location=0) in vec2 Position;\n"
        "layout(location=1) in vec2 TexCoord;\n"
        "layout(location=2) in vec4 Color;\n"
        "out gl_PerVertex{ vec4 gl_Position; };\n"
        "layout(location = 0) out struct {\n"
        "vec2 Frag_UV;\n"
        "vec4 Frag_Color;\n"
        "} Out;\n"
        "void main() {\n"
        "   Out.Frag_UV = TexCoord;\n"
        "   Out.Frag_Color = Color;\n"
        "   gl_Position = uPushConst.ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";

    const char* fragment_shader =
        "#version 450 core\n"
        "layout(location=0) out vec4 Out_Color;\n"
        "layout(set=0, binding=0) uniform sampler2D Texture;\n"
        "layout(location=0) in struct {\n"
        "vec2 Frag_UV;\n"
        "vec4 Frag_Color;\n"
        "} In;\n"
        "void main(){\n"
        "   Out_Color = In.Frag_Color * texture(Texture, In.Frag_UV.st);\n"
        "}\n";

    shaderc::Compiler compiler;
    
    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level_size );

    shaderc::PreprocessedSourceCompilationResult nuklear_preprocessed[] = {
        compiler.PreprocessGlsl( vertex_shader, shaderc_glsl_default_vertex_shader, "nuklear.vert", options ),
        compiler.PreprocessGlsl( fragment_shader, shaderc_glsl_default_fragment_shader, "nuklear.frag", options )};

    if ( shaderc_compilation_status_success != nuklear_preprocessed[ 0 ].GetCompilationStatus( ) ||
         shaderc_compilation_status_success != nuklear_preprocessed[ 1 ].GetCompilationStatus( ) ) {
        DebugBreak( );
        return;
    }

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
    pDevice     = initParametersVk->pDevice;
    pDescPool   = initParametersVk->pDescPool;
    pRenderPass = initParametersVk->pRenderPass;

    TInfoStruct< VkShaderModuleCreateInfo > vertexShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*) nuklear_compiled[ 0 ].begin( );
    vertexShaderCreateInfo->codeSize = (size_t) std::distance( nuklear_compiled[ 0 ].begin( ), nuklear_compiled[ 0 ].end( ) );

    TInfoStruct< VkShaderModuleCreateInfo > fragmentShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*) nuklear_compiled[ 1 ].begin( );
    vertexShaderCreateInfo->codeSize = (size_t) std::distance( nuklear_compiled[ 1 ].begin( ), nuklear_compiled[ 1 ].end( ) );

    TDispatchableHandle< VkShaderModule > hVertexShaderModule;
    TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( *pDevice, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( *pDevice, fragmentShaderCreateInfo ) ) {
        DebugBreak( );
        return;
    }

    if ( nullptr != pDevice && false == hFontSampler.IsNotNull( ) ) {
        apemode::TInfoStruct< VkSamplerCreateInfo > fontSamplerCreateInfo;
        fontSamplerCreateInfo->magFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo->minFilter    = VK_FILTER_LINEAR;
        fontSamplerCreateInfo->mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        fontSamplerCreateInfo->addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        fontSamplerCreateInfo->minLod       = -1000;
        fontSamplerCreateInfo->maxLod       = +1000;

        if ( false == hFontSampler.Recreate( *pDevice, fontSamplerCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }
    if ( nullptr != pDevice && false == hDescSetLayout.IsNotNull( ) ) {
        VkDescriptorSetLayoutBinding binding[ 1 ];
        apemode::ZeroMemory( binding );

        binding[ 0 ].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[ 0 ].descriptorCount    = 1;
        binding[ 0 ].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding[ 0 ].pImmutableSamplers = hFontSampler;

        TInfoStruct< VkDescriptorSetLayoutCreateInfo > descSetLayoutCreateInfo;
        descSetLayoutCreateInfo->bindingCount = 1;
        descSetLayoutCreateInfo->pBindings    = binding;

        if ( false == hDescSetLayout.Recreate( *pDevice, descSetLayoutCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }

    if ( nullptr != pDevice && false == hPipelineLayout.IsNotNull( ) ) {
        VkDescriptorSetLayout descSetLayouts[] = {hDescSetLayout};
        if ( false == DescSet.RecreateResourcesFor( *pDevice, *pDescPool, descSetLayouts ) ) {
            DebugBreak( );
            return;
        }

        VkPushConstantRange pushConstants[ 1 ] = {};
        pushConstants[ 0 ].stageFlags          = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstants[ 0 ].offset              = sizeof( float ) * 0;
        pushConstants[ 0 ].size                = sizeof( float ) * 4;

        TInfoStruct< VkPipelineLayoutCreateInfo > pipelineLayoutCreateInfo;

        pipelineLayoutCreateInfo->setLayoutCount         = 1;
        pipelineLayoutCreateInfo->pSetLayouts            = descSetLayouts;
        pipelineLayoutCreateInfo->pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo->pPushConstantRanges    = pushConstants;

        if ( false == hPipelineLayout.Recreate( *pDevice, pipelineLayoutCreateInfo ) ) {
            DebugBreak( );
            return;
        }
    }

    TInfoStruct< VkPipelineShaderStageCreateInfo > pipelineStages[ 2 ];

    pipelineStages[ 0 ]->stage  = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineStages[ 0 ]->module = hVertexShaderModule;
    pipelineStages[ 0 ]->pName  = "main";

    pipelineStages[ 1 ]->stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineStages[ 1 ]->module = hFragmentShaderModule;
    pipelineStages[ 1 ]->pName  = "main";

    VkVertexInputBindingDescription bindingDescs[ 1 ];
    apemode::ZeroMemory( bindingDescs );
    bindingDescs[ 0 ].stride    = sizeof( Vertex );
    bindingDescs[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription inputAttributeDesc[ 3 ];
    apemode::ZeroMemory( inputAttributeDesc );

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

    TInfoStruct< VkPipelineVertexInputStateCreateInfo > pipelineVertexInputState;
    pipelineVertexInputState->vertexBindingDescriptionCount   = _Get_array_length_u( bindingDescs );
    pipelineVertexInputState->pVertexBindingDescriptions      = bindingDescs;
    pipelineVertexInputState->vertexAttributeDescriptionCount = _Get_array_length_u( inputAttributeDesc );
    pipelineVertexInputState->pVertexAttributeDescriptions    = inputAttributeDesc;

    TInfoStruct< VkPipelineInputAssemblyStateCreateInfo > pipelineInputAssemblyState;
    pipelineInputAssemblyState->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    TInfoStruct< VkPipelineViewportStateCreateInfo > pipelineViewportState;
    pipelineViewportState->viewportCount = 1;
    pipelineViewportState->scissorCount  = 1;

    TInfoStruct< VkPipelineRasterizationStateCreateInfo > pipelineRasterizationState;
    pipelineRasterizationState->polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationState->cullMode    = VK_CULL_MODE_NONE;
    pipelineRasterizationState->frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationState->lineWidth   = 1.0f;

    TInfoStruct< VkPipelineMultisampleStateCreateInfo > pipelineMultisampleState;
    pipelineMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState[ 1 ];
    apemode::ZeroMemory( pipelineColorBlendAttachmentState );
    pipelineColorBlendAttachmentState[ 0 ].blendEnable         = VK_TRUE;
    pipelineColorBlendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].colorBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipelineColorBlendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pipelineColorBlendAttachmentState[ 0 ].alphaBlendOp        = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState[ 0 ].colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    TInfoStruct< VkPipelineDepthStencilStateCreateInfo > pipelineDepthStencilState;

    TInfoStruct< VkPipelineColorBlendStateCreateInfo > pipelineColorBlendState;
    pipelineColorBlendState->attachmentCount = 1;
    pipelineColorBlendState->pAttachments    = pipelineColorBlendAttachmentState;

    VkDynamicState dynamicStates[ 2 ] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    TInfoStruct< VkPipelineDynamicStateCreateInfo > pipelineDynamicState;
    pipelineDynamicState->dynamicStateCount = _Get_array_length_u( dynamicStates );
    pipelineDynamicState->pDynamicStates    = dynamicStates;

    TInfoStruct< VkGraphicsPipelineCreateInfo > graphicsPipeline;
    graphicsPipeline->stageCount          = _Get_array_length_u( pipelineStages );
    graphicsPipeline->pStages             = reinterpret_cast< const VkPipelineShaderStageCreateInfo* >( pipelineStages );
    graphicsPipeline->pVertexInputState   = pipelineVertexInputState;
    graphicsPipeline->pInputAssemblyState = pipelineInputAssemblyState;
    graphicsPipeline->pViewportState      = pipelineViewportState;
    graphicsPipeline->pRasterizationState = pipelineRasterizationState;
    graphicsPipeline->pMultisampleState   = pipelineMultisampleState;
    graphicsPipeline->pDepthStencilState  = pipelineDepthStencilState;
    graphicsPipeline->pColorBlendState    = pipelineColorBlendState;
    graphicsPipeline->pDynamicState       = pipelineDynamicState;
    graphicsPipeline->layout              = hPipelineLayout;
    graphicsPipeline->renderPass          = *pRenderPass;

    if ( false == hPipeline.Recreate( *pDevice, VK_NULL_HANDLE, graphicsPipeline ) ) {
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
        TInfoStruct< VkImageCreateInfo > imageCreateInfo;
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

        if ( false == hFontImg.Recreate( *pDevice, *pDevice, imageCreateInfo ) ) {
            DebugBreak( );
            return;
        }

        if ( false ==
             hFontImgMemory.Recreate( *pDevice, hFontImg.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) ) ) {
            DebugBreak( );
            return;
        }

        if ( false == hFontImg.BindMemory( hFontImgMemory, 0 ) ) {
            DebugBreak( );
            return;
        }
    }

    // Create the Image View:
    {
        TInfoStruct< VkImageViewCreateInfo > fontImgView;
        fontImgView->image                       = hFontImg;
        fontImgView->viewType                    = VK_IMAGE_VIEW_TYPE_2D;
        fontImgView->format                      = VK_FORMAT_R8G8B8A8_UNORM;
        fontImgView->subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        fontImgView->subresourceRange.levelCount = 1;
        fontImgView->subresourceRange.layerCount = 1;

        if ( false == hFontImgView.Recreate( *pDevice, fontImgView ) ) {
            DebugBreak( );
            return;
        }
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo fontImgDescriptor[ 1 ];
        apemode::ZeroMemory( fontImgDescriptor );
        fontImgDescriptor[ 0 ].sampler     = hFontSampler;
        fontImgDescriptor[ 0 ].imageView   = hFontImgView;
        fontImgDescriptor[ 0 ].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        TInfoStruct< VkWriteDescriptorSet > writeDescriptorSet[ 1 ];
        writeDescriptorSet[ 0 ]->dstSet          = DescSet.hSets[ 0 ];
        writeDescriptorSet[ 0 ]->descriptorCount = 1;
        writeDescriptorSet[ 0 ]->descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet[ 0 ]->pImageInfo      = fontImgDescriptor;
        vkUpdateDescriptorSets( *pDevice, 1, writeDescriptorSet[ 0 ], 0, nullptr );
    }

    // Create the Upload Buffer:
    {
        TInfoStruct< VkBufferCreateInfo > bufferCreateInfo;
        bufferCreateInfo->size        = uploadSize;
        bufferCreateInfo->usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if ( false == hUploadBuffer.Recreate( *pDevice, *pDevice, bufferCreateInfo ) ) {
            DebugBreak( );
            return;
        }

        if ( false ==
             hUploadBufferMemory.Recreate( *pDevice, hUploadBuffer.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) ) ) {
            DebugBreak( );
            return;
        }

        if ( false == hUploadBuffer.BindMemory( hUploadBufferMemory, 0 ) ) {
            DebugBreak( );
            return;
        }
    }

    // Upload to Buffer:
    if ( auto mappedUploadBufferData = hUploadBufferMemory.Map( 0, uploadSize, 0 ) ) {
        memcpy( mappedUploadBufferData, fontImgPixels, uploadSize );

        TInfoStruct< VkMappedMemoryRange > range[ 1 ];
        range[ 0 ]->memory = hUploadBufferMemory;
        range[ 0 ]->size   = uploadSize;
        if ( ResultHandle::Failed( vkFlushMappedMemoryRanges( *pDevice, 1, range[ 0 ] ) ) ) {
            hUploadBufferMemory.Unmap( );
            DebugBreak( );
            return;
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

        vkCmdPipelineBarrier( *initParametersVk->pCmdBuffer,
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

        vkCmdCopyBufferToImage( *initParametersVk->pCmdBuffer,
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

        vkCmdPipelineBarrier( *initParametersVk->pCmdBuffer,
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
