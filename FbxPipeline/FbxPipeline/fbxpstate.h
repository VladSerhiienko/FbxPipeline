#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

inline void DebugBreak( ) {
}

namespace apemode {

    /**
     * Flatbuffers generate immutable fields (where mutable are expected) for defined structs.
     * Mostly the usage is: Mutable( some_struct.mutable_some_field( ) ) = some_field_value.
     **/
    template < typename TConst, typename TMutable = typename std::remove_const< TConst >::type >
    inline TMutable& Mutable( TConst& v ) {
        return const_cast< TMutable& >( v );
    }

    struct Skin {
        uint64_t                nameId = (uint64_t) 0;
        std::vector< uint64_t > linkFbxIds;
    };

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
        std::vector< uint32_t >             animCurveIds;
        apemodefb::EIndexTypeFb             indexType;
        uint32_t                            skinId = -1;
    };

    struct Node {
        apemodefb::ECullingType cullingType = apemodefb::ECullingType_CullingOff;
        uint32_t                id          = (uint32_t) -1;
        uint64_t                fbxId       = (uint64_t) 0;
        uint64_t                nameId      = (uint64_t) 0;
        uint32_t                meshId      = (uint32_t) -1;
        uint32_t                lightId     = (uint32_t) -1;
        uint32_t                cameraId    = (uint32_t) -1;
        std::vector< uint32_t > childIds;
        std::vector< uint32_t > materialIds;
        std::vector< uint32_t > curveIds;
    };

    struct AnimStack {
        uint32_t id;
        uint64_t nameId;
    };

    struct AnimLayer {
        uint32_t id;
        uint64_t nameId;
        uint32_t animStackId;
    };

    struct AnimCurveKey {
        float                         time;
        float                         value;
        float                         tangents[ 2 ][ 2 ];
        apemodefb::EInterpolationMode interpolationMode;
    };

    struct AnimCurve {
        uint32_t                      id;
        uint32_t                      animStackId;
        uint32_t                      animLayerId;
        uint32_t                      nodeId;
        uint64_t                      nameId;
        apemodefb::EAnimCurveProperty property;
        apemodefb::EAnimCurveChannel  channel;
        std::vector< AnimCurveKey >   keys;
    };

    struct Material {
        uint32_t                                 id;
        uint64_t                                 nameId;
        std::vector< apemodefb::MaterialPropFb > props;
    };

    using TupleUintUint = std::tuple< uint32_t, uint32_t >;

    struct State;
    State& Get( );
    State& Main( int argc, char** argv );

    struct State {
        bool                                  legacyTriangulationSdk = false;
        FbxManager*                           manager                = nullptr;
        FbxScene*                             scene                  = nullptr;
        std::string                           executableName;
        std::shared_ptr< spdlog::logger >     console;
        flatbuffers::FlatBufferBuilder        builder;
        cxxopts::Options                      options;
        std::string                           fileName;
        std::string                           folderPath;
        std::vector< Node >                   nodes;
        std::vector< Material >               materials;
        std::map< uint64_t, uint32_t >        nodeDict;
        std::map< uint64_t, uint32_t >        textureDict;
        std::map< uint64_t, uint32_t >        materialDict;
        std::map< uint64_t, uint32_t >        animStackDict;
        std::map< uint64_t, uint32_t >        animLayerDict;
        std::map< uint64_t, std::string >     names;
        std::vector< apemodefb::TransformFb > transforms;
        std::vector< apemodefb::TextureFb >   textures;
        std::vector< apemodefb::CameraFb >    cameras;
        std::vector< apemodefb::LightFb >     lights;
        std::vector< Mesh >                   meshes;
        std::vector< AnimStack >              animStacks;
        std::vector< AnimLayer >              animLayers;
        std::vector< AnimCurve >              animCurves;
        std::vector< Skin >                   skins;
        std::vector< std::string >            searchLocations;
        std::set< std::string >               embedQueue;
        std::set< std::string >               missingQueue;
        float                                 resampleFPS       = 24.0f;
        bool                                  reduceKeys        = false;
        bool                                  reduceConstKeys   = false;
        bool                                  propertyCurveSync = true;

        State( );
        ~State( );

        bool     Initialize( );
        void     Release( );
        bool     Load( );
        bool     Finish( );
        uint64_t PushName( std::string const& name );

        friend State& Get( );
        friend State& Main( int argc, char** argv );
    };
}