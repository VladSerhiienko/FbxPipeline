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
        static void SdlClipboardPaste( nk_handle usr, struct nk_text_edit *edit );
        static void SdlClipboardCopy( nk_handle usr, const char *text, int len );

    public:
        enum Theme { Black, White, Red, Blue, Dark };
        enum Impl { kImpl_Unknown, kImpl_GL, kImpl_Vk };

        struct Vertex {
            float   pos [ 2 ];
            float   uv  [ 2 ];
            nk_byte col [ 4 ];
        };

        struct InitParametersBase {
            typedef void ( *NkClipbardPasteFn )( nk_handle, struct nk_text_edit * );
            typedef void ( *NkClipbardCopyFn )( nk_handle, const char *, int );
            NkClipbardPasteFn pClipboardPasteCallback = SdlClipboardPaste; /* Ok */
            NkClipbardCopyFn  pClipboardCopyCallback  = SdlClipboardCopy;  /* Ok */
        };

        struct UploadFontAtlasParametersBase {};

        struct RenderParametersBase {
            float            dims[ 2 ]          = {};                  /* Required */
            float            scale[ 2 ]         = {};                  /* Required */
            nk_anti_aliasing aa                 = NK_ANTI_ALIASING_ON; /* Ok */
            uint32_t         max_vertex_buffer  = 65536;               /* Ok */
            uint32_t         max_element_buffer = 65536;               /* Ok */
        };

    public:
        Impl                 Impl;
        nk_draw_null_texture NullTexture;
        nk_font *            pDefaultFont;
        nk_font_atlas        Atlas;
        nk_buffer            RenderCmds;
        nk_context           Context;

    public:
        void FontStashBegin( nk_font_atlas **atlas );
        bool FontStashEnd( InitParametersBase *init_params );
        int  HandleEvent( SDL_Event *evt );
        void Shutdown( );
        void SetStyle( Theme theme );

    public:
        virtual bool  Init( InitParametersBase *init_params );
        virtual bool  Render( RenderParametersBase *render_params );
        virtual void  DeviceDestroy( );
        virtual bool  DeviceCreate( InitParametersBase *init_params );
        virtual void *DeviceUploadAtlas( InitParametersBase *init_params, const void *image, int width, int height );
    };
}
