/*
* Nuklear - 1.32.0 - public domain
* no warrenty implied; use at your own risk.
* authored from 2015-2016 by Micha Mettke
*/
/*
* ==============================================================
*
*                              API
*
* ===============================================================
*/

#include <fbxvpch.h>

#include <SDL.h>
#include <bx/fpumath.h>
//#include <SDL_opengles2.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>

enum theme { THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK };

NK_API struct nk_context *nk_sdl_init( SDL_Window *win );
NK_API void nk_sdl_font_stash_begin( struct nk_font_atlas **atlas );
NK_API void nk_sdl_font_stash_end( void );
NK_API int nk_sdl_handle_event( SDL_Event *evt );
NK_API void nk_sdl_render( enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer );
NK_API void nk_sdl_shutdown( void );
NK_API void nk_sdl_device_destroy( void );
NK_API void nk_sdl_device_create( void );
void nk_set_style( struct nk_context *ctx, enum theme theme );

namespace fbxv {
    void *Malloc( size_t bytes );
    void *Memalign( size_t alignment, size_t bytes );
    void Free( void *p );

    namespace bgfxUtils {
        void releaseRef( void *data, void *userData ) {
            fbxv::Free( data );
            (void) userData;
        }

        const bgfx::Memory *makeReleasableRef( const void *data, size_t dataSize ) {
            return bgfx::makeRef( data, dataSize, &releaseRef, nullptr );
        }
    }
}


void *nk_plugin_alloc_fbxvimpl( nk_handle, void *old, nk_size size ) {
    if ( old )
        fbxv::Free( old );
    return fbxv::Memalign( 16, size );
}
void nk_plugin_free_fbxvimpl( nk_handle, void *old ) {
    fbxv::Free( old );
}

/*
* ==============================================================
*
*                          IMPLEMENTATION
*
* ===============================================================
*/
#include <string.h>

struct nk_sdl_device {
    struct nk_buffer            cmds;
    struct nk_draw_null_texture null;

    bgfx::ShaderHandle              vertexShaderHandle;
    bgfx::ShaderHandle              fragmentShaderHandle;
    bgfx::ProgramHandle             textureProgramHandle;
    bgfx::TextureHandle             fontTextureHandle;
    bgfx::VertexDecl                vertexDecl;
    bgfx::VertexDeclHandle          vertexDeclHandle;
    bgfx::DynamicVertexBufferHandle vertexBufferHandle;
    bgfx::DynamicIndexBufferHandle  elementBufferHandle;
    bgfx::UniformHandle             textureUniformHandle;

    nk_allocator                  allocator;
    size_t                        maxVertexBufferSize;
    size_t                        maxIndexBufferSize;
    nk_draw_vertex_layout_element vertexLayout[ 4 ];
    nk_font *                     defaultFont;
    nk_buffer                     vertexBuffer, elementBuffer;

    GLuint   vbo, vao, ebo;
    GLuint   prog;
    GLuint   vert_shdr;
    GLuint   frag_shdr;
    GLint    attrib_pos;
    GLint    attrib_uv;
    GLint    attrib_col;
    GLint    uniform_tex;
    GLint    uniform_proj;
    GLuint   font_tex;
};

struct nk_sdl_vertex {
    float   position[ 2 ];
    float   uv[ 2 ];
    nk_byte col[ 4 ];
};

static struct nk_sdl {
    SDL_Window *         win;
    struct nk_sdl_device ogl;
    struct nk_context    ctx;
    struct nk_font_atlas atlas;
} sdl;

NK_API void nk_sdl_device_create( void ) {
    struct nk_sdl_device *dev = &sdl.ogl;
    nk_buffer_init_default( &dev->cmds );

#include <fs_nuklear_texture_android.bin.h>
#include <vs_nuklear_texture_android.bin.h>
#include <droidsans.ttf.h>

    dev->vertexShaderHandle = bgfx::createShader( bgfx::makeRef( (uint8_t *) fs_nuklear_texture_android_bin_h, sizeof( fs_nuklear_texture_android_bin_h ) ) );
    dev->fragmentShaderHandle = bgfx::createShader( bgfx::makeRef( (uint8_t *) fs_nuklear_texture_android_bin_h, sizeof( fs_nuklear_texture_android_bin_h ) ) );
    dev->textureProgramHandle = bgfx::createProgram( dev->vertexShaderHandle, dev->fragmentShaderHandle, false );

    dev->allocator.alloc     = &nk_plugin_alloc_fbxvimpl;
    dev->allocator.free      = &nk_plugin_free_fbxvimpl;
    dev->allocator.userdata  = nk_handle_ptr( &sdl );
    dev->maxVertexBufferSize = 512 * 1024;
    dev->maxIndexBufferSize  = 128 * 1024;

    dev->vertexDecl.begin( )
        .add( bgfx::Attrib::Position, 2, bgfx::AttribType::Float, false, false )
        .add( bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float, false, false )
        .add( bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false )
        .end( );

    dev->textureUniformHandle = bgfx::createUniform( "s_texColor", bgfx::UniformType::Int1 );

    const size_t vs = sizeof( struct nk_sdl_vertex );
    const size_t vp = offsetof( struct nk_sdl_vertex, position );
    const size_t vt = offsetof( struct nk_sdl_vertex, uv );
    const size_t vc = offsetof( struct nk_sdl_vertex, col );
    assert( dev->vertexDecl.getOffset( bgfx::Attrib::Position ) == vp );
    assert( dev->vertexDecl.getOffset( bgfx::Attrib::TexCoord0 ) == vt );
    assert( dev->vertexDecl.getOffset( bgfx::Attrib::Color0 ) == vc );

    static const nk_draw_vertex_layout_element vertexLayout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, dev->vertexDecl.getOffset( bgfx::Attrib::Position )},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, dev->vertexDecl.getOffset( bgfx::Attrib::TexCoord0 )},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, dev->vertexDecl.getOffset( bgfx::Attrib::Color0 )},
        {NK_VERTEX_LAYOUT_END}};

    static_assert( sizeof( dev->vertexLayout ) == sizeof( vertexLayout ), "Size mismatch." );
    memcpy( dev->vertexLayout, vertexLayout, sizeof( vertexLayout ) );

    dev->vertexBufferHandle = bgfx::createDynamicVertexBuffer( dev->maxVertexBufferSize, dev->vertexDecl, BGFX_BUFFER_ALLOW_RESIZE );
    dev->elementBufferHandle = bgfx::createDynamicIndexBuffer( dev->maxIndexBufferSize, BGFX_BUFFER_ALLOW_RESIZE );

    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin( &atlas );
    dev->defaultFont = nk_font_atlas_add_from_memory( atlas, (void *) s_droidSansTtf, sizeof( s_droidSansTtf ), 14, 0 );
    nk_sdl_font_stash_end( );

    nk_set_style( &sdl.ctx, theme::THEME_WHITE );
    sdl.ctx.style.font = &dev->defaultFont->handle;
    sdl.atlas.default_font = dev->defaultFont;
    nk_style_set_font( &sdl.ctx, &dev->defaultFont->handle );

    nk_buffer_init( &sdl.ogl.vertexBuffer, &dev->allocator, dev->maxVertexBufferSize );
    nk_buffer_init( &sdl.ogl.elementBuffer, &dev->allocator, dev->maxIndexBufferSize );

#if 0
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct nk_sdl_vertex);
        size_t vp = offsetof(struct nk_sdl_vertex, position);
        size_t vt = offsetof(struct nk_sdl_vertex, uv);
        size_t vc = offsetof(struct nk_sdl_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

NK_INTERN void nk_sdl_device_upload_atlas( const void *image, int width, int height ) {
    struct nk_sdl_device *dev = &sdl.ogl;

    // TODO: Linear samping? Is it default? Cannot see correspondent flags.
    void *imageCopyData = fbxv::Malloc( width * height * 4 );
    memcpy( imageCopyData, image, width * height * 4 );
    auto imageCopyRef = fbxv::bgfxUtils::makeReleasableRef( imageCopyData, width * height * 4 );
    dev->fontTextureHandle = bgfx::createTexture2D( width, height, false, 1, bgfx::TextureFormat::RGBA8, 0, imageCopyRef );

#if 0
    glGenTextures( 1, &dev->font_tex );
    glBindTexture( GL_TEXTURE_2D, dev->font_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) width, (GLsizei) height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
#endif
}

NK_API void nk_sdl_device_destroy( void ) {
    struct nk_sdl_device *dev = &sdl.ogl;

    bgfx::destroyTexture( dev->fontTextureHandle );
    bgfx::destroyShader( dev->vertexShaderHandle );
    bgfx::destroyShader( dev->fragmentShaderHandle );
    bgfx::destroyProgram( dev->textureProgramHandle );

#if 0
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
#endif

    nk_buffer_free(&dev->cmds);
}

NK_API void nk_sdl_render( enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer ) {
    struct nk_sdl_device *dev = &sdl.ogl;

    int width, height;
    SDL_GetWindowSize( sdl.win, &width, &height );

    int display_width, display_height;
    SDL_GL_GetDrawableSize( sdl.win, &display_width, &display_height );
    
    float ortho[ 4 ][ 4 ] = {
        { 2.0f,  0.0f,  0.0f, 0.0f},
        { 0.0f, -2.0f,  0.0f, 0.0f},
        { 0.0f,  0.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f,  0.0f, 1.0f},
    };

    ortho[ 0 ][ 0 ] /= (float) width;
    ortho[ 1 ][ 1 ] /= (float) height;

    float at[ 3 ]  = {0, 0, 0.0f};
    float eye[ 3 ] = {0, 0, -1.0f};
    float view[ 16 ];
    bx::mtxLookAt( view, eye, at );

    struct nk_vec2 scale;
    scale.x = (float) display_width / (float) width;
    scale.y = (float) display_height / (float) height;

    bgfx::setViewRect( 0, 0, 0, display_width, display_height );
    bgfx::setState( BGFX_STATE_RGB_WRITE | BGFX_STATE_ALPHA_WRITE | BGFX_STATE_BLEND_EQUATION_ADD |
                    BGFX_STATE_BLEND_FUNC( BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA ) );

    nk_convert_config config    = {0};
    config.vertex_layout        = dev->vertexLayout;
    config.vertex_size          = sizeof( nk_sdl_vertex );
    config.vertex_alignment     = NK_ALIGNOF( nk_sdl_vertex );
    config.null                 = dev->null;
    config.circle_segment_count = 22;
    config.curve_segment_count  = 22;
    config.arc_segment_count    = 22;
    config.global_alpha         = 1.0f;
    config.shape_AA             = AA;
    config.line_AA              = AA;

    nk_buffer_init( &sdl.ogl.vertexBuffer, &dev->allocator, dev->maxVertexBufferSize );
    nk_buffer_init( &sdl.ogl.elementBuffer, &dev->allocator, dev->maxIndexBufferSize );
    nk_convert( &sdl.ctx, &dev->cmds, &sdl.ogl.vertexBuffer, &sdl.ogl.elementBuffer, &config );

    bgfx::updateDynamicVertexBuffer(dev->vertexBufferHandle, 0, fbxv::bgfxUtils::makeReleasableRef( sdl.ogl.vertexBuffer.memory.ptr, sdl.ogl.vertexBuffer.allocated ) );
    bgfx::updateDynamicIndexBuffer(dev->elementBufferHandle, 0, fbxv::bgfxUtils::makeReleasableRef( sdl.ogl.elementBuffer.memory.ptr, sdl.ogl.elementBuffer.allocated ) );

    const nk_draw_command *cmd    = nullptr;
    uint32_t               offset = 0;

    uint32_t drawcallCount = 0;

    nk_draw_foreach( cmd, &sdl.ctx, &dev->cmds ) {
        if ( !cmd->elem_count )
            continue;

        bgfx::setScissor( ( GLint )( cmd->clip_rect.x * scale.x ),
                          ( GLint )( ( height - ( GLint )( cmd->clip_rect.y + cmd->clip_rect.h ) ) * scale.y ),
                          ( GLint )( cmd->clip_rect.w * scale.x ),
                          ( GLint )( cmd->clip_rect.h * scale.y ) );

        // TODO: How to bind textures?
        auto &textureHandle = *(bgfx::TextureHandle *) cmd->texture.ptr;
        bgfx::setTexture( 0, dev->textureUniformHandle, textureHandle );

        bgfx::setViewTransform( 0, view, ortho );
        bgfx::setVertexBuffer( dev->vertexBufferHandle );
        bgfx::setIndexBuffer( dev->elementBufferHandle, offset, cmd->elem_count );
        drawcallCount += bgfx::submit( 0, dev->textureProgramHandle );

        offset += cmd->elem_count;
    }

    nk_clear( &sdl.ctx );

#if 0

    /* setup global state */
    glViewport(0,0,display_width,display_height);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load vertices/elements directly into vertex/element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sdl_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_sdl_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            {struct nk_buffer vertexBuffer, elementBuffer;
            nk_buffer_init_fixed(&vertexBuffer, vertices, (nk_size)max_vertex_buffer);
            nk_buffer_init_fixed(&elementBuffer, elements, (nk_size)max_element_buffer);
            nk_convert(&sdl.ctx, &dev->cmds, &vertexBuffer, &elementBuffer, &config);}
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &sdl.ctx, &dev->cmds) {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)(cmd->clip_rect.x * scale.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
                (GLint)(cmd->clip_rect.w * scale.x),
                (GLint)(cmd->clip_rect.h * scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&sdl.ctx);
    }

    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
#endif
}

static void nk_sdl_clipbard_paste( nk_handle usr, struct nk_text_edit *edit ) {
    const char *text = SDL_GetClipboardText( );
    if ( text )
        nk_textedit_paste( edit, text, nk_strlen( text ) );
    (void) usr;
}

static void nk_sdl_clipbard_copy( nk_handle usr, const char *text, int len ) {
    char *str = 0;
    (void) usr;
    if ( !len )
        return;
    str = (char *) fbxv::Malloc( (size_t) len + 1 );
    if ( !str )
        return;
    memcpy( str, text, (size_t) len );
    str[ len ] = '\0';
    SDL_SetClipboardText( str );
    fbxv::Free( str );
}

NK_API struct nk_context *nk_sdl_init( SDL_Window *win ) {
    sdl.win = win;
    nk_init_default( &sdl.ctx, 0 );
    sdl.ctx.clip.copy           = nk_sdl_clipbard_copy;
    sdl.ctx.clip.paste          = nk_sdl_clipbard_paste;
    sdl.ctx.clip.userdata       = nk_handle_ptr( 0 );
    sdl.ctx.pool.alloc.alloc    = &nk_plugin_alloc_fbxvimpl;
    sdl.ctx.pool.alloc.free     = &nk_plugin_free_fbxvimpl;
    sdl.ctx.pool.alloc.userdata = nk_handle_ptr( &sdl );
    nk_sdl_device_create( );
    return &sdl.ctx;
}

NK_API void nk_sdl_font_stash_begin( struct nk_font_atlas **atlas ) {
    nk_font_atlas_init_default( &sdl.atlas );
    nk_font_atlas_begin( &sdl.atlas );
    *atlas = &sdl.atlas;
}

NK_API void nk_sdl_font_stash_end( void ) {
    const void *image;
    int         w, h;
    image = nk_font_atlas_bake( &sdl.atlas, &w, &h, NK_FONT_ATLAS_RGBA32 );
    nk_sdl_device_upload_atlas( image, w, h );
    nk_font_atlas_end( &sdl.atlas, nk_handle_ptr( &sdl.ogl.fontTextureHandle ), &sdl.ogl.null );
    if ( sdl.atlas.default_font )
        nk_style_set_font( &sdl.ctx, &sdl.atlas.default_font->handle );
}

NK_API int nk_sdl_handle_event( SDL_Event *evt ) {
    struct nk_context *ctx = &sdl.ctx;
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
        if ( evt->button.button == SDL_BUTTON_LEFT )
            nk_input_button( ctx, NK_BUTTON_LEFT, x, y, down );
        if ( evt->button.button == SDL_BUTTON_MIDDLE )
            nk_input_button( ctx, NK_BUTTON_MIDDLE, x, y, down );
        if ( evt->button.button == SDL_BUTTON_RIGHT )
            nk_input_button( ctx, NK_BUTTON_RIGHT, x, y, down );
        return 1;
    } else if ( evt->type == SDL_MOUSEMOTION ) {
        if ( ctx->input.mouse.grabbed ) {
            int x = (int) ctx->input.mouse.prev.x, y = (int) ctx->input.mouse.prev.y;
            nk_input_motion( ctx, x + evt->motion.xrel, y + evt->motion.yrel );
        } else
            nk_input_motion( ctx, evt->motion.x, evt->motion.y );
        return 1;
    } else if ( evt->type == SDL_TEXTINPUT ) {
        nk_glyph glyph;
        memcpy( glyph, evt->text.text, NK_UTF_SIZE );
        nk_input_glyph( ctx, glyph );
        return 1;
    } else if ( evt->type == SDL_MOUSEWHEEL ) {
        nk_input_scroll( ctx, (float) evt->wheel.y );
        return 1;
    }
    return 0;
}

NK_API
void nk_sdl_shutdown( void ) {
    nk_font_atlas_clear( &sdl.atlas );
    nk_free( &sdl.ctx );
    nk_sdl_device_destroy( );
    memset( &sdl, 0, sizeof( sdl ) );
}

void nk_set_style( struct nk_context *ctx, enum theme theme ) {
    struct nk_color table[ NK_COLOR_COUNT ];
    if ( theme == THEME_WHITE ) {
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
    } else if ( theme == THEME_RED ) {
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
    } else if ( theme == THEME_BLUE ) {
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
    } else if ( theme == THEME_DARK ) {
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