#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace fbxp {

    struct Mesh {
        fbxp::fb::vec3                     min;
        fbxp::fb::vec3                     max;
        std::vector< uint32_t >            polygons;
        std::vector< fbxp::fb::vec3 >      controlPoints;
        std::vector< fbxp::fb::SubmeshFb > submeshes;
        std::vector< fbxp::fb::SubsetFb >  subsets;
        std::vector< uint32_t >            subsetIndices;
        std::vector< uint8_t >             vertices;
        std::vector< uint8_t >             indices;
    };

    struct Node {
        uint32_t                id;
        uint64_t                nameId;
        uint32_t                meshId = (uint32_t) -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
    };

    struct Material {
        uint32_t                          id;
        uint64_t                          nameId;
        std::vector< fb::MaterialPropFb > props;
    };

    struct State {
        bool                              legacyTriangulationSdk = false;
        fbxsdk::FbxManager*               manager = nullptr;
        fbxsdk::FbxScene*                 scene   = nullptr;
        std::shared_ptr< spdlog::logger > console;
        flatbuffers::FlatBufferBuilder    builder;
        cxxopts::Options                  options;
        std::string                       fileName;
        std::string                       folderPath;
        std::vector< Node >               nodes;
        std::vector< Material >           materials;
        std::map< uint64_t, uint32_t >    textureDict;
        std::map< uint64_t, uint32_t >    materialDict;
        std::map< uint64_t, std::string > names;
        std::vector< fb::TransformFb >    transforms;
        std::vector< fb::TextureFb >      textures;
        std::vector< Mesh >               meshes;

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