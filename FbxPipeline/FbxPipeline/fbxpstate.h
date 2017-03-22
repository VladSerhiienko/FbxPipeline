#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace fbxp {

    struct Mesh {
        bool                               hasTexcoords = false;
        fbxp::fb::vec3                     positionMin;
        fbxp::fb::vec3                     positionMax;
        fbxp::fb::vec3                     positionOffset;
        fbxp::fb::vec3                     positionScale;
        fbxp::fb::vec2                     texcoordMin;
        fbxp::fb::vec2                     texcoordMax;
        fbxp::fb::vec2                     texcoordOffset;
        fbxp::fb::vec2                     texcoordScale;
        std::vector< fbxp::fb::SubmeshFb > submeshes;
        std::vector< fbxp::fb::SubsetFb >  subsets;
        std::vector< fbxp::fb::SubsetFb >  subsetsPolies;
        std::vector< uint8_t >             subsetIndices;
        std::vector< uint8_t >             vertices;
        std::vector< uint8_t >             indices;
        fbxp::fb::EIndexTypeFb             subsetIndexType;
    };

    struct Node {
        fbxp::fb::ECullingType  cullingType = fbxp::fb::ECullingType_CullingOff;
        uint32_t                id          = (uint32_t) -1;
        uint64_t                nameId      = (uint64_t) 0;
        uint32_t                meshId      = (uint32_t) -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
    };

    struct Material {
        uint32_t                          id;
        uint64_t                          nameId;
        std::vector< fb::MaterialPropFb > props;
    };

    using TupleUintUint = std::tuple< uint32_t, uint32_t >;

    struct State {
        bool                              legacyTriangulationSdk = false;
        fbxsdk::FbxManager*               manager                = nullptr;
        fbxsdk::FbxScene*                 scene                  = nullptr;
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