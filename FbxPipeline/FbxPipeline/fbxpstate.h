#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace fbxp {

    struct Node {
        uint32_t                id;
        uint64_t                nameId;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
    };

    struct Material {
        uint32_t                          id;
        uint64_t                          nameId;
        std::vector< fb::MaterialPropFb > props;
    };

    struct State {
        fbxsdk::FbxManager*               manager = nullptr;
        fbxsdk::FbxScene*                 scene   = nullptr;
        std::shared_ptr< spdlog::logger > console;
        flatbuffers::FlatBufferBuilder    builder;
        cxxopts::Options                  options;
        std::string                       fileName;
        std::string                       folderPath;
        std::vector< Node >               nodes;
        std::vector< Material >           materials;
        std::map< uint64_t, uint32_t >    materialDict;
        std::map< uint64_t, std::string > names;
        std::vector< fb::TransformFb >    transforms;
        std::vector< fb::TextureFb >      textures;

        State( );
        ~State( );

        bool Initialize( );
        void Release( );
        bool Load( const char* fileName );
        bool Finish( );
        uint64_t PushName( std::string const& name );

        friend State& Get( );
    };
}