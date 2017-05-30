
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

    apemode::TInfoStruct< VkShaderModuleCreateInfo > vertexShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*) nuklear_compiled[ 0 ].begin( );
    vertexShaderCreateInfo->codeSize = (size_t) std::distance( nuklear_compiled[ 0 ].begin( ), nuklear_compiled[ 0 ].end( ) );

    apemode::TInfoStruct< VkShaderModuleCreateInfo > fragmentShaderCreateInfo;
    vertexShaderCreateInfo->pCode = (const uint32_t*) nuklear_compiled[ 1 ].begin( );
    vertexShaderCreateInfo->codeSize = (size_t) std::distance( nuklear_compiled[ 1 ].begin( ), nuklear_compiled[ 1 ].end( ) );

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

/*

        pDescSet = new DescriptorSet( );
        pDescSet->RecreateResourcesFor( *pDevice, *pDescPool, hDescSetLayout );

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = g_DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &g_DescriptorSetLayout;*/
    }
}

void* apemode::NuklearSdlVk::DeviceUploadAtlas( const void* image, int width, int height ) {
    return nullptr;
}
