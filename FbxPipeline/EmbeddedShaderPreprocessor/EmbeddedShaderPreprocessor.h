#pragma once

#include <string>
#include <vector>

namespace apemode {

    struct EmbeddedShaderPreprocessor {
        enum ShaderTarget {
            OpenGL,
            OpenGLES20,
            OpenGLES30,
            Metal,
        };

        enum ShaderType {
            Vertex,
            Fragment,
            Compute
        };

        struct Impl;
        friend Impl;
        Impl * impl;

        EmbeddedShaderPreprocessor( );
        ~EmbeddedShaderPreprocessor( );

        bool Preprocess( std::string*                      preprocessedContent,
                         std::string*                      optimizedContent,
                         std::string*                      log,
                         std::vector< uint32_t >*          dependencies,
                         std::vector< std::string > const& shaderTable,
                         std::vector< std::string > const& definitions,
                         std::string const&                shaderName,
                         ShaderTarget                      shaderTarget,
                         ShaderType                        shaderType );
    };
} // namespace apemode