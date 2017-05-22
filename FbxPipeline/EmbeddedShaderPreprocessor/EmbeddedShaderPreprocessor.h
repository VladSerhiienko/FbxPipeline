#pragma once

#include <string>

namespace apemode {
    struct EmbeddedShaderPreprocessor {
        enum Target {
            OpenGL,
            OpenGLES20,
            OpenGLES30,
            Metal,
        };

        enum Type {
            Vertex,
            Fragment,
            Compute
        };

        enum Status {
            Preprocessed = 1,
            Optimized    = 2,
        };

        struct Impl;
        friend Impl;
        Impl * impl;

        EmbeddedShaderPreprocessor( );
        ~EmbeddedShaderPreprocessor( );

        /**
         * Adds include file 
         */
        void PushFile( std::string const& file, std::string const& content );
        void PushDefinition( std::string const& define );
        bool Preprocess( std::string*       preprocessedContent,
                         std::string*       optimizedContent,
                         std::string const& initialContent,
                         Target             shaderTarget,
                         Type               shaderType );
    };
} // namespace apemode