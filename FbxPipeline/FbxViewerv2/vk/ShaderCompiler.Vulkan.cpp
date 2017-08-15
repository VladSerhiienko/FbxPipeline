#include <ShaderCompiler.Vulkan.h>
#include <shaderc/shaderc.hpp>

struct apemodevk::ShaderCompiler::Impl {
    shaderc::Compiler Compiler;
    const apemodevk::ShaderCompiler::IShaderFileReader* pShaderFileReader;
};

class Includer : public shaderc::CompileOptions::IncluderInterface {
public:
    struct UserData {
        std::string Content;
        std::string Path;
    };

    std::set< std::string >& IncludedFiles;
    const apemodevk::ShaderCompiler::IShaderFileReader& FileReader;

    Includer( const apemodevk::ShaderCompiler::IShaderFileReader& FileReader, std::set< std::string >& IncludedFiles )
        : FileReader( FileReader ), IncludedFiles( IncludedFiles ) {
    }

    // Handles shaderc_include_resolver_fn callbacks.
    shaderc_include_result* GetInclude( const char*          requested_source,
                                        shaderc_include_type type,
                                        const char*          requesting_source,
                                        size_t               include_depth ) {
        auto userData = std::make_unique< UserData >( );
        if ( FileReader.ReadShaderTxtFile( requested_source, userData->Path, userData->Content ) ) {
            IncludedFiles.insert( userData->Path );

            auto includeResult = std::make_unique< shaderc_include_result >( );
            includeResult->content = userData->Content.data( );
            includeResult->content_length = userData->Content.size( );
            includeResult->source_name = userData->Path.data( );
            includeResult->source_name_length = userData->Path.size( );
            includeResult->user_data = userData.release( );
            return includeResult.release( );
        }

        return nullptr;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude( shaderc_include_result* data ) {
        delete (UserData*) data->user_data;
        delete data;
    }
};

apemodevk::ShaderCompiler::ShaderCompiler( ) : pImpl( new Impl( ) ) {
}

apemodevk::ShaderCompiler::~ShaderCompiler( ) {
    delete pImpl;
}

const apemodevk::ShaderCompiler::IShaderFileReader* apemodevk::ShaderCompiler::GetShaderFileReader( ) {
    return pImpl->pShaderFileReader;
}

void apemodevk::ShaderCompiler::SetShaderFileReader( const IShaderFileReader* pShaderFileReader ) {
    pImpl->pShaderFileReader = pShaderFileReader;
}

bool apemodevk::ShaderCompiler::Compile( const std::string&      ShaderName,
                                         const std::string&      ShaderCode,
                                         EShaderType             eShaderKind,
                                         std::vector< uint8_t >& OutCompiledShader ) {
    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level_size );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pImpl->Compiler.PreprocessGlsl( ShaderCode, (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus( ) ) {
        platform::DebugTrace( "ShaderCompiler: Failed to PreprocessGlsl: %s: %s",
                              ShaderName.c_str( ),
                              preprocessedSourceCompilationResult.GetErrorMessage( ).c_str( ) );

        platform::DebugBreak( );
        return false;
    }

#if 1
    shaderc::AssemblyCompilationResult assemblyCompilationResult = pImpl->Compiler.CompileGlslToSpvAssembly(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );
    OutputDebugStringA( "ShaderCompiler: " );
    OutputDebugStringA( ShaderName.c_str( ) );
    OutputDebugStringA( "\n" );
    OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );
    OutputDebugStringA( assemblyCompilationResult.begin( ) );
    OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );

    if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
        platform::DebugTrace( "ShaderCompiler: CompileGlslToSpvAssembly: %s: %s",
                              ShaderName.c_str( ),
                              assemblyCompilationResult.GetErrorMessage( ).c_str( ) );

        platform::DebugBreak( );
    }
#endif

    shaderc::SpvCompilationResult spvCompilationResult = pImpl->Compiler.CompileGlslToSpv(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
        platform::DebugTrace( "ShaderCompiler: CompileGlslToSpv: %s: %s", ShaderName.c_str( ), spvCompilationResult.GetErrorMessage( ).c_str( ) );
        platform::DebugBreak( );
        return false;
    }

    OutCompiledShader.resize( std::distance( spvCompilationResult.begin( ), spvCompilationResult.end( ) ) *
                              sizeof( shaderc::SpvCompilationResult::element_type ) );
    memcpy( OutCompiledShader.data( ), spvCompilationResult.begin( ), OutCompiledShader.size( ) );
    return true;
}

bool apemodevk::ShaderCompiler::Compile( const std::string&                InFilePath,
                                         const std::vector< std::string >& Macros,
                                         EShaderType                       eShaderKind,
                                         std::set< std::string >&          OutIncludedFiles,
                                         std::vector< uint8_t >&           OutCompiledShader ) {
    if ( nullptr == pImpl->pShaderFileReader ) {
        platform::DebugTrace( "ShaderCompiler: pShaderFileReader must be set." );
        platform::DebugBreak( );
        return false;
    }

    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level_size );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );
    options.SetIncluder( std::make_unique< Includer >( *pImpl->pShaderFileReader, OutIncludedFiles ) );

    for ( uint32_t i = 0; i < Macros.size( ); i += 2 ) {
        options.AddMacroDefinition( Macros[ i ], Macros[ i + 1 ] );
    }

    std::string fullPath;
    std::string fileContent;

    if ( pImpl->pShaderFileReader->ReadShaderTxtFile( InFilePath, fullPath, fileContent ) ) {
        shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
            pImpl->Compiler.PreprocessGlsl( fileContent, (shaderc_shader_kind) eShaderKind, fullPath.c_str( ), options );

        if ( shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus( ) ) {
            OutputDebugStringA( "ShaderCompiler: PreprocessGlsl: " );
            OutputDebugStringA( InFilePath.c_str( ) );
            OutputDebugStringA( " " ); 
            OutputDebugStringA( preprocessedSourceCompilationResult.GetErrorMessage( ).c_str( ) );
            OutputDebugStringA( "\n" ); 
            platform::DebugBreak( );
            return false;
        }

#if 1
        OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );
        OutputDebugStringA( "ShaderCompiler: PreprocessedSourceCompilationResult: " );
        OutputDebugStringA( fullPath.c_str( ) );
        OutputDebugStringA( "\n" );
        OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );
        OutputDebugStringA( preprocessedSourceCompilationResult.begin( ) );
        OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );

        shaderc::AssemblyCompilationResult assemblyCompilationResult = pImpl->Compiler.CompileGlslToSpvAssembly(
            preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, fullPath.c_str( ), options );

        OutputDebugStringA( "ShaderCompiler: AssemblyCompilationResult: " ); 
        OutputDebugStringA( fullPath.c_str( ) );
        OutputDebugStringA( "\n" ); 
        OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );
        OutputDebugStringA( assemblyCompilationResult.begin( ) );
        OutputDebugStringA( "ShaderCompiler: -------------------------------------------\n" );

        if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
            OutputDebugStringA( "ShaderCompiler: CompileGlslToSpvAssembly: " );
            OutputDebugStringA( assemblyCompilationResult.GetErrorMessage( ).c_str( ) );
            OutputDebugStringA( "\n" ); 
            platform::DebugBreak( );
            return false;
        }
#endif

        shaderc::SpvCompilationResult spvCompilationResult = pImpl->Compiler.CompileGlslToSpv(
            preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, fullPath.c_str( ), options );

        if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
            OutputDebugStringA( "ShaderCompiler: CompileGlslToSpv: " );
            OutputDebugStringA( InFilePath.c_str( ) );
            OutputDebugStringA( " " ); 
            OutputDebugStringA( spvCompilationResult.GetErrorMessage( ).c_str( ) );
            OutputDebugStringA( "\n" ); 
            platform::DebugBreak( );
            return false;
        }

        OutCompiledShader.resize( std::distance( spvCompilationResult.begin( ), spvCompilationResult.end( ) ) *
                                  sizeof( shaderc::SpvCompilationResult::element_type ) );
        memcpy( OutCompiledShader.data( ), spvCompilationResult.begin( ), OutCompiledShader.size( ) );
        return true;
    }

    return false;
}
