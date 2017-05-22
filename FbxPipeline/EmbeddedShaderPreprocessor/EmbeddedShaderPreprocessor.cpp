#include "EmbeddedShaderPreprocessor.h"

#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <sstream>

#include <Windows.h>

extern "C" {
#include <fpp.h>
}

#include <glsl_optimizer.h>

struct apemode::EmbeddedShaderPreprocessor::Impl {
    std::map< std::string, std::pair< std::string, uint32_t > > files;
    std::set< std::string >                                     definitions;
    std::vector< fppTag >                                       tags;
    uint32_t                                                    pos;
    std::stringstream                                           err;
    std::stringstream                                           out;
    std::set< std::string >                                     dependencies;
    std::string                                                 log;

    static void fppDepends( char* _fileName, void* _userData ) {
        Impl* self = (Impl*) _userData;
        self->dependencies.insert( _fileName );
    }

    static char* Fgets( char* _buffer, int _size, std::string const& input, uint32_t& p ) {
        if (p >= input.size()) {
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

    static char* fppFgets( char* _buffer, int _size, void* _file, void* _userData ) {
        Impl* self = (Impl*) _userData;

        auto fileIt = self->files.find( (char*) _file );
        if ( fileIt != self->files.end( ) ) {
            return Fgets( _buffer, _size, fileIt->second.first, fileIt->second.second );
        }

        return nullptr;
    }

    static char* fppInput( char* _buffer, int _size, void* _userData ) {
        return fppFgets( _buffer, _size, "this", _userData );
    }

    static void fppOutput( int _ch, void* _userData ) {
        Impl* self = (Impl*) _userData;
        self->out << (char) _ch;
    }

    static void* fppFopen( char* _fileName, void* _userData ) {
        Impl* self = (Impl*) _userData;

        auto fileIt = self->files.find( _fileName );
        if ( fileIt != self->files.end( ) )
            return (void*) fileIt->first.c_str( );

        return nullptr;
    }

    static void fppError( void* _userData, char* _format, va_list _vargs ) {
        char err[ 1024 ] = {0};
        vsprintf_s( err, _format, _vargs );

        Impl* self = (Impl*) _userData;
        self->err << err;
        self->err << "\n\n";
    }
};

apemode::EmbeddedShaderPreprocessor::EmbeddedShaderPreprocessor( ) : impl( new Impl( ) ) {
    assert( impl );
}

apemode::EmbeddedShaderPreprocessor::~EmbeddedShaderPreprocessor( ) {
    if ( impl )
        delete impl;
}

void apemode::EmbeddedShaderPreprocessor::PushFile( std::string const& file, std::string const& content ) {
    impl->files[ file ] = std::make_pair( content, uint32_t( 0 ) );
}

void apemode::EmbeddedShaderPreprocessor::PushDefinition( std::string const& definition) {
    impl->definitions.insert( definition );
}

bool apemode::EmbeddedShaderPreprocessor::Preprocess( std::string*       preprocessedContent,
                                                      std::string*       optimizedContent,
                                                      std::string const& initialContent,
                                                      Target             shaderTarget,
                                                      Type               shaderType ) {
    impl->files[ "this" ] = std::make_pair( initialContent, uint32_t( 0 ) );
    impl->tags.reserve( 11 + impl->definitions.size( ) );

    impl->tags.emplace_back( fppTag{FPPTAG_USERDATA, (void*) impl} );
    impl->tags.emplace_back( fppTag{FPPTAG_INPUT, (void*) &Impl::fppInput} );
    impl->tags.emplace_back( fppTag{FPPTAG_ERROR, (void*) &Impl::fppError} );
    impl->tags.emplace_back( fppTag{FPPTAG_FOPEN, (void*) &Impl::fppFopen} );
    impl->tags.emplace_back( fppTag{FPPTAG_FGETS, (void*) &Impl::fppFgets} );
    impl->tags.emplace_back( fppTag{FPPTAG_OUTPUT, (void*) &Impl::fppOutput} );
    impl->tags.emplace_back( fppTag{FPPTAG_DEPENDS, (void*) &Impl::fppDepends} );
    impl->tags.emplace_back( fppTag{FPPTAG_LINE, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_IGNOREVERSION, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_KEEPCOMMENTS, (void*) 0} );
    impl->tags.emplace_back( fppTag{FPPTAG_INPUT_NAME, (void*) impl->files.find( "this" )->first.c_str( )} );

    for ( auto& definition : impl->definitions )
        impl->tags.emplace_back( fppTag{FPPTAG_DEFINE, (void*) definition.c_str( )} );

    impl->tags.emplace_back( fppTag{FPPTAG_END, (void*) 0} );

    impl->pos = 0;
    impl->err.str( "" );
    impl->err.clear( );
    impl->out.str( "" );
    impl->out.clear( );
    impl->dependencies.clear( );

    auto fppResult = fppPreProcess( impl->tags.data( ) );
    if ( fppResult != 0 )
        return false;

    OutputDebugStringA( impl->out.str( ).c_str( ) );
    OutputDebugStringA( "\n\n" );

    if ( preprocessedContent ) {
        *preprocessedContent = impl->out.str( );
    }

    if ( optimizedContent ) {
        if ( glslopt_ctx* ctx = glslopt_initialize( glslopt_target( shaderTarget ) ) ) {
            glslopt_shader* shader =
                glslopt_optimize( ctx,
                                  glslopt_shader_type( shaderType ),
                                  preprocessedContent ? preprocessedContent->c_str( ) : impl->out.str( ).c_str( ),
                                  0 );

            if ( !glslopt_get_status( shader ) ) {
                impl->log = glslopt_get_log( shader );
                glslopt_cleanup( ctx );
                return false;
            }

            *optimizedContent = glslopt_get_output( shader );

            OutputDebugStringA( optimizedContent->c_str( ) );
            OutputDebugStringA( "\n\n" );

            glslopt_cleanup( ctx );
            return true;
        } else
            return false;
    }

    return true;
}
