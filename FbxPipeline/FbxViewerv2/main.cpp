// STL
//

#include <fstream>
#include <iostream>
#include <memory>

//
// ThirdParty
//

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// Generated
//

#include <scene_generated.h>

#include <bgfx/bgfx.h>

auto console = spdlog::stdout_color_mt( "cppdump" );

int main( int argc, char** argv ) {
    std::string file;
    bool convert = false;

    try {
        cxxopts::Options options( argv[ 0 ] );
        options.add_options( "input" )( "f,file", "File", cxxopts::value< std::string >( ) );
        options.parse( argc, argv );
        file = options[ "f" ].as< std::string >( );
    } catch ( const cxxopts::OptionException& e ) {
        console->critical( "error parsing options: {0}", e.what( ) );
        std::exit( 1 );
    }

    std::string f;
    if ( flatbuffers::LoadFile( file.c_str( ), true, &f ) ) {
        if ( auto scene = fbxp::fb::GetSceneFb( f.c_str( ) ) ) {
            if ( auto names = scene->names( ) ) {
                for ( auto name : *names ) {
                    console->info( "{} -> {}", name->h( ), name->v( )->c_str( ) );
                }
            }
        }
    }

    auto bgfxOk           = bgfx::init( bgfx::RendererType::Vulkan );
    auto bgfxRendererType = bgfx::getRendererType( );
    auto bgfxRendererName = bgfx::getRendererName( bgfxRendererType );

    return 0;
}
