#pragma once
#include <NuklearSdlBase.h>

namespace apemode {
    class NuklearSdlGL : public NuklearSdlBase {
        uint32_t vbo          = 0;
        uint32_t vao          = 0;
        uint32_t ebo          = 0;
        uint32_t prog         = 0;
        uint32_t vert_shdr    = 0;
        uint32_t frag_shdr    = 0;
        int32_t  attrib_pos   = 0;
        int32_t  attrib_uv    = 0;
        int32_t  attrib_col   = 0;
        int32_t  uniform_tex  = 0;
        int32_t  uniform_proj = 0;
        uint32_t font_tex     = 0;

    public:
        virtual bool  Render( RenderParametersBase *render_params ) override;
        virtual void  DeviceDestroy( ) override;
        virtual bool  DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( InitParametersBase *init_params, const void *image, int width, int height ) override;
    };
} // namespace apemode