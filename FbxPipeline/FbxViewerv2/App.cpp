#include <fbxvpch.h>

#include <App.h>
#include <AppSurfaceSdlVk.h>
#include <Input.h>
#include <NuklearSdlVk.h>

#include <Scene.h>

#include <EmbeddedShaderPreprocessor.h>

using namespace apemode;

struct apemode::AppContent {
    NuklearSdlBase* Nk = nullptr;
    nk_color        diffColor;
    nk_color        specColor;
    uint32_t        width      = 0;
    uint32_t        height     = 0;
    uint32_t        resetFlags = 0;
    uint32_t        envId      = 0;
    uint32_t        sceneId    = 0;
    uint32_t        maskId     = 0;
    Scene*          scenes[ 2 ];
};

App::App( ) : content( new AppContent( ) ) {
}

App::~App( ) {
}

IAppSurface* App::CreateAppSurface( ) {
    return new AppSurfaceSdlVk( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {
    if ( AppBase::Initialize( Args, ppArgs ) ) {
        totalSecs = 0.0f;

        content->Nk = new NuklearSdlVk();
        NuklearSdlVk::InitParametersVk initParams;
        content->Nk->Init( &initParams );

        apemode::EmbeddedShaderPreprocessor preproc;
        std::vector< std::string > definitions = {"AM_OPTION_1=1", "AM_OPTION_2=0"};
        std::vector< std::string > shaderTable = {"func_1.glsl",
                                                  "float func_1() { return 1.0; }",
                                                  "func_2.glsl",
                                                  "float func_2() { return 2.0; }",
                                                  "func_3.glsl",
                                                  "float func_3() { return 3.0; }",
                                                  "test.glsl",
                                                  "#if AM_OPTION_1\n"
                                                  "#include <func_1.glsl>\n"
                                                  "#endif\n"
                                                  "#if AM_OPTION_2\n"
                                                  "#include <func_2.glsl>\n"
                                                  "#endif\n"
                                                  "#include <func_3.glsl>\n"
                                                  "\n"
                                                  "void main () {\n"
                                                  "float a = 1.0;\n"
                                                  "#if AM_OPTION_1\n"
                                                  "a += func_1();\n"
                                                  "#endif\n"
                                                  "#if AM_OPTION_2\n"
                                                  "a += func_2();\n"
                                                  "#endif\n"
                                                  "a += func_3();\n"
                                                  "gl_FragColor = vec4(a, a, a, 1.0);\n"
                                                  "}\n"
                                                  ""};

        std::string shaderCode, log;
        std::vector< uint32_t > dependencies;

        preproc.Preprocess( nullptr,
                            &shaderCode,
                            &log,
                            &dependencies,
                            shaderTable,
                            definitions,
                            "test.glsl",
                            apemode::EmbeddedShaderPreprocessor::OpenGLES20,
                            apemode::EmbeddedShaderPreprocessor::Fragment );

        content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/iron-man.fbxp" );
        content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/kalestra-the-sorceress.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6kv3ps.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/Mech6k_v2.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90_v2.fbxp" );
        // content->scenes[ 0 ] = LoadSceneFromFile( "../../../assets/MercedesBenzA45AMG.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/MercedesBenzSLR.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/P90.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/IronMan.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Cathedral.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Leica1933.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/UnrealOrb.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Artorias.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/9mm.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/Knife.fbxp" );
        // content->scenes[ 1 ] = LoadSceneFromFile( "../../../assets/mech-m-6k.fbxp" );


        return true;
    }

    return false;
}

void App::OnFrameMove( ) {
    SDL_Event evt;
   /* nk_input_begin( content->nk );
    while ( SDL_PollEvent( &evt ) ) {
        nk_sdl_handle_event( &evt );
    }
    nk_input_end( content->nk );*/

    AppBase::OnFrameMove( );
}

void App::Update( float deltaSecs, Input const& inputState ) {
    totalSecs += deltaSecs;

    const uint32_t width  = GetSurface( )->GetWidth( );
    const uint32_t height = GetSurface( )->GetHeight( );

    bool hovered = false;
    bool reset   = false;

    const nk_flags windowFlags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE;

    //if ( nk_begin( content->nk, "FBXV: Demo.", nk_rect( 10, 60, 400, 600 ), windowFlags ) ) {
    //    nk_layout_row_dynamic( content->nk, 20, 1 );
    //    nk_labelf_wrap( content->nk, "Orbit - left button" );
    //    nk_labelf_wrap( content->nk, "Zoom - right button" );
    //    nk_labelf_wrap( content->nk, "Tab - statistics" );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    //nk_property_float( content->nk, "Glossiness", 0, &content->uniforms.m_glossiness, 1, 0.01, 0.001 );
    //    //nk_property_float( content->nk, "Reflectivity", 0, &content->uniforms.m_reflectivity, 1, 0.01, 0.001 );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    nk_label_wrap( content->nk, "Diffuse Color" );

    //    nk_layout_row_dynamic( content->nk, 100, 1 );
    //    content->diffColor = nk_color_picker( content->nk, content->diffColor, nk_color_format::NK_RGB );
    //    //nk_color_fv( content->uniforms.m_rgbDiff, content->diffColor );

    //    nk_layout_row_dynamic( content->nk, 10, 1 );
    //    nk_spacing( content->nk, 1 );

    //    nk_layout_row_dynamic( content->nk, 30, 1 );
    //    nk_combobox_string( content->nk,
    //                        "env/kyoto\0"
    //                        "env/bolonga\0\0",
    //                        (int*) &content->envId,
    //                        2,
    //                        30,
    //                        nk_vec2( 300, 300 ) );

    //    nk_combobox_string( content->nk,
    //                        "scene/A45-AMG\0"
    //                        "scene/Mech-6k\0\0",
    //                        (int*) &content->sceneId,
    //                        2,
    //                        30,
    //                        nk_vec2( 300, 300 ) );

    //    nk_layout_row_dynamic( content->nk, 10, 1 );
    //    nk_spacing( content->nk, 1 );

    //    hovered = nk_window_is_any_hovered( content->nk );
    //}
    //nk_end( content->nk );

    //glClearColor( 0.0f, 1.0f, 1.0f, 1.0f );
    //glClearDepth( 1.0 );
    //glClearStencil( 0 );

    //glEnable( GL_BLEND );
    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glEnable( GL_CULL_FACE );
    //glDisable( GL_DEPTH_TEST );

    //glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

    //nk_sdl_render( NK_ANTI_ALIASING_ON, 64 * 1024, 64 * 1024 );
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateApp( ) {
    return new App( );
}