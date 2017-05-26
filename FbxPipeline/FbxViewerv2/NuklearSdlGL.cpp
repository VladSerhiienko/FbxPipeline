
#include <NuklearSdlGL.h>

#include <GL/glew.h>
#include <assert.h>
#include <string.h>

// TODO: Correct version
#ifdef __APPLE__
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif

void apemode::NuklearSdlGL::Render( RenderParametersBase *p ) {
    const GLfloat ortho[ 4 ][ 4 ] = {
        {2.0f / (GLfloat) p->dims[ 0 ], 0.0f, 0.0f, 0.0f},
        {0.0f, -2.0f / (GLfloat) p->dims[ 1 ], 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };

    /* setup global state */
    glViewport( 0, 0, p->dims[ 0 ] * p->scale[ 0 ], p->dims[ 1 ] * p->scale[ 1 ] );
    glEnable( GL_BLEND );
    glBlendEquation( GL_FUNC_ADD );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable( GL_CULL_FACE );
    glDisable( GL_DEPTH_TEST );
    glEnable( GL_SCISSOR_TEST );
    glActiveTexture( GL_TEXTURE0 );

    /* setup program */
    glUseProgram( prog );
    glUniform1i( uniform_tex, 0 );
    glUniformMatrix4fv( uniform_proj, 1, GL_FALSE, &ortho[ 0 ][ 0 ] );
    {
        /* convert from command queue into draw list and draw to screen */
        const nk_draw_command *cmd      = nullptr;
        void *                 vertices = nullptr;
        void *                 elements = nullptr;
        const nk_draw_index *  offset   = nullptr;

        /* allocate vertex and element buffer */
        glBindVertexArray( vao );
        glBindBuffer( GL_ARRAY_BUFFER, vbo );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );

        glBufferData( GL_ARRAY_BUFFER, p->max_vertex_buffer, NULL, GL_STREAM_DRAW );
        glBufferData( GL_ELEMENT_ARRAY_BUFFER, p->max_element_buffer, NULL, GL_STREAM_DRAW );

        /* load vertices/elements directly into vertex/element buffer */
        vertices = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
        elements = glMapBuffer( GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY );
        {
            /* fill convert configuration */
            struct nk_convert_config                          config;
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
            config.shape_AA             = p->aa;
            config.line_AA              = p->aa;

            /* setup buffers to load vertices and elements */
            {
                nk_buffer vbuf, ebuf;
                nk_buffer_init_fixed( &vbuf, vertices, (nk_size) p->max_vertex_buffer );
                nk_buffer_init_fixed( &ebuf, elements, (nk_size) p->max_element_buffer );
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
            glScissor( ( GLint )( cmd->clip_rect.x * p->scale[ 0 ] ),
                       ( GLint )( ( p->dims[ 1 ] - ( GLint )( cmd->clip_rect.y + cmd->clip_rect.h ) ) * p->scale[ 1 ] ),
                       ( GLint )( cmd->clip_rect.w * p->scale[ 0 ] ),
                       ( GLint )( cmd->clip_rect.h * p->scale[ 1 ] ) );
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

void apemode::NuklearSdlGL::DeviceDestroy( ) {
    glDetachShader( prog, vert_shdr );
    glDetachShader( prog, frag_shdr );
    glDeleteShader( vert_shdr );
    glDeleteShader( frag_shdr );
    glDeleteProgram( prog );
    glDeleteTextures( 1, &font_tex );
    glDeleteBuffers( 1, &vbo );
    glDeleteBuffers( 1, &ebo );
}

void apemode::NuklearSdlGL::DeviceCreate( InitParametersBase *init_params ) {
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
    prog      = glCreateProgram( );
    vert_shdr = glCreateShader( GL_VERTEX_SHADER );
    frag_shdr = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( vert_shdr, 1, &vertex_shader, 0 );
    glShaderSource( frag_shdr, 1, &fragment_shader, 0 );
    glCompileShader( vert_shdr );
    glCompileShader( frag_shdr );
    glGetShaderiv( vert_shdr, GL_COMPILE_STATUS, &status );
    assert( status == GL_TRUE );
    glGetShaderiv( frag_shdr, GL_COMPILE_STATUS, &status );
    assert( status == GL_TRUE );
    glAttachShader( prog, vert_shdr );
    glAttachShader( prog, frag_shdr );
    glLinkProgram( prog );
    glGetProgramiv( prog, GL_LINK_STATUS, &status );
    assert( status == GL_TRUE );

    uniform_tex  = glGetUniformLocation( prog, "Texture" );
    uniform_proj = glGetUniformLocation( prog, "ProjMtx" );
    attrib_pos   = glGetAttribLocation( prog, "Position" );
    attrib_uv    = glGetAttribLocation( prog, "TexCoord" );
    attrib_col   = glGetAttribLocation( prog, "Color" );

    GLsizei vs = sizeof( Vertex );
    size_t  vp = offsetof( Vertex, position );
    size_t  vt = offsetof( Vertex, uv );
    size_t  vc = offsetof( Vertex, col );

    glGenBuffers( 1, &vbo );
    glGenBuffers( 1, &ebo );
    glGenVertexArrays( 1, &vao );

    glBindVertexArray( vao );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );

    glEnableVertexAttribArray( (GLuint) attrib_pos );
    glEnableVertexAttribArray( (GLuint) attrib_uv );
    glEnableVertexAttribArray( (GLuint) attrib_col );

    glVertexAttribPointer( (GLuint) attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *) vp );
    glVertexAttribPointer( (GLuint) attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *) vt );
    glVertexAttribPointer( (GLuint) attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *) vc );

    glBindTexture( GL_TEXTURE_2D, 0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
}

void *apemode::NuklearSdlGL::DeviceUploadAtlas( const void *image, int width, int height ) {
    glGenTextures( 1, &font_tex );
    glBindTexture( GL_TEXTURE_2D, font_tex );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) width, (GLsizei) height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );
    return (void *) font_tex;
}
