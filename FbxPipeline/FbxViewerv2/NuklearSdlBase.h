#pragma once

#include <IAppSurface.h>
#include <SDL.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include <nuklear.h>

namespace apemode {
    class NuklearSdlBase {
    public:
        enum Theme { Black, White, Red, Blue, Dark };

        enum Impl {
            kImpl_Unknown,
            kImpl_GL,
            kImpl_Vk
        };

        struct Vertex {
            float   position[ 2 ];
            float   uv[ 2 ];
            nk_byte col[ 4 ];
        };

        struct InitParametersBase {};

        struct RenderParametersBase {
            float            dims[ 2 ]          = {};
            float            scale[ 2 ]         = {};
            nk_anti_aliasing aa                 = NK_ANTI_ALIASING_ON;
            int              max_vertex_buffer  = 65536;
            int              max_element_buffer = 65536;
        };

    public:
        Impl                 Impl;
        nk_context           Context;
        nk_draw_null_texture NullTexture;
        nk_font *            pDefaultFont;
        nk_font_atlas        Atlas;
        nk_buffer            RenderCmds;

    public:
        void FontStashBegin( nk_font_atlas **atlas );
        void FontStashEnd( );
        int HandleEvent( SDL_Event *evt );
        void Shutdown( );
        void SetStyle( Theme theme );

    public:
        virtual nk_context *Init( InitParametersBase *init_params );
        virtual void Render( RenderParametersBase *render_params );
        virtual void DeviceDestroy( );
        virtual void DeviceCreate( InitParametersBase *init_params );
        virtual void *DeviceUploadAtlas( const void *image, int width, int height );

    public:
        static void SdlClipbardPaste( nk_handle usr, struct nk_text_edit *edit );
        static void SdlClipbardCopy( nk_handle usr, const char *text, int len );
    };
}
