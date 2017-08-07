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
                                            std::string&       OutFileContent ) const = 0;
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

        /* @note No files or preprocessing, only ready to compile shader sources */

        virtual bool Compile( const std::string&      ShaderName,
                              const std::string&      ShaderCode,
                              EShaderType             eShaderKind,
                              std::vector< uint8_t >& OutCompiledShader );

        /* @note Compiling from source files */

        virtual const IShaderFileReader* GetShaderFileReader( );
        virtual void                     SetShaderFileReader( const IShaderFileReader* pShaderFileReader );

        virtual bool Compile( const std::string&                FilePath,
                              const std::vector< std::string >& Macros,
                              EShaderType                       eShaderKind,
                              std::set< std::string >&          OutIncludedFiles,
                              std::vector< uint8_t >&           OutCompiledShader );
    };
} // namespace apemodevk