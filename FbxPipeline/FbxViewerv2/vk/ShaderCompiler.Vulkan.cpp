#include <ShaderCompiler.Vulkan.h>
#include <shaderc/shaderc.hpp>

struct apemodevk::ShaderCompiler::Impl {
    shaderc::Compiler                                 Compiler;
    apemodevk::ShaderCompiler::IShaderFileReader*     pShaderFileReader     = nullptr;
    apemodevk::ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter = nullptr;
};

class Includer : public shaderc::CompileOptions::IncluderInterface {
public:
    struct UserData {
        std::string Content;
        std::string Path;
    };

    std::set< std::string >& IncludedFiles;
    apemodevk::ShaderCompiler::IShaderFileReader& FileReader;

    Includer( apemodevk::ShaderCompiler::IShaderFileReader& FileReader, std::set< std::string >& IncludedFiles )
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

            auto includeResult                = std::make_unique< shaderc_include_result >( );
            includeResult->content            = userData->Content.data( );
            includeResult->content_length     = userData->Content.size( );
            includeResult->source_name        = userData->Path.data( );
            includeResult->source_name_length = userData->Path.size( );
            includeResult->user_data          = userData.release( );
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

apemodevk::ShaderCompiler::IShaderFileReader* apemodevk::ShaderCompiler::GetShaderFileReader( ) {
    return pImpl->pShaderFileReader;
}

apemodevk::ShaderCompiler::IShaderFeedbackWriter* apemodevk::ShaderCompiler::GetShaderFeedbackWriter( ) {
    return pImpl->pShaderFeedbackWriter;
}

void apemodevk::ShaderCompiler::SetShaderFileReader( IShaderFileReader* pShaderFileReader ) {
    pImpl->pShaderFileReader = pShaderFileReader;
}

void apemodevk::ShaderCompiler::SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter ) {
    pImpl->pShaderFeedbackWriter = pShaderFeedbackWriter;
}

bool apemodevk::ShaderCompiler::Compile( const std::string&                ShaderName,
                                         const std::string&                ShaderCode,
                                         const std::vector< std::string >& Macros,
                                         EShaderType                       eShaderKind,
                                         std::vector< uint8_t >&           OutCompiledShader ) {
    shaderc::CompileOptions options;
    options.SetSourceLanguage( shaderc_source_language_glsl );
    options.SetOptimizationLevel( shaderc_optimization_level_size );
    options.SetTargetEnvironment( shaderc_target_env_vulkan, 0 );

    for ( uint32_t i = 0; i < Macros.size( ); i += 2 ) {
        options.AddMacroDefinition( Macros[ i ], Macros[ i + 1 ] );
    }

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pImpl->Compiler.PreprocessGlsl( ShaderCode, (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                                                         preprocessedSourceCompilationResult.GetCompilationStatus( ),
                                                         ShaderName,
                                                         Macros,
                                                         preprocessedSourceCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                         preprocessedSourceCompilationResult.GetErrorMessage( ).cend( ).  operator->( ) );
        }

        platform::DebugBreak( );
        return false;
    }

    if ( nullptr != pImpl->pShaderFeedbackWriter ) {
        pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                                                     IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                     ShaderName,
                                                     Macros,
                                                     preprocessedSourceCompilationResult.cbegin( ),
                                                     preprocessedSourceCompilationResult.cend( ) );
    }

#if 1
    shaderc::AssemblyCompilationResult assemblyCompilationResult = pImpl->Compiler.CompileGlslToSpvAssembly(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                                                         assemblyCompilationResult.GetCompilationStatus( ),
                                                         ShaderName,
                                                         Macros,
                                                         assemblyCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                         assemblyCompilationResult.GetErrorMessage( ).cend( ).  operator->( ) );
        }

        platform::DebugBreak( );
        return false;
    }

    if ( nullptr != pImpl->pShaderFeedbackWriter ) {
        pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                                                     IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                     ShaderName,
                                                     Macros,
                                                     assemblyCompilationResult.cbegin( ),
                                                     assemblyCompilationResult.cend( ) );
    }
#endif

    shaderc::SpvCompilationResult spvCompilationResult = pImpl->Compiler.CompileGlslToSpv(
        preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, ShaderName.c_str( ), options );

    if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                                                         spvCompilationResult.GetCompilationStatus( ),
                                                         ShaderName,
                                                         Macros,
                                                         spvCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                         spvCompilationResult.GetErrorMessage( ).cend( ).  operator->( ) );
        }

        platform::DebugBreak( );
        return false;
    }

    if ( nullptr != pImpl->pShaderFeedbackWriter ) {
        pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                                                     IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                     ShaderName,
                                                     Macros,
                                                     spvCompilationResult.cbegin( ),
                                                     spvCompilationResult.cend( ) );
    }

    size_t spvSize = (size_t) std::distance( spvCompilationResult.begin( ), spvCompilationResult.end( ) ) *
                     sizeof( shaderc::SpvCompilationResult::element_type );

    OutCompiledShader.resize( spvSize );
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
            if ( nullptr != pImpl->pShaderFeedbackWriter ) {
                pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                                                             preprocessedSourceCompilationResult.GetCompilationStatus( ),
                                                             fullPath,
                                                             Macros,
                                                             preprocessedSourceCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                             preprocessedSourceCompilationResult.GetErrorMessage( ).cend( ).operator->( ) );
            }

            platform::DebugBreak( );
            return false;
        }

        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                                                         IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                         fullPath,
                                                         Macros,
                                                         preprocessedSourceCompilationResult.cbegin( ),
                                                         preprocessedSourceCompilationResult.cend( ) );
        }

        OutIncludedFiles.insert( fullPath );

#if 1
        shaderc::AssemblyCompilationResult assemblyCompilationResult = pImpl->Compiler.CompileGlslToSpvAssembly(
            preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, fullPath.c_str( ), options );

        if ( shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus( ) ) {
            if ( nullptr != pImpl->pShaderFeedbackWriter ) {
                pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                                                             assemblyCompilationResult.GetCompilationStatus( ),
                                                             fullPath,
                                                             Macros,
                                                             assemblyCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                             assemblyCompilationResult.GetErrorMessage( ).cend( ).operator->( ) );
            }

            platform::DebugBreak( );
            return false;
        }

        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                                                         IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                         fullPath,
                                                         Macros,
                                                         assemblyCompilationResult.cbegin( ),
                                                         assemblyCompilationResult.cend( ) );
        }
#endif

        shaderc::SpvCompilationResult spvCompilationResult = pImpl->Compiler.CompileGlslToSpv(
            preprocessedSourceCompilationResult.begin( ), (shaderc_shader_kind) eShaderKind, fullPath.c_str( ), options );

        if ( shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus( ) ) {
            if ( nullptr != pImpl->pShaderFeedbackWriter ) {
                pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                                                             spvCompilationResult.GetCompilationStatus( ),
                                                             fullPath,
                                                             Macros,
                                                             spvCompilationResult.GetErrorMessage( ).cbegin( ).operator->( ),
                                                             spvCompilationResult.GetErrorMessage( ).cend( ).operator->( ) );
            }

            platform::DebugBreak( );
            return false;
        }

        if ( nullptr != pImpl->pShaderFeedbackWriter ) {
            pImpl->pShaderFeedbackWriter->WriteFeedback( IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                                                         IShaderFeedbackWriter::eFeedbackType_CompilationStatus_Success,
                                                         fullPath,
                                                         Macros,
                                                         spvCompilationResult.cbegin( ),
                                                         spvCompilationResult.cend( ) );
        }

        size_t spvSize = (size_t) std::distance( spvCompilationResult.begin( ), spvCompilationResult.end( ) ) *
                         sizeof( shaderc::SpvCompilationResult::element_type );

        OutCompiledShader.resize( spvSize );
        memcpy( OutCompiledShader.data( ), spvCompilationResult.begin( ), OutCompiledShader.size( ) );

        return true;
    }

    return false;
}
