
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <scene_generated.h>

namespace apemodefb {
    inline uint32_t MaterialPropertyGetIndex( const uint32_t packed ) {
        const uint32_t valueIndex = ( packed >> 8 ) & 0x0fff;
        return valueIndex;
    }

    inline apemodefb::EValueTypeFb MaterialPropertyGetType( const uint32_t packed ) {
        const uint32_t valueType = packed & 0x000f;
        return apemodefb::EValueTypeFb( valueType );
    }

    template < typename T >
    inline bool FlatbuffersTVectorIsNotNullAndNotEmpty( const flatbuffers::Vector< T > *pVector ) {
        return pVector && pVector->size( );
    }

    template < typename T >
    inline typename flatbuffers::Vector< T >::return_type FlatbuffersTVectorGetAtIndex( const flatbuffers::Vector< T > *pVector,
                                                                                        const size_t itemIndex ) {
        assert( pVector );
        const size_t vectorSize = pVector->size( );

        assert( vectorSize > itemIndex );
        return pVector->Get( static_cast< flatbuffers::uoffset_t >( itemIndex ) );
    }

    inline std::string GetStringProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->str( );
    }

    inline const char *GetCStringProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_String == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->string_values( ), valueIndex )->c_str( );
    }

    inline bool GetBoolProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_Bool == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->bool_values( ), valueIndex );
    }

    inline float GetScalarProperty( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );
        assert( apemodefb::EValueTypeFb_Float == MaterialPropertyGetType( valueId ) );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex );
    }

    inline apemodefb::Vec2Fb GetVec2Property( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );

        const auto valueType = MaterialPropertyGetType( valueId );
        assert( apemodefb::EValueTypeFb_Float2 == valueType );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return apemodefb::Vec2Fb( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                                  FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ) );
    }

    inline apemodefb::Vec3Fb GetVec3Property( const apemodefb::SceneFb *pScene, const uint32_t valueId ) {
        assert( pScene );

        const auto valueType = MaterialPropertyGetType( valueId );
        assert( apemodefb::EValueTypeFb_Float3 == valueType );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return apemodefb::Vec3Fb( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                         FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ),
                         FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 2 ) );
    }

    inline apemodefb::Vec4Fb GetVec4Property( const apemodefb::SceneFb *pScene, const uint32_t valueId, const float defaultW = 1.0f ) {
        assert( pScene );

        const auto valueType = MaterialPropertyGetType( valueId );
        assert( apemodefb::EValueTypeFb_Float3 == valueType || apemodefb::EValueTypeFb_Float4 == valueType );

        const uint32_t valueIndex = MaterialPropertyGetIndex( valueId );
        return apemodefb::Vec4Fb( FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex ),
                                  FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 1 ),
                                  FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 2 ),
                                  apemodefb::EValueTypeFb_Float4 == valueType
                                      ? FlatbuffersTVectorGetAtIndex( pScene->float_values( ), valueIndex + 3 )
                                      : defaultW );
    }
} // namespace apemodefb