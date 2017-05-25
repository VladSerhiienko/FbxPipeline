
#include <NuklearSdlGL.h>

#include <GL/glew.h>
#include <assert.h>
#include <string.h>

struct RenderAssets {
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
};

// TODO: Correct version

#ifdef __APPLE__
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif

void apemode::NuklearSdlGL::Render( nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer ) {
    if ( auto dev = (RenderAssets *) pRenderAssets ) {

        int width;
        int height;
        SDL_GetWindowSize( (SDL_Window *) pSurface->GetWindowHandle( ), &width, &height );

        int display_width;
        int display_height;
        SDL_GL_GetDrawableSize( (SDL_Window *) pSurface->GetWindowHandle( ), &display_width, &display_height );

        const GLfloat ortho[ 4 ][ 4 ] = {
            {2.0f / (GLfloat) width, 0.0f, 0.0f, 0.0f},
            {0.0f, -2.0f / (GLfloat) height, 0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f},
        };

        struct nk_vec2 scale;
        scale.x = (float) display_width / (float) width;
        scale.y = (float) display_height / (float) height;

        /* setup global state */
        glViewport( 0, 0, display_width, display_height );
        glEnable( GL_BLEND );
        glBlendEquation( GL_FUNC_ADD );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable( GL_CULL_FACE );
        glDisable( GL_DEPTH_TEST );
        glEnable( GL_SCISSOR_TEST );
        glActiveTexture( GL_TEXTURE0 );

        /* setup program */
        glUseProgram( dev->prog );
        glUniform1i( dev->uniform_tex, 0 );
        glUniformMatrix4fv( dev->uniform_proj, 1, GL_FALSE, &ortho[ 0 ][ 0 ] );
        {
            /* convert from command queue into draw list and draw to screen */
            const nk_draw_command *cmd      = nullptr;
            void *                 vertices = nullptr;
            void *                 elements = nullptr;
            const nk_draw_index *  offset   = nullptr;

            /* allocate vertex and element buffer */
            glBindVertexArray( dev->vao );
            glBindBuffer( GL_ARRAY_BUFFER, dev->vbo );
            glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, dev->ebo );

            glBufferData( GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW );
            glBufferData( GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW );

            /* load vertices/elements directly into vertex/element buffer */
            vertices = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
            elements = glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY );
            {
                /* fill convert configuration */
                struct nk_convert_config config;
                static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, position )},
                    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF( Vertex, uv )},
                    {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF( Vertex, col )},
                    {NK_VERTEX_LAYOUT_END}};

                memset( &config, 0, sizeof( config ) );
                config.vertex_layout        = vertex_layout;
                config.vertex_size          = sizeof( Vertex );
                config.vertex_alignment     = NK_ALIGNOF( Vertex );
                config.null                 = NullTexture;
                config.circle_segment_count = 22;
                config.curve_segment_count  = 22;
                config.arc_segment_count    = 22;
                config.global_alpha         = 1.0f;
                config.shape_AA             = AA;
                config.line_AA              = AA;

                /* setup buffers to load vertices and elements */
                {
                    nk_buffer vbuf, ebuf;
                    nk_buffer_init_fixed( &vbuf, vertices, (nk_size) max_vertex_buffer );
                    nk_buffer_init_fixed( &ebuf, elements, (nk_size) max_element_buffer );
                    nk_convert( &Context, &RenderCmds, &vbuf, &ebuf, &config );
                }
            }

            glUnmapBuffer( GL_ARRAY_BUFFER );
            glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );

            /* iterate over and execute each draw command */
            nk_draw_foreach( cmd, &Context, &RenderCmds ) {
                if ( !cmd->elem_count )
                    continue;
                glBindTexture( GL_TEXTURE_2D, (GLuint) cmd->texture.id );
                glScissor( ( GLint )( cmd->clip_rect.x * scale.x ),
                           ( GLint )( ( height - ( GLint )( cmd->clip_rect.y + cmd->clip_rect.h ) ) * scale.y ),
                           ( GLint )( cmd->clip_rect.w * scale.x ),
                           ( GLint )( cmd->clip_rect.h * scale.y ) );
                glDrawElements( GL_TRIANGLES, (GLsizei) cmd->elem_count, GL_UNSIGNED_SHORT, offset );
                offset += cmd->elem_count;
            }

            nk_clear( &Context );
        }

        glUseProgram( 0 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindVertexArray( 0 );
        glDisable( GL_BLEND );
        glDisable( GL_SCISSOR_TEST );
    }
}

void apemode::NuklearSdlGL::DeviceDestroy( ) {
    if ( auto dev = (RenderAssets *) pRenderAssets ) {
        glDetachShader( dev->prog, dev->vert_shdr );
        glDetachShader( dev->prog, dev->frag_shdr );
        glDeleteShader( dev->vert_shdr );
        glDeleteShader( dev->frag_shdr );
        glDeleteProgram( dev->prog );
        glDeleteTextures( 1, &dev->font_tex );
        glDeleteBuffers( 1, &dev->vbo );
        glDeleteBuffers( 1, &dev->ebo );
    }
}

void apemode::NuklearSdlGL::DeviceCreate( ) {
    if ( nullptr == pRenderAssets )
        pRenderAssets = new RenderAssets( );
    auto dev = (RenderAssets *) pRenderAssets;

    const GLchar *vertex_shader = NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 TexCoord;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";

    const GLchar *fragment_shader = NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    GLint status;
    nk_buffer_init_default( &RenderCmds );
    dev->prog      = glCreateProgram( );
    dev->vert_shdr = glCreateShader( GL_VERTEX_SHADER );
    dev->frag_shdr = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( dev->vert_shdr, 1, &vertex_shader, 0 );
    glShaderSource( dev->frag_shdr, 1, &fragment_shader, 0 );
    glCompileShader( dev->vert_shdr );
    glCompileShader( dev->frag_shdr );
    glGetShaderiv( dev->vert_shdr, GL_COMPILE_STATUS, &status );
    assert( status == GL_TRUE );
    glGetShaderiv( dev->frag_shdr, GL_COMPILE_STATUS, &status );
    assert( status == GL_TRUE );
    glAttachShader( dev->prog, dev->vert_shdr );
    glAttachShader( dev->prog, dev->frag_shdr );
    glLinkProgram( dev->prog );
    glGetProgramiv( dev->prog, GL_LINK_STATUS, &status );
    assert( status == GL_TRUE );

    dev->uniform_tex  = glGetUniformLocation( dev->prog, "Texture" );
    dev->uniform_proj = glGetUniformLocation( dev->prog, "ProjMtx" );
    dev->attrib_pos   = glGetAttribLocation( dev->prog, "Position" );
    dev->attrib_uv    = glGetAttribLocation( dev->prog, "TexCoord" );
    dev->attrib_col   = glGetAttribLocation( dev->prog, "Color" );

    GLsizei vs = sizeof( Vertex );
    size_t  vp = offsetof( Vertex, position );
    size_t  vt = offsetof( Vertex, uv );
    size_t  vc = offsetof( Vertex, col );

    glGenBuffers( 1, &dev->vbo );
    glGenBuffers( 1, &dev->ebo );
    glGenVertexArrays( 1, &dev->vao );

    glBindVertexArray( dev->vao );
    glBindBuffer( GL_ARRAY_BUFFER, dev->vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, dev->ebo );

    glEnableVertexAttribArray( (GLuint) dev->attrib_pos );
    glEnableVertexAttribArray( (GLuint) dev->attrib_uv );
    glEnableVertexAttribArray( (GLuint) dev->attrib_col );

    glVertexAttribPointer( (GLuint) dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *) vp );
    glVertexAttribPointer( (GLuint) dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *) vt );
    glVertexAttribPointer( (GLuint) dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *) vc );

    glBindTexture( GL_TEXTURE_2D, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
}

void *apemode::NuklearSdlGL::DeviceUploadAtlas( const void *image, int width, int height ) {
    if ( auto dev = (RenderAssets *) pRenderAssets ) {
        glGenTextures( 1, &dev->font_tex );
        glBindTexture( GL_TEXTURE_2D, dev->font_tex );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) width, (GLsizei) height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
        return (void *) dev->font_tex;
    }
}
