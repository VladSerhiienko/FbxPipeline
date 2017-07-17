#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace apemode {

    struct Mesh {
        bool                                hasTexcoords = false;
        apemodefb::vec3                     positionMin;
        apemodefb::vec3                     positionMax;
        apemodefb::vec3                     positionOffset;
        apemodefb::vec3                     positionScale;
        apemodefb::vec2                     texcoordMin;
        apemodefb::vec2                     texcoordMax;
        apemodefb::vec2                     texcoordOffset;
        apemodefb::vec2                     texcoordScale;
        std::vector< apemodefb::SubmeshFb > submeshes;
        std::vector< apemodefb::SubsetFb >  subsets;
        std::vector< uint8_t >              indices;
        std::vector< uint8_t >              vertices;
        apemodefb::EIndexTypeFb             indexType;
    };

    struct Node {
        apemodefb::ECullingType  cullingType = apemodefb::ECullingType_CullingOff;
        uint32_t                id          = (uint32_t) -1;
        uint64_t                nameId      = (uint64_t) 0;
        uint32_t                meshId      = (uint32_t) -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
    };

    struct Material {
        uint32_t                          id;
        uint64_t                          nameId;
        std::vector<apemodefb::MaterialPropFb > props;
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
        std::vector<apemodefb::TransformFb >    transforms;
        std::vector<apemodefb::TextureFb >      textures;
        std::vector< Mesh >               meshes;
        std::vector< std::string >        searchLocations;
        std::set< std::string >        embedQueue;

        State( );
        ~State( );

        bool     Initialize( );
        void     Release( );
        bool     Load( );
        bool     Finish( );
        uint64_t PushName( std::string const& name );

        friend State& Get( );
    };
}