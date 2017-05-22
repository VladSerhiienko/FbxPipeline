#include "EmbeddedShaderPreprocessor.h"

#include <assert.h>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include <Windows.h>

extern "C" {
#include <fpp.h>
}

#include <glsl_optimizer.h>

struct apemode::EmbeddedShaderPreprocessor::Impl {
    std::set< std::string >           definitions;
    std::vector< std::string > const* shaderTable;
    std::vector< uint32_t >           pos;
    std::vector< fppTag >             tags;
    std::stringstream                 err;
    std::stringstream                 out;
    std::vector< uint32_t >*          dependencies;
    std::string                       log;
    uint32_t                          inputIndex;
    glslopt_ctx*                      glsl[ 3 ] = {nullptr};

    ~Impl( ) {
        for ( auto& ctx : glsl )
            if ( ctx )
                glslopt_cleanup( ctx );
    }

    uint32_t GetShaderIndex( std::string const& shaderName ) {
        if ( nullptr != shaderTable )
            for ( uint32_t i = 0; i < shaderTable->size( ); i += 2 )
                if ( shaderTable->at( i ) == shaderName )
                    return i;

        return -1;
    }

    static void FppDepends( char* _fileName, void* _userData ) {
        if ( Impl* impl = (Impl*) _userData )
            if ( nullptr != impl->dependencies )
                impl->dependencies->push_back( impl->GetShaderIndex( _fileName ) );
    }

    static char* FGets( char* _buffer, int _size, std::string const& input, uint32_t& p ) {
        if ( p >= input.size( ) ) {
            _buffer[ 0 ] = '\0';
            return nullptr;
        }

        int  ii = 0;
        char c  = input[ p ];
        for ( ; p < input.size( ) && ii < ( _size - 1 ); c = input[ ++p ] ) {
            _buffer[ ii++ ] = c;
            if ( c == '\n' || ii == _size ) {
                break;
            }
        }

        _buffer[ ii ] = '\0';
        p++;
        return _buffer;
    }

    static char* FppFGets( char* _buffer, int _size, void* _file, void* _userData ) {
        Impl* impl = (Impl*) _userData;

        uint32_t shaderIndex = (uint32_t) _file;
        if ( shaderIndex != -1 )
            return FGets( _buffer, _size, impl->shaderTable->at( shaderIndex ), impl->pos[ shaderIndex >> 1 ] );

        return nullptr;
    }

    static char* FppInput( char* _buffer, int _size, void* _userData ) {
        Impl* impl = (Impl*) _userData;
        return FppFGets( _buffer, _size, (void*) impl->inputIndex, _userData );
    }

    static void FppOutput( int _ch, void* _userData ) {
        Impl* impl = (Impl*) _userData;
        impl->out << (char) _ch;
    }

    static void* FppFOpen( char* _fileName, void* _userData ) {
        Impl* impl = (Impl*) _userData;

        auto shaderIndex = impl->GetShaderIndex( _fileName );
        if ( shaderIndex != -1 )
            return (void*) ( shaderIndex + 1 );

        return nullptr;
    }

    static void FppError( void* _userData, char* _format, va_list _vargs ) {
        char err[ 1024 ] = {0};
        vsprintf_s( err, _format, _vargs );

        Impl* impl = (Impl*) _userData;
        impl->err << err;
        impl->err << "\n\n";
    }

    glslopt_ctx* GetGlslOpt( glslopt_target shaderTarget ) {
        if ( auto ctx = glsl[ shaderTarget ] )
            return ctx;

        return ( glsl[ shaderTarget ] = glslopt_initialize( shaderTarget ) );
    }
};

apemode::EmbeddedShaderPreprocessor::EmbeddedShaderPreprocessor( ) : impl( new Impl( ) ) {
    assert( impl );
}

apemode::EmbeddedShaderPreprocessor::~EmbeddedShaderPreprocessor( ) {
    if ( impl )
        delete impl;
}

bool apemode::EmbeddedShaderPreprocessor::Preprocess( std::string*                      preprocessedContent,
                                                      std::string*                      optimizedContent,
                                                      std::string*                      log,
                                                      std::vector< uint32_t >*          dependencies,
                                                      std::vector< std::string > const& shaderTable,
                                                      std::vector< std::string > const& definitions,
                                                      std::string const&                shaderName,
                                                      ShaderTarget                      shaderTarget,
                                                      ShaderType                        shaderType ) {
    impl->shaderTable  = &shaderTable;
    impl->dependencies = dependencies;
    impl->pos.resize( shaderTable.size( ) >> 1, 0 );

    impl->inputIndex    = impl->GetShaderIndex( shaderName ) + 1;
    auto& shaderContent = shaderTable[ impl->inputIndex ];

    impl->tags.reserve( 12 + impl->definitions.size( ) );

    impl->tags.emplace_back( fppTag{FPPTAG_USERDATA, (void*) impl} );
    impl->tags.emplace_back( fppTag{FPPTAG_INPUT, (void*) &Impl::FppInput} );
    impl->tags.emplace_back( fppTag{FPPTAG_ERROR, (void*) &Impl::FppError} );
    impl->tags.emplace_back( fppTag{FPPTAG_FOPEN, (void*) &Impl::FppFOpen} );
    impl->tags.emplace_back( fppTag{FPPTAG_FGETS, (void*) &Impl::FppFGets} );
    impl->tags.emplace_back( fppTag{FPPTAG_OUTPUT, (void*) &Impl::FppOutput} );
    impl->tags.emplace_back( fppTag{FPPTAG_DEPENDS, (void*) &Impl::FppDepends} );
    impl->tags.emplace_back( fppTag{FPPTAG_LINE, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_IGNOREVERSION, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_KEEPCOMMENTS, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_INPUT_NAME, (void*) shaderContent.c_str( )} );

    for ( auto& definition : definitions )
        impl->tags.emplace_back( fppTag{FPPTAG_DEFINE, (void*) definition.c_str( )} );

    impl->tags.emplace_back( fppTag{FPPTAG_END, (void*) 0} );

    impl->err.str( "" );
    impl->err.clear( );

    impl->out.str( "" );
    impl->out.clear( );

    auto fppResult = fppPreProcess( impl->tags.data( ) );
    if ( fppResult != 0 ) {
        if ( log )
            // TODO: Consider error codes of the fpp.
            *log = "Failed to preprocess code.";
        return false;
    }

    // OutputDebugStringA( impl->out.str( ).c_str( ) );
    // OutputDebugStringA( "\n----------------------------------------------------\n" );

    if ( preprocessedContent ) {
        *preprocessedContent = impl->out.str( );
    }

    if ( optimizedContent ) {
        if ( glslopt_ctx* ctx = impl->GetGlslOpt( (glslopt_target) shaderTarget ) ) {
            glslopt_shader* shader =
                glslopt_optimize( ctx,
                                  glslopt_shader_type( shaderType ),
                                  preprocessedContent ? preprocessedContent->c_str( ) : impl->out.str( ).c_str( ),
                                  0 );

            if ( !glslopt_get_status( shader ) ) {
                if ( log )
                    *log = glslopt_get_log( shader );
                return false;
            }

            *optimizedContent = glslopt_get_output( shader );

            // OutputDebugStringA( optimizedContent->c_str( ) );
            // OutputDebugStringA( "\n----------------------------------------------------\n" );
        }
    }

    return true;
}
