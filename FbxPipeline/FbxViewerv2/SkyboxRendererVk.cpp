#include <fbxvpch.h>

#include <SkyboxRendererVk.h>

#include <BufferPools.Vulkan.h>
#include <QueuePools.Vulkan.h>
#include <ShaderCompiler.Vulkan.h>

#include <ArrayUtils.h>
#include <shaderc/shaderc.hpp>

struct FrameUniformBuffer {
    apemodem::mat4 projMarix;
    apemodem::mat4 envMarix;
    apemodem::vec4 params0;
    apemodem::vec4 params1;
};

struct SkyboxVertex {
    float position[ 3 ];
    float texcoords[ 2 ];
};

void FillScreenSpaceQuad( SkyboxVertex* pOutVertices,
                          const float   textureWidth,
                          const float   textureHeight,
                          const float   texelHalf = 0.0f,
                          const float   width     = 1.0f,
                          const float   height    = 1.0f ) {
    const float minx = -width;
    const float maxx = +width;
    const float miny = 0.0f;
    const float maxy = height * 2.0f;

    const float texelHalfW = texelHalf / textureWidth;
    const float texelHalfH = texelHalf / textureHeight;
    const float minu       = -1.0f + texelHalfW;
    const float maxu       = +1.0f + texelHalfH;

    const float zz = 0.0f;

    float minv = texelHalfH;
    float maxv = 2.0f + texelHalfH;

    pOutVertices[ 0 ].position[ 0 ]  = minx;
    pOutVertices[ 0 ].position[ 1 ]  = miny;
    pOutVertices[ 0 ].position[ 2 ]  = zz; 
    pOutVertices[ 0 ].texcoords[ 0 ] = minu;
    pOutVertices[ 0 ].texcoords[ 1 ] = minv;

    pOutVertices[ 1 ].position[ 0 ]  = maxx;
    pOutVertices[ 1 ].position[ 1 ]  = miny;
    pOutVertices[ 1 ].position[ 2 ]  = zz;
    pOutVertices[ 1 ].texcoords[ 0 ] = maxu;
    pOutVertices[ 1 ].texcoords[ 1 ] = minv;

    pOutVertices[ 2 ].position[ 0 ]  = maxx;
    pOutVertices[ 2 ].position[ 1 ]  = maxy;
    pOutVertices[ 2 ].position[ 2 ]  = zz;
    pOutVertices[ 2 ].texcoords[ 0 ] = maxu;
    pOutVertices[ 2 ].texcoords[ 1 ] = maxv;
}

void ProcessSkyboxTexcoords( SkyboxVertex* pOutVertices,
                             float         fieldOfView,
                             const float   textureWidth,
                             const float   textureHeight ) {

    const float tanHalfFov = tanf( 0.5f * fieldOfView );
    const float aspectWidthHeight = textureWidth / textureHeight;

    for ( uint32_t i = 0; i < 3; ++i ) {
        auto& skyboxVertex = pOutVertices[ i ];

        apemodem::vec2 texcoords;
        texcoords.x = skyboxVertex.texcoords[ 0 ];
        texcoords.y = skyboxVertex.texcoords[ 1 ];

        texcoords *= 2.0f;
        texcoords -= 1.0f;

        texcoords.x *= aspectWidthHeight;
        texcoords *= tanHalfFov;

        skyboxVertex.texcoords[ 0 ] = texcoords.x;
        skyboxVertex.texcoords[ 1 ] = texcoords.y;
    }
}

bool apemodevk::SkyboxRenderer::Recreate( RecreateParameters* pParams ) {
    if ( nullptr == pParams->pNode )
        return false;

    std::set< std::string > includedFiles;
    std::vector< uint8_t >  compiledShaders[ 2 ];

    if ( false == pParams->pShaderCompiler->Compile( "shaders/apemode/Skyboxv2.vert",
                                                     {},
                                                     apemodevk::ShaderCompiler::eShaderType_GLSL_VertexShader,
                                                     includedFiles,
                                                     compiledShaders[ 0 ] ) ||
         false == pParams->pShaderCompiler->Compile( "shaders/apemode/Skybox.frag",
                                                     {},
                                                     apemodevk::ShaderCompiler::eShaderType_GLSL_FragmentShader,
                                                     includedFiles,
                                                     compiledShaders[ 1 ] ) ) {
        return false;
    }

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    InitializeStruct( vertexShaderCreateInfo );
    vertexShaderCreateInfo.pCode    = (const uint32_t*) compiledShaders[ 0 ].data( );
    vertexShaderCreateInfo.codeSize = compiledShaders[ 0 ].size( );

    VkShaderModuleCreateInfo fragmentShaderCreateInfo;
    InitializeStruct( fragmentShaderCreateInfo );
    fragmentShaderCreateInfo.pCode    = (const uint32_t*) compiledShaders[ 1 ].data( );
    fragmentShaderCreateInfo.codeSize = compiledShaders[ 1 ].size( );

    TDispatchableHandle< VkShaderModule > hVertexShaderModule;
    TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
    if ( false == hVertexShaderModule.Recreate( *pParams->pNode, vertexShaderCreateInfo ) ||
         false == hFragmentShaderModule.Recreate( *pParams->pNode, fragmentShaderCreateInfo ) ) {
        return false;
    }

    VkDescriptorSetLayoutBinding bindings[ 2 ];
    InitializeStruct( bindings );

    bindings[ 0 ].binding         = 0;
    bindings[ 0 ].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[ 0 ].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[ 0 ].descriptorCount = 1;

    bindings[ 1 ].binding         = 1;
    bindings[ 1 ].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;
    bindings[ 1 ].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[ 1 ].descriptorCount = 1;

    VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo;
    InitializeStruct( descSetLayoutCreateInfo );

    descSetLayoutCreateInfo.bindingCount = GetArraySizeU( bindings );
    descSetLayoutCreateInfo.pBindings    = bindings;

    if ( false == hDescSetLayout.Recreate( *pParams->pNode, descSetLayoutCreateInfo ) ) {
        return false;
    }

    VkDescriptorSetLayout descriptorSetLayouts[ kMaxFrameCount ];
    for ( auto& descriptorSetLayout : descriptorSetLayouts ) {
        descriptorSetLayout = hDescSetLayout;
    }

    if ( false == DescSets.RecreateResourcesFor( *pParams->pNode, pParams->pDescPool, descriptorSetLayouts ) ) {
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    InitializeStruct( pipelineLayoutCreateInfo );
    pipelineLayoutCreateInfo.setLayoutCount = GetArraySizeU( descriptorSetLayouts );
    pipelineLayoutCreateInfo.pSetLayouts    = descriptorSetLayouts;

    if ( false == hPipelineLayout.Recreate( *pParams->pNode, pipelineLayoutCreateInfo ) ) {
        return false;
    }

    VkGraphicsPipelineCreateInfo           graphicsPipelineCreateInfo;
    VkPipelineCacheCreateInfo              pipelineCacheCreateInfo;
    VkPipelineVertexInputStateCreateInfo   vertexInputStateCreateInfo;
    VkVertexInputAttributeDescription      vertexInputAttributeDescription[ 2 ];
    VkVertexInputBindingDescription        vertexInputBindingDescription[ 1 ];
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
    VkPipelineColorBlendStateCreateInfo    colorBlendStateCreateInfo;
    VkPipelineColorBlendAttachmentState    colorBlendAttachmentState[ 1 ];
    VkPipelineDepthStencilStateCreateInfo  depthStencilStateCreateInfo;
    VkPipelineViewportStateCreateInfo      viewportStateCreateInfo;
    VkPipelineMultisampleStateCreateInfo   multisampleStateCreateInfo;
    VkDynamicState                         dynamicStateEnables[ 2 ];
    VkPipelineDynamicStateCreateInfo       dynamicStateCreateInfo;
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
#if 0

    vertexInputBindingDescription[ 0 ].stride    = sizeof( SkyboxVertex );
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (SkyboxVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R16G16_UNORM;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (SkyboxVertex*) 0 )->texcoords );

#else

    vertexInputBindingDescription[ 0 ].stride    = sizeof(SkyboxVertex);
    vertexInputBindingDescription[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    vertexInputAttributeDescription[ 0 ].location = 0;
    vertexInputAttributeDescription[ 0 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
    vertexInputAttributeDescription[ 0 ].offset   = ( size_t )( &( (SkyboxVertex*) 0 )->position );

    vertexInputAttributeDescription[ 1 ].location = 1;
    vertexInputAttributeDescription[ 1 ].binding  = vertexInputBindingDescription[ 0 ].binding;
    vertexInputAttributeDescription[ 1 ].format   = VK_FORMAT_R32G32_SFLOAT;
    vertexInputAttributeDescription[ 1 ].offset   = ( size_t )( &( (SkyboxVertex*) 0 )->texcoords );

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

    rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_NONE;
    rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_CLOCKWISE; /* CW */
    rasterizationStateCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.polygonMode             = VK_POLYGON_MODE_FILL;
    //rasterizationStateCreateInfo.cullMode                = VK_CULL_MODE_BACK_BIT;
    //rasterizationStateCreateInfo.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE; /* CCW */
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

    depthStencilStateCreateInfo.depthTestEnable       = VK_FALSE;
    depthStencilStateCreateInfo.depthWriteEnable      = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp        = VK_COMPARE_OP_NEVER;
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.stencilTestEnable     = VK_FALSE;
    depthStencilStateCreateInfo.back.failOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.passOp           = VK_STENCIL_OP_KEEP;
    depthStencilStateCreateInfo.back.compareOp        = VK_COMPARE_OP_NEVER;
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
        return false;
    }

    if ( false == hPipeline.Recreate( *pParams->pNode, hPipelineCache, graphicsPipelineCreateInfo ) ) {
        return false;
    }

    VkPhysicalDeviceProperties adapterProps;
    vkGetPhysicalDeviceProperties( *pParams->pNode, &adapterProps ); 

    for ( uint32_t i = 0; i < pParams->FrameCount; ++i ) {
        BufferPools[ i ].Recreate( *pParams->pNode, *pParams->pNode, &adapterProps.limits, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, false );
        DescSetPools[ i ].Recreate( *pParams->pNode, pParams->pDescPool, hDescSetLayout );
    }

    const uint32_t vertexBufferSize = sizeof(SkyboxVertex) * 3;

    VkBufferCreateInfo bufferCreateInfo;
    InitializeStruct( bufferCreateInfo );
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.size  = vertexBufferSize;

    if ( false == hVertexBuffer.Recreate( *pParams->pNode, *pParams->pNode, bufferCreateInfo ) ) {
        return false;
    }

    auto vertexBufferMemoryProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto memoryAllocateInfo = hVertexBuffer.GetMemoryAllocateInfo( vertexBufferMemoryProps );
    if ( false == hVertexBufferMemory.Recreate( *pParams->pNode, memoryAllocateInfo ) ) {
        return false;
    }

    if ( false == hVertexBuffer.BindMemory( hVertexBufferMemory, 0 ) ) {
        return false;
    }

    return true;
}

void apemodevk::SkyboxRenderer::Reset(uint32_t FrameIndex)
{
    BufferPools[ FrameIndex ].Reset( );
}

bool apemodevk::SkyboxRenderer::Render( Skybox* pSkybox, RenderParameters* pParams ) {

    const uint32_t vertexBufferSize = sizeof( SkyboxVertex ) * 3;

    if ( auto mappedVertices = (SkyboxVertex*) hVertexBufferMemory.Map( 0, vertexBufferSize, 0 ) ) {
        const apemodem::vec2 textureWidthHeight = pParams->Dims * pParams->Scale;
        FillScreenSpaceQuad( mappedVertices, textureWidthHeight.x, textureWidthHeight.y );
        //ProcessSkyboxTexcoords( mappedVertices, pParams->FieldOfView, textureWidthHeight.x, textureWidthHeight.y );

        VkMappedMemoryRange range;
        InitializeStruct( range );
        range.memory = hVertexBufferMemory;
        range.size   = vertexBufferSize;
        hVertexBufferMemory.Unmap( );
    }

    auto FrameIndex = (pParams->FrameIndex) % kMaxFrameCount;

    FrameUniformBuffer frameData;
    frameData.envMarix  = pParams->EnvMatrix;
    frameData.projMarix = pParams->ProjMatrix;
    frameData.params0.x = 0;                      /* u_lerpFactor */
    frameData.params0.y = pSkybox->Exposure;      /* u_exposure0 */
    frameData.params0.z = pSkybox->Dimension;     /* u_textureCubeDim0 */
    frameData.params0.w = pSkybox->LevelOfDetail; /* u_textureCubeLod0 */

    auto suballocResult = BufferPools[ FrameIndex ].TSuballocate( frameData );
    assert( VK_NULL_HANDLE != suballocResult.descBufferInfo.buffer );
    suballocResult.descBufferInfo.range = sizeof( FrameUniformBuffer );

    VkDescriptorImageInfo skyboxImageInfo;
    InitializeStruct( skyboxImageInfo );
    skyboxImageInfo.imageLayout = pSkybox->eImgLayout;
    skyboxImageInfo.imageView   = pSkybox->pImgView;
    skyboxImageInfo.sampler     = pSkybox->pSampler;

    VkDescriptorSet descriptorSet[ 1 ]  = {nullptr};
    uint32_t        dynamicOffsets[ 1 ] = {0};

    TDescriptorSet< 2 > descSet;
    descSet.pBinding[ 0 ].eDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descSet.pBinding[ 0 ].BufferInfo      = suballocResult.descBufferInfo;
    descSet.pBinding[ 1 ].eDescriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descSet.pBinding[ 1 ].ImageInfo       = skyboxImageInfo;

    descriptorSet[ 0 ]  = DescSetPools[ FrameIndex ].GetDescSet( &descSet );
    dynamicOffsets[ 0 ] = suballocResult.dynamicOffset;

    vkCmdBindPipeline( pParams->pCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, hPipeline );
    vkCmdBindDescriptorSets( pParams->pCmdBuffer,
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             hPipelineLayout,
                             0,
                             GetArraySizeU( descriptorSet ),
                             descriptorSet,
                             GetArraySizeU( dynamicOffsets ),
                             dynamicOffsets );

#if 1
    VkBuffer     vertexBuffers[ 1 ] = {hVertexBuffer};
    VkDeviceSize vertexOffsets[ 1 ] = {0};
    vkCmdBindVertexBuffers( pParams->pCmdBuffer, 0, 1, vertexBuffers, vertexOffsets );
#endif

    VkViewport viewport;
    InitializeStruct( viewport );
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = pParams->Dims[ 0 ] * pParams->Scale[ 0 ];
    viewport.height   = pParams->Dims[ 1 ] * pParams->Scale[ 1 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( pParams->pCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor;
    InitializeStruct( scissor );
    scissor.offset.x      = 0;
    scissor.offset.y      = 0;
    scissor.extent.width  = ( uint32_t )( pParams->Dims[ 0 ] * pParams->Scale[ 0 ] );
    scissor.extent.height = ( uint32_t )( pParams->Dims[ 1 ] * pParams->Scale[ 1 ] );

    vkCmdSetScissor( pParams->pCmdBuffer, 0, 1, &scissor );
    vkCmdDraw( pParams->pCmdBuffer, 3, 1, 0, 0 );

    return true;
}

void apemodevk::SkyboxRenderer::Flush(uint32_t FrameIndex)
{
    BufferPools[ FrameIndex ].Flush( );
}
