#include <fbxppch.h>
#include <fbxpstate.h>

void ExportLight(FbxNode* pNode, apemode::Node& n) {
    auto& s = apemode::Get( );
    if ( auto pLight = pNode->GetLight( ) ) {
        switch ( pLight->LightType.Get( ) ) {
            case FbxLight::eVolume:
                return;
        }

        uint32_t lightId = (uint32_t) s.lights.size( );
        n.lightId = lightId;

        s.lights.emplace_back( );
        auto& light = s.lights.back( );

        light.mutate_id( lightId );
        apemode::Mutable( light.mutable_color( ) ).mutate_x( (float) pLight->Color.Get( )[ 0 ] );
        apemode::Mutable( light.mutable_color( ) ).mutate_y( (float) pLight->Color.Get( )[ 1 ] );
        apemode::Mutable( light.mutable_color( ) ).mutate_z( (float) pLight->Color.Get( )[ 2 ] );
        light.mutate_near_attenuation_start( (float) pLight->NearAttenuationStart.Get( ) );
        light.mutate_near_attenuation_end( (float) pLight->NearAttenuationEnd.Get( ) );
        light.mutate_far_attenuation_start( (float) pLight->FarAttenuationStart.Get( ) );
        light.mutate_far_attenuation_end( (float) pLight->FarAttenuationEnd.Get( ) );
        light.mutate_inner_angle( (float) pLight->InnerAngle.Get( ) );
        light.mutate_outer_angle( (float) pLight->OuterAngle.Get( ) );
        light.mutate_decay_start( (float) pLight->DecayStart.Get( ) );
        light.mutate_fog( (float) pLight->Fog.Get( ) );
        light.mutate_casts_shadows( pLight->CastShadows.Get( ) );

        switch ( const auto lightType = pLight->LightType.Get( ) ) {
            case FbxLight::ePoint:
                light.mutate_light_type( apemodefb::ELightTypeFb_Point );
                break;
            case FbxLight::eDirectional:
                light.mutate_light_type( apemodefb::ELightTypeFb_Directional );
                break;
            case FbxLight::eSpot:
                light.mutate_light_type( apemodefb::ELightTypeFb_Spot );
                break;
            case FbxLight::eArea:
                light.mutate_light_type( apemodefb::ELightTypeFb_Area );
                switch ( const auto areaLightShape = pLight->AreaLightShape.Get( ) ) {
                    case FbxLight::eRectangle:
                        light.mutate_area_light_type( apemodefb::EAreaLightTypeFb_Rect );
                        break;
                    case FbxLight::eSphere:
                        light.mutate_area_light_type( apemodefb::EAreaLightTypeFb_Sphere );
                        break;
                } break;
        }

        if ( pLight->GetName( ) && 0 != strlen( pLight->GetName( ) ) )
            light.mutate_name_id( s.PushValue( pLight->GetName( ) ) );
        else
            light.mutate_name_id( s.PushValue( pNode->GetName( ) ) );

        /*FbxLight::EType lightType = pLight->LightType.Get( );

        switch (lightType)
        {
        case FbxLight::ePoint:
            break;
        case FbxLight::eDirectional:
            break;
        case FbxLight::eSpot:
            break;
        case FbxLight::eArea:
            break;
        case FbxLight::eVolume:
            break;
        default:
            break;
        }

        float decayStart = (float) pLight->DecayStart.Get( );
        apemodefb::vec2 innerOuterAngles( (float) pLight->InnerAngle.Get( ), (float) pLight->OuterAngle.Get( ) );
        apemodefb::vec3 color( (float) pLight->Color.Get( )[ 0 ], (float) pLight->Color.Get( )[ 1 ], (float) pLight->Color.Get( )[ 2 ] );*/
    }
}