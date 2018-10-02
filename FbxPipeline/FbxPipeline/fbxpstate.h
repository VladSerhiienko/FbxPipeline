#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

#ifndef DebugBreak
inline void DebugBreak( ) {
}
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE( arr ) sizeof( arr ) / sizeof( arr[ 0 ] )
#endif

namespace apemode {

    /**
     * Flatbuffers generate immutable fields (where mutable are expected) for defined structs.
     * Mostly the usage is: Mutable( some_struct.mutable_some_field( ) ) = some_field_value.
     **/
    template < typename TConst, typename TMutable = typename std::remove_const< TConst >::type >
    inline TMutable& Mutable( TConst& v ) {
        return const_cast< TMutable& >( v );
    }

    struct FBXPIPELINE_API Skin {
        uint32_t                nameId = (uint64_t) 0;
        std::vector< uint64_t > linkFbxIds;
    };

    /**
     * ValueId class contains value type (string, float, ...) and value index (0, 1, 2, ...).
     * Type indicate the collection (floatValues, stringValues, ...), index indicate the position of the value in the collection.
     * How to unpack:
     * const uint32_t packedId = ... uint32_t id from somewhere;
     * const uint32_t valueType = packedId & 0x000f;
     * const uint32_t valueIndex = ( packedId >> 8 ) & 0x0fff;
     **/
    struct FBXPIPELINE_API ValueId {
        uint32_t valueType : 8;
        uint32_t valueIndex : 24;

        ValueId( uint32_t value );
        ValueId( uint8_t type, uint32_t index );
        operator uint32_t( ) const;
    };

    static_assert( sizeof( ValueId ) == sizeof( uint32_t ), "Not packed." );
    static_assert( apemodefb::EValueTypeFb_MAX < 255, "Does not fit into a byte." );

    struct FBXPIPELINE_API Mesh {
        bool                                hasTexcoords = false;
        apemodefb::Vec3Fb                   positionMin;
        apemodefb::Vec3Fb                   positionMax;
        apemodefb::Vec3Fb                   positionOffset;
        apemodefb::Vec3Fb                   positionScale;
        apemodefb::Vec2Fb                   texcoordMin;
        apemodefb::Vec2Fb                   texcoordMax;
        apemodefb::Vec2Fb                   texcoordOffset;
        apemodefb::Vec2Fb                   texcoordScale;
        std::vector< apemodefb::SubmeshFb > submeshes;
        std::vector< apemodefb::SubsetFb >  subsets;
        std::vector< uint8_t >              indices;
        std::vector< uint8_t >              vertices;
        std::vector< uint32_t >             animCurveIds;
        apemodefb::EIndexTypeFb             indexType;
        uint32_t                            skinId = -1;
    };

    struct FBXPIPELINE_API Node {
        apemodefb::ECullingTypeFb cullingType = apemodefb::ECullingTypeFb_CullingOff;
        uint32_t                  id          = (uint32_t) -1;
        uint64_t                  fbxId       = (uint64_t) 0;
        uint32_t                  nameId      = (uint64_t) 0;
        uint32_t                  meshId      = (uint32_t) -1;
        uint32_t                  lightId     = (uint32_t) -1;
        uint32_t                  cameraId    = (uint32_t) -1;
        std::vector< uint32_t >   childIds;
        std::vector< uint32_t >   materialIds;
        std::vector< uint32_t >   curveIds;
    };

    struct FBXPIPELINE_API AnimStack {
        uint32_t id;
        uint32_t nameId;
    };

    struct FBXPIPELINE_API AnimLayer {
        uint32_t id;
        uint32_t nameId;
        uint32_t animStackId;
    };

    struct FBXPIPELINE_API AnimCurveKey {
        float                           time;
        float                           value;
        float                           arriveTangent;
        float                           leaveTangent;
        apemodefb::EInterpolationModeFb interpolationMode;
    };

    struct FBXPIPELINE_API AnimCurve {
        uint32_t                        id;
        uint32_t                        animStackId;
        uint32_t                        animLayerId;
        uint32_t                        nodeId;
        uint32_t                        nameId;
        apemodefb::EAnimCurvePropertyFb property;
        apemodefb::EAnimCurveChannelFb  channel;
        std::vector< AnimCurveKey >     keys;
    };

    struct FBXPIPELINE_API Material {
        uint32_t                                 id;
        uint32_t                                 nameId;
        std::vector< apemodefb::MaterialPropFb > properties;
        std::vector< apemodefb::MaterialPropFb > textureProperties;
    };

    struct FBXPIPELINE_API File {
        uint32_t    id;
        uint32_t    nameId;
        std::string fullPath;
    };

    struct FBXPIPELINE_API State {
        FbxManager*                           manager = nullptr;
        FbxScene*                             scene   = nullptr;
        std::string                           executableName;
        std::shared_ptr< spdlog::logger >     console;
        flatbuffers::FlatBufferBuilder        builder;
        cxxopts::Options                      options;
        std::string                           fileName;
        std::string                           folderPath;
        std::vector< Node >                   nodes;
        std::vector< Material >               materials;
        std::vector< File >                   embeddedFiles;
        std::map< uint64_t, uint32_t >        nodeDict;
        std::map< uint64_t, uint32_t >        textureDict;
        std::map< uint64_t, uint32_t >        materialDict;
        std::map< uint64_t, uint32_t >        animStackDict;
        std::map< uint64_t, uint32_t >        animLayerDict;
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
        std::vector< bool >                   boolValues;
        std::vector< int32_t >                intValues;
        std::vector< float >                  floatValues;
        std::vector< std::string >            stringValues;

        std::vector< std::function< void( apemode::State*, std::string ) > > extensions;

        bool forceResampling         = false;
        float resampleFPS            = 24.0f;
        bool  reduceKeys             = false;
        bool  reduceConstKeys        = false;
        bool  propertyCurveSync      = true;
        bool  legacyTriangulationSdk = false;

        State( );
        ~State( );

        bool Initialize( );
        void Release( );
        bool Load( );
        bool Finalize( );

        ValueId  PushValue( const char* value );
        ValueId  PushValue( const std::string& value );
        ValueId  PushValue( const int32_t value );
        ValueId  PushValue( const float value );
        ValueId  PushValue( const float x, const float y );
        ValueId  PushValue( const float x, const float y, const float z );
        ValueId  PushValue( const float x, const float y, const float z, const float w );
        ValueId  PushValue( const bool value );
        uint32_t PushValue( const apemodefb::TextureFb& value );

        uint32_t EmbedFile( const std::string fullPath );

        static State& Get( );
        static State& Main( int argc, const char**& argv );
    };
} // namespace apemode