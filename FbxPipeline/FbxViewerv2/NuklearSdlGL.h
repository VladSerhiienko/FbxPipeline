#pragma once
#include <NuklearSdlBase.h>

namespace apemode {
    class NuklearSdlGL : public NuklearSdlBase {
        GLuint vbo          = 0;
        GLuint vao          = 0;
        GLuint ebo          = 0;
        GLuint prog         = 0;
        GLuint vert_shdr    = 0;
        GLuint frag_shdr    = 0;
        GLint  attrib_pos   = 0;
        GLint  attrib_uv    = 0;
        GLint  attrib_col   = 0;
        GLint  uniform_tex  = 0;
        GLint  uniform_proj = 0;
        GLuint font_tex     = 0;

    public:
        virtual void Render( RenderParametersBase *render_params ) override;
        virtual void DeviceDestroy( ) override;
        virtual void DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( const void *image, int width, int height ) override;
    };
} // namespace apemode