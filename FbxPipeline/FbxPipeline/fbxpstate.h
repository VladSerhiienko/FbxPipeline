#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace fbxp {
    struct Node {
        uint32_t                id;
        uint64_t                nameId;
        std::vector< uint32_t > childIds;
    };

    struct Material {
        uint32_t id;
        uint64_t nameId;

        struct Prop {
            uint64_t nameId;

            enum {
                eScalar,
                eColor,
                eTexture,
            } type;

            union {
                float    scalar;
                float    color[ 3 ];
                uint64_t textureId;
            };
        };
        std::vector< Prop > props;
    };

    struct State {
        fbxsdk::FbxManager*               manager = nullptr;
        fbxsdk::FbxScene*                 scene   = nullptr;
        std::shared_ptr< spdlog::logger > console;
        flatbuffers::FlatBufferBuilder    builder;
        cxxopts::Options                  options;
        std::string                       fileName;
        std::string                       folderPath;
        std::map< uint64_t, std::string > names;
        std::vector< fb::TransformFb >    transforms;
        std::vector< Node >               nodes;
        std::vector< Material >           materials;
        std::vector< fb::TextureFb >      textures;

        State( );
        ~State( );

        bool Initialize( );
        void Release( );
        bool Load( const char* fileName );
        bool Finish( );
        uint64_t PushName( std::string const& name );

        friend State& GetState( );
    };
}