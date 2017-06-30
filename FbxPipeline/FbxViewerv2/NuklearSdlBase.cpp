#define NK_IMPLEMENTATION
#include <NuklearSdlBase.h>
#include <memory>

void apemode::NuklearRendererSdlBase::SdlClipboardPaste( nk_handle usr, struct nk_text_edit *edit ) {
    (void) usr;

    if ( const char *text = SDL_GetClipboardText( ) )
        nk_textedit_paste( edit, text, nk_strlen( text ) );
}

void apemode::NuklearRendererSdlBase::SdlClipboardCopy( nk_handle usr, const char *text, int len ) {
    (void) usr;

    if ( !len )
        return;

    if ( char *str = (char *) malloc( (size_t) len + 1 ) ) {
        memcpy( str, text, (size_t) len );
        str[ len ] = '\0';
        SDL_SetClipboardText( str );
        free( str );
    }
}

void *apemode::NuklearRendererSdlBase::DeviceUploadAtlas( InitParametersBase *initParamsBase,
                                                          const void *        image,
                                                          int                 width,
                                                          int                 height ) {
    (void) initParamsBase;
    (void) image;
    (void) width;
    (void) height;

    return nullptr;
}

bool apemode::NuklearRendererSdlBase::Init( InitParametersBase *initParamsBase ) {
    nk_init_default( &Context, nullptr /* User font */ );
    nk_buffer_init_default( &RenderCmds );
    Context.clip.copy     = initParamsBase->pClipboardCopyCallback;
    Context.clip.paste    = initParamsBase->pClipboardPasteCallback;
    Context.clip.userdata = nk_handle_ptr( this );

    /* Overrided */
    DeviceCreate( initParamsBase );

#include <droidsans.ttf.h>

    nk_font_atlas *atlas = nullptr;
    FontStashBegin( &atlas );
    pDefaultFont = nk_font_atlas_add_from_memory( atlas, (void *) s_droidSansTtf, sizeof( s_droidSansTtf ), 14, 0 );

    /* Calls overrided DeviceUploadAtlas(). */
    FontStashEnd( initParamsBase );

    SetStyle( Dark );
    Context.style.font = &pDefaultFont->handle;
    Atlas.default_font = pDefaultFont;
    nk_style_set_font( &Context, &pDefaultFont->handle );

    return true;
}

void apemode::NuklearRendererSdlBase::FontStashBegin( nk_font_atlas **atlas ) {
    nk_font_atlas_init_default( &Atlas );
    nk_font_atlas_begin( &Atlas );
    *atlas = &Atlas;
}

bool apemode::NuklearRendererSdlBase::FontStashEnd( InitParametersBase *initParamsBase ) {
    int imageWidth  = 0;
    int imageHeight = 0;

    if ( const void *imageData = nk_font_atlas_bake( &Atlas, &imageWidth, &imageHeight, NK_FONT_ATLAS_RGBA32 ) ) {
        assert( imageWidth && imageHeight && "Invalid font image dimensions." );

        /* Overrided */
        if ( auto atlasHandle = DeviceUploadAtlas( initParamsBase, imageData, imageWidth, imageHeight ) ) {
            nk_font_atlas_end( &Atlas, nk_handle_ptr( atlasHandle ), &NullTexture );

            if ( pDefaultFont )
                nk_style_set_font( &Context, &Atlas.default_font->handle );

            return true;
        }
    }

    return false;
}

bool apemode::NuklearRendererSdlBase::Render( RenderParametersBase *renderParamsBase ) {
    (void) renderParamsBase;
    return true;
}

void apemode::NuklearRendererSdlBase::Shutdown( ) {
    nk_font_atlas_clear( &Atlas );
    nk_free( &Context );

    /* Overrided */
    DeviceDestroy( );

    nk_buffer_free( &RenderCmds );
}

void apemode::NuklearRendererSdlBase::DeviceDestroy( ) {
}

bool apemode::NuklearRendererSdlBase::DeviceCreate( InitParametersBase *initParamsBase ) {
    (void) initParamsBase;
    return true;
}

void apemode::NuklearRendererSdlBase::SetStyle( Theme theme ) {
    auto            ctx = &Context;
    struct nk_color table[ NK_COLOR_COUNT ];
    if ( theme == White ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 70, 70, 70, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 0, 0, 0, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 185, 185, 185, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 170, 170, 170, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 120, 120, 120, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 80, 80, 80, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 70, 70, 70, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 60, 60, 60, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 0, 0, 0, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 45, 45, 45, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 180, 180, 180, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 140, 140, 140, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 180, 180, 180, 255 );
        nk_style_from_table( ctx, table );
    } else if ( theme == Red ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 30, 33, 40, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 181, 45, 69, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 190, 50, 70, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 195, 55, 75, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 45, 60, 60, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 186, 50, 74, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 191, 55, 79, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 51, 55, 67, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 170, 40, 60, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 30, 33, 40, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 64, 84, 95, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 70, 90, 100, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 75, 95, 105, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 181, 45, 69, 220 );
        nk_style_from_table( ctx, table );
    } else if ( theme == Blue ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 20, 20, 20, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 202, 212, 214, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 137, 182, 224, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 140, 159, 173, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 142, 187, 229, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 147, 192, 234, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 182, 215, 215, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 137, 182, 224, 245 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 142, 188, 229, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 147, 193, 234, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 210, 210, 210, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 20, 20, 20, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 190, 200, 200, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 64, 84, 95, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 70, 90, 100, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 75, 95, 105, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 156, 193, 220, 255 );
        nk_style_from_table( ctx, table );
    } else if ( theme == Dark ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 57, 67, 71, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 51, 51, 56, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 46, 46, 46, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 63, 98, 126, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 45, 53, 56, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 57, 67, 61, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 48, 83, 111, 245 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 53, 88, 116, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 50, 58, 61, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 53, 88, 116, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 48, 83, 111, 255 );
        nk_style_from_table( ctx, table );
    } else {
        nk_style_default( ctx );
    }
}

int apemode::NuklearRendererSdlBase::HandleEvent( SDL_Event *evt ) {
    struct nk_context *ctx = &Context;
    if ( evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN ) {
        /* key events */
        int          down  = evt->type == SDL_KEYDOWN;
        const Uint8 *state = SDL_GetKeyboardState( 0 );
        SDL_Keycode  sym   = evt->key.keysym.sym;
        if ( sym == SDLK_RSHIFT || sym == SDLK_LSHIFT )
            nk_input_key( ctx, NK_KEY_SHIFT, down );
        else if ( sym == SDLK_DELETE )
            nk_input_key( ctx, NK_KEY_DEL, down );
        else if ( sym == SDLK_RETURN )
            nk_input_key( ctx, NK_KEY_ENTER, down );
        else if ( sym == SDLK_TAB )
            nk_input_key( ctx, NK_KEY_TAB, down );
        else if ( sym == SDLK_BACKSPACE )
            nk_input_key( ctx, NK_KEY_BACKSPACE, down );
        else if ( sym == SDLK_HOME ) {
            nk_input_key( ctx, NK_KEY_TEXT_START, down );
            nk_input_key( ctx, NK_KEY_SCROLL_START, down );
        } else if ( sym == SDLK_END ) {
            nk_input_key( ctx, NK_KEY_TEXT_END, down );
            nk_input_key( ctx, NK_KEY_SCROLL_END, down );
        } else if ( sym == SDLK_PAGEDOWN ) {
            nk_input_key( ctx, NK_KEY_SCROLL_DOWN, down );
        } else if ( sym == SDLK_PAGEUP ) {
            nk_input_key( ctx, NK_KEY_SCROLL_UP, down );
        } else if ( sym == SDLK_z )
            nk_input_key( ctx, NK_KEY_TEXT_UNDO, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_r )
            nk_input_key( ctx, NK_KEY_TEXT_REDO, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_c )
            nk_input_key( ctx, NK_KEY_COPY, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_v )
            nk_input_key( ctx, NK_KEY_PASTE, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_x )
            nk_input_key( ctx, NK_KEY_CUT, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_b )
            nk_input_key( ctx, NK_KEY_TEXT_LINE_START, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_e )
            nk_input_key( ctx, NK_KEY_TEXT_LINE_END, down && state[ SDL_SCANCODE_LCTRL ] );
        else if ( sym == SDLK_UP )
            nk_input_key( ctx, NK_KEY_UP, down );
        else if ( sym == SDLK_DOWN )
            nk_input_key( ctx, NK_KEY_DOWN, down );
        else if ( sym == SDLK_LEFT ) {
            if ( state[ SDL_SCANCODE_LCTRL ] )
                nk_input_key( ctx, NK_KEY_TEXT_WORD_LEFT, down );
            else
                nk_input_key( ctx, NK_KEY_LEFT, down );
        } else if ( sym == SDLK_RIGHT ) {
            if ( state[ SDL_SCANCODE_LCTRL ] )
                nk_input_key( ctx, NK_KEY_TEXT_WORD_RIGHT, down );
            else
                nk_input_key( ctx, NK_KEY_RIGHT, down );
        } else
            return 0;
        return 1;
    } else if ( evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP ) {
        /* mouse button */
        int       down = evt->type == SDL_MOUSEBUTTONDOWN;
        const int x = evt->button.x, y = evt->button.y;
        if ( evt->button.button == SDL_BUTTON_LEFT ) {
            // if (evt->button.clicks > 1)
            // nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button( ctx, NK_BUTTON_LEFT, x, y, down );
        } else if ( evt->button.button == SDL_BUTTON_MIDDLE )
            nk_input_button( ctx, NK_BUTTON_MIDDLE, x, y, down );
        else if ( evt->button.button == SDL_BUTTON_RIGHT )
            nk_input_button( ctx, NK_BUTTON_RIGHT, x, y, down );
        return 1;
    } else if ( evt->type == SDL_MOUSEMOTION ) {
        /* mouse motion */
        if ( ctx->input.mouse.grabbed ) {
            int x = (int) ctx->input.mouse.prev.x, y = (int) ctx->input.mouse.prev.y;
            nk_input_motion( ctx, x + evt->motion.xrel, y + evt->motion.yrel );
        } else
            nk_input_motion( ctx, evt->motion.x, evt->motion.y );
        return 1;
    } else if ( evt->type == SDL_TEXTINPUT ) {
        /* text input */
        nk_glyph glyph;
        memcpy( glyph, evt->text.text, NK_UTF_SIZE );
        nk_input_glyph( ctx, glyph );
        return 1;
    } else if ( evt->type == SDL_MOUSEWHEEL ) {
        /* mouse wheel */
        nk_input_scroll( ctx, nk_vec2( (float) evt->wheel.x, (float) evt->wheel.y ) );
        return 1;
    }
    return 0;
}