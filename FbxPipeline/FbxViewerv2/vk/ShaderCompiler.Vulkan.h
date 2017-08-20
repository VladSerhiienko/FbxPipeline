#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <set>

namespace apemodevk {
    class ShaderCompiler {
        struct Impl;
        friend Impl;
        Impl* pImpl;

    public:
        /* Simple interface to read files from shader assets */
        class IShaderFileReader {
        public:
            virtual bool ReadShaderTxtFile( const std::string& FilePath,
                                            std::string&       OutFileFullPath,
                                            std::string&       OutFileContent ) = 0;
        };

        /* Simple interface to read files from shader assets */
        class IShaderFeedbackWriter {
        public:
            enum EFeedbackTypeBits {
                /* Copy-paste from shaderc */
                eFeedbackType_CompilationStatus_Success = 0,
                eFeedbackType_CompilationStatus_InvalidStage, /* Error stage deduction */
                eFeedbackType_CompilationStatus_CompilationError,
                eFeedbackType_CompilationStatus_InternalError, /* Unexpected failure */
                eFeedbackType_CompilationStatus_NullResultObject,
                eFeedbackType_CompilationStatus_InvalidAssembly,
                eFeedbackType_CompilationStatusMask = 0xf,

                /* Compilation Stage */
                eFeedbackType_CompilationStage_Preprocessed          = 0x10, /* Txt */
                eFeedbackType_CompilationStage_PreprocessedOptimized = 0x20, /* Txt */
                eFeedbackType_CompilationStage_Assembly              = 0x30, /* Txt */
                eFeedbackType_CompilationStage_Spv                   = 0x40, /* Bin */
                eFeedbackType_CompilationStageMask                   = 0xf0,
            };

            using EFeedbackType = std::underlying_type< EFeedbackTypeBits >::type;

            /**
             * If CompilationStatus bit is not Success, pContent is an error message (Txt).
             * pContent is Bin in case CompilationStatus bit is success and Stage is Spv.
             **/
            virtual void WriteFeedback( EFeedbackType                     eType,
                                        const std::string&                FullFilePath,
                                        const std::vector< std::string >& Macros,
                                        const void*                       pContent, /* Txt or bin, @see EFeedbackType */
                                        const void*                       pContentEnd ) = 0;
        };

        /* Copy-paste from shaderc */
        enum EShaderType {
            // Forced shader kinds. These shader kinds force the compiler to compile the
            // source code as the specified kind of shader.
            eShaderType_VertexShader,
            eShaderType_FragmentShader,
            eShaderType_ComputeShader,
            eShaderType_GeometryShader,
            eShaderType_TessControlShader,
            eShaderType_TessevaluationShader,

            eShaderType_GLSL_VertexShader         = eShaderType_VertexShader,
            eShaderType_GLSL_FragmentShader       = eShaderType_FragmentShader,
            eShaderType_GLSL_ComputeShader        = eShaderType_ComputeShader,
            eShaderType_GLSL_GeometryShader       = eShaderType_GeometryShader,
            eShaderType_GLSL_TessControlShader    = eShaderType_TessControlShader,
            eShaderType_GLSL_TessEvaluationShader = eShaderType_TessevaluationShader,

            // Deduce the shader kind from #pragma annotation in the source code. Compiler
            // will emit error if #pragma annotation is not found.
            eShaderType_GLSL_InferFromSource,

            // Default shader kinds. Compiler will fall back to compile the source code as
            // the specified kind of shader when #pragma annotation is not found in the
            // source code.
            eShaderType_GLSL_Default_VertexShader,
            eShaderType_GLSL_Default_FragmentShader,
            eShaderType_GLSL_Default_ComputeShader,
            eShaderType_GLSL_Default_GeometryShader,
            eShaderType_GLSL_Default_TessControlShader,
            eShaderType_GLSL_Default_TessEvaluationShader,

            eShaderType_SPIRV_assembly,
        };

    public:
        ShaderCompiler( );
        virtual ~ShaderCompiler( );

        /* @note No files, only ready to compile shader sources */

        virtual bool Compile( const std::string&                ShaderName,
                              const std::string&                ShaderCode,
                              const std::vector< std::string >& Macros,
                              EShaderType                       eShaderKind,
                              std::vector< uint8_t >&           OutCompiledShader );

        /* @note Compiling from source files */

        virtual IShaderFileReader*     GetShaderFileReader( );
        virtual IShaderFeedbackWriter* GetShaderFeedbackWriter( );
        virtual void                   SetShaderFileReader( IShaderFileReader* pShaderFileReader );
        virtual void                   SetShaderFeedbackWriter( IShaderFeedbackWriter* pShaderFeedbackWriter );

        virtual bool Compile( const std::string&                FilePath,
                              const std::vector< std::string >& Macros,
                              EShaderType                       eShaderKind,
                              std::set< std::string >&          OutIncludedFiles,
                              std::vector< uint8_t >&           OutCompiledShader );
    };
} // namespace apemodevk