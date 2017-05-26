
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsEcosystems.Vulkan.h>
#include <NuklearSdlVk.h>

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
        "} Out\n"
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

    if ( auto initParametersVk = (InitParametersVk*) init_params ) {
        Allocator      = initParametersVk->allocator;
        Gpu            = initParametersVk->gpu;
        Device         = initParametersVk->device;
        CheckVkResult  = initParametersVk->check_vk_result;
        DescriptorPool = initParametersVk->descriptor_pool;
        PipelineCache  = initParametersVk->pipeline_cache;
        RenderPass     = initParametersVk->render_pass;
    }
}

void* apemode::NuklearSdlVk::DeviceUploadAtlas( const void* image, int width, int height ) {
    return nullptr;
}
