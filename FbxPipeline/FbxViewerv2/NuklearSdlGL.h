#pragma once
#include <NuklearSdlBase.h>

namespace apemode {
    class NuklearSdlGL : public NuklearSdlBase {
    public:
        virtual void  Render( nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer ) override;
        virtual void  DeviceDestroy( ) override;
        virtual void  DeviceCreate( ) override;
        virtual void *DeviceUploadAtlas( const void *image, int width, int height ) override;
    };
} // namespace apemode