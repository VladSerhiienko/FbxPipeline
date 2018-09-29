
#include <fbxppch.h>
#include <fbxpstate.h>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

namespace py = pybind11;

namespace apemode {

    void LogInfo( const std::string message ) {
        auto& s = apemode::State::Get( );
        assert( s.console );
        s.console->info( "[py] " + message );
    }

    void LogError( const std::string message ) {
        auto& s = apemode::State::Get( );
        assert( s.console );
        s.console->error( "[py] " + message );
    }

    void RegisterExtension( std::function< void( apemode::State*, std::string ) > func ) {
        auto& s = apemode::State::Get( );
        s.console->info( "Registering extension function ..." );
        s.extensions.push_back( func );
    }

} // namespace apemode

std::string ResolveFullPath( const char* path );
std::string FindFile( const char* filepath );

PYBIND11_MAKE_OPAQUE( std::vector< bool > );
PYBIND11_MAKE_OPAQUE( std::vector< int32_t > );
PYBIND11_MAKE_OPAQUE( std::vector< float_t > );
PYBIND11_MAKE_OPAQUE( std::vector< std::string > );
PYBIND11_MAKE_OPAQUE( std::vector< apemodefb::TextureFb > );
PYBIND11_MAKE_OPAQUE( std::vector< apemodefb::MaterialPropFb > );
PYBIND11_MAKE_OPAQUE( std::vector< apemode::Material > );

void InitializePyModule( py::module & m ) {
    m.doc( ) = "python extension base for FbxPipeline";

    py::bind_vector< std::vector< bool > >                      ( m, "BoolVector" );
    py::bind_vector< std::vector< int32_t > >                   ( m, "IntVector" );
    py::bind_vector< std::vector< float_t > >                   ( m, "FloatVector" );
    py::bind_vector< std::vector< std::string > >               ( m, "StringVector" );
    py::bind_vector< std::vector< apemode::Material > >         ( m, "MaterialVector" );
    py::bind_vector< std::vector< apemodefb::TextureFb > >      ( m, "TextureFbVector" );
    py::bind_vector< std::vector< apemodefb::MaterialPropFb > > ( m, "MaterialPropFbVector" );

    py::enum_< apemodefb::EValueTypeFb >( m, "EValueTypeFb" )
        .value( "Bool",     apemodefb::EValueTypeFb::EValueTypeFb_Bool )
        .value( "Int",      apemodefb::EValueTypeFb::EValueTypeFb_Int )
        .value( "Float",    apemodefb::EValueTypeFb::EValueTypeFb_Float )
        .value( "Float2",   apemodefb::EValueTypeFb::EValueTypeFb_Float2 )
        .value( "Float3",   apemodefb::EValueTypeFb::EValueTypeFb_Float3 )
        .value( "Float4",   apemodefb::EValueTypeFb::EValueTypeFb_Float4 )
        .value( "String",   apemodefb::EValueTypeFb::EValueTypeFb_String )
        .export_values( );

    py::enum_< apemodefb::EAlphaSourceFb >( m, "EAlphaSourceFb" )
        .value( "Unknown",      apemodefb::EAlphaSourceFb::EAlphaSourceFb_None )
        .value( "RGBIntensity", apemodefb::EAlphaSourceFb::EAlphaSourceFb_RGBIntensity )
        .value( "Black",        apemodefb::EAlphaSourceFb::EAlphaSourceFb_Black )
        .export_values( );

    py::enum_< apemodefb::EBlendModeFb >( m, "EBlendModeFb" )
        .value( "Translucent",  apemodefb::EBlendModeFb::EBlendModeFb_Translucent )
        .value( "Additive",     apemodefb::EBlendModeFb::EBlendModeFb_Additive )
        .value( "Modulate",     apemodefb::EBlendModeFb::EBlendModeFb_Modulate )
        .value( "Modulate2",    apemodefb::EBlendModeFb::EBlendModeFb_Modulate2 )
        .export_values( );

    py::enum_< apemodefb::EMappingTypeFb >( m, "EMappingTypeFb" )
        .value( "Null",         apemodefb::EMappingTypeFb::EMappingTypeFb_Null )
        .value( "Spherical",    apemodefb::EMappingTypeFb::EMappingTypeFb_Spherical )
        .value( "Cylindrical",  apemodefb::EMappingTypeFb::EMappingTypeFb_Cylindrical )
        .value( "Box",          apemodefb::EMappingTypeFb::EMappingTypeFb_Box )
        .value( "Face",         apemodefb::EMappingTypeFb::EMappingTypeFb_Face )
        .value( "UV",           apemodefb::EMappingTypeFb::EMappingTypeFb_UV )
        .value( "Environment",  apemodefb::EMappingTypeFb::EMappingTypeFb_Environment )
        .export_values( );

    py::enum_< apemodefb::EPlanarMappingNormalFb >( m, "EPlanarMappingNormalFb" )
        .value( "PlanarNormalX", apemodefb::EPlanarMappingNormalFb::EPlanarMappingNormalFb_PlanarNormalX )
        .value( "PlanarNormalY", apemodefb::EPlanarMappingNormalFb::EPlanarMappingNormalFb_PlanarNormalY )
        .value( "PlanarNormalZ", apemodefb::EPlanarMappingNormalFb::EPlanarMappingNormalFb_PlanarNormalZ )
        .export_values( );

    py::enum_< apemodefb::EWrapModeFb >( m, "EWrapModeFb" )
        .value( "Repeat",   apemodefb::EWrapModeFb::EWrapModeFb_Repeat )
        .value( "Clamp",    apemodefb::EWrapModeFb::EWrapModeFb_Clamp )
        .export_values( );

    py::enum_< apemodefb::ETextureUseFb >( m, "ETextureUseFb" )
        .value( "Standard",                 apemodefb::ETextureUseFb::ETextureUseFb_Standard )
        .value( "ShadowMap",                apemodefb::ETextureUseFb::ETextureUseFb_ShadowMap )
        .value( "LightMap",                 apemodefb::ETextureUseFb::ETextureUseFb_LightMap )
        .value( "SphericalReflectionMap",   apemodefb::ETextureUseFb::ETextureUseFb_SphericalReflectionMap )
        .value( "SphereReflectionMap",      apemodefb::ETextureUseFb::ETextureUseFb_SphereReflectionMap )
        .value( "BumpNormalMap",            apemodefb::ETextureUseFb::ETextureUseFb_BumpNormalMap )
        .export_values( );

    py::class_< apemodefb::TextureFb > textureFbClass( m, "TextureFb" );
    textureFbClass.def( py::init<>( ) )
        .def_property( "id",                    &apemodefb::TextureFb::id, &apemodefb::TextureFb::mutate_id )
        .def_property( "name_id",               &apemodefb::TextureFb::name_id, &apemodefb::TextureFb::mutate_name_id )
        .def_property( "alpha_source",          &apemodefb::TextureFb::alpha_source, &apemodefb::TextureFb::mutate_alpha_source )
        .def_property( "blend_mode",            &apemodefb::TextureFb::blend_mode, &apemodefb::TextureFb::mutate_blend_mode )
        .def_property( "cropping_bottom",       &apemodefb::TextureFb::cropping_bottom, &apemodefb::TextureFb::mutate_cropping_bottom )
        .def_property( "cropping_left",         &apemodefb::TextureFb::cropping_left, &apemodefb::TextureFb::mutate_cropping_left )
        .def_property( "cropping_right",        &apemodefb::TextureFb::cropping_right, &apemodefb::TextureFb::mutate_cropping_right )
        .def_property( "cropping_top",          &apemodefb::TextureFb::cropping_top, &apemodefb::TextureFb::mutate_cropping_top )
        .def_property( "file_id",               &apemodefb::TextureFb::file_id, &apemodefb::TextureFb::mutate_file_id )
        .def_property( "mapping_type",          &apemodefb::TextureFb::mapping_type, &apemodefb::TextureFb::mutate_mapping_type )
        .def_property( "offset_u",              &apemodefb::TextureFb::offset_u, &apemodefb::TextureFb::mutate_offset_u )
        .def_property( "offset_v",              &apemodefb::TextureFb::offset_v, &apemodefb::TextureFb::mutate_offset_v )
        .def_property( "planar_mapping_normal", &apemodefb::TextureFb::planar_mapping_normal, &apemodefb::TextureFb::mutate_planar_mapping_normal )
        .def_property( "premultiplied_alpha",   &apemodefb::TextureFb::premultiplied_alpha, &apemodefb::TextureFb::mutate_premultiplied_alpha )
        .def_property( "rotation_u",            &apemodefb::TextureFb::rotation_u, &apemodefb::TextureFb::mutate_rotation_u )
        .def_property( "rotation_v",            &apemodefb::TextureFb::rotation_v, &apemodefb::TextureFb::mutate_rotation_v )
        .def_property( "rotation_w",            &apemodefb::TextureFb::rotation_w, &apemodefb::TextureFb::mutate_rotation_w )
        .def_property( "scale_u",               &apemodefb::TextureFb::scale_u, &apemodefb::TextureFb::mutate_scale_u )
        .def_property( "scale_v",               &apemodefb::TextureFb::scale_v, &apemodefb::TextureFb::mutate_scale_v )
        .def_property( "swap_uv",               &apemodefb::TextureFb::swap_uv, &apemodefb::TextureFb::mutate_swap_uv )
        .def_property( "texture_type_id",       &apemodefb::TextureFb::texture_type_id, &apemodefb::TextureFb::mutate_texture_type_id )
        .def_property( "texture_use",           &apemodefb::TextureFb::texture_use, &apemodefb::TextureFb::mutate_texture_use )
        .def_property( "wipe_mode",             &apemodefb::TextureFb::wipe_mode, &apemodefb::TextureFb::mutate_wipe_mode )
        .def_property( "wrap_mode_u",           &apemodefb::TextureFb::wrap_mode_u, &apemodefb::TextureFb::mutate_wrap_mode_u )
        .def_property( "wrap_mode_v",           &apemodefb::TextureFb::wrap_mode_v, &apemodefb::TextureFb::mutate_wrap_mode_v );


    py::class_< apemodefb::MaterialPropFb > materialPropFbClass(m, "MaterialPropFb");
    materialPropFbClass.def( py::init<>( ) )
        .def_property( "name_id",               &apemodefb::MaterialPropFb::name_id, &apemodefb::MaterialPropFb::mutate_name_id )
        .def_property( "value_id",              &apemodefb::MaterialPropFb::value_id, &apemodefb::MaterialPropFb::mutate_value_id );

    py::class_< apemode::Material > materialClass( m, "Material" );
    materialClass.def( py::init<>( ) )
        .def_readwrite( "id",                   &apemode::Material::id )
        .def_readwrite( "name_id",              &apemode::Material::nameId )
        .def_readwrite( "properties",           &apemode::Material::properties )
        .def_readwrite( "texture_properties",   &apemode::Material::textureProperties );

    py::class_< apemode::State > stateClass( m, "State" );
    stateClass.def( py::init<>( ) )
        .def_readwrite( "bool_values",      &apemode::State::boolValues )
        .def_readwrite( "int_values",       &apemode::State::intValues )
        .def_readwrite( "float_values",     &apemode::State::floatValues )
        .def_readwrite( "string_values",    &apemode::State::stringValues )
        .def_readwrite( "textures",         &apemode::State::textures )
        .def_readwrite( "materials",        &apemode::State::materials )
        .def( "embed_file",                 &apemode::State::EmbedFile )
        .def( "push_texture",               []( apemode::State* pState, const apemodefb::TextureFb t ) { return (uint32_t) pState->PushValue( t ); } )
        .def( "push_string",                []( apemode::State* pState, std::string s ) { return (uint32_t) pState->PushValue( s ); } )
        .def( "push_float4",                []( apemode::State* pState, const float x, const float y, const float z, const float w ) { return (uint32_t) pState->PushValue( x, y, z, w ); } )
        .def( "push_float3",                []( apemode::State* pState, const float x, const float y, const float z ) { return (uint32_t) pState->PushValue( x, y, z ); } )
        .def( "push_float2",                []( apemode::State* pState, const float x, const float y ) { return (uint32_t) pState->PushValue( x, y ); } )
        .def( "push_float",                 []( apemode::State* pState, const float v ) { return (uint32_t) pState->PushValue( v ); } )
        .def( "push_bool",                  []( apemode::State* pState, const bool v ) { return (uint32_t) pState->PushValue( v ); } )
        .def( "push_int",                   []( apemode::State* pState, const int32_t v ) { return (uint32_t) pState->PushValue( v ); } );

    m.def( "register_extension",            &apemode::RegisterExtension );
    m.def( "log_info",                      &apemode::LogInfo );
    m.def( "log_error",                     &apemode::LogError );
    m.def( "find_file",                     []( const std::string s ) { return FindFile( s.c_str( ) ); } );
    m.def( "resolve_full_path",             []( const std::string s ) { return ResolveFullPath( s.c_str( ) ); } );
}

// TODO: PYBIND11_MODULE
PYBIND11_EMBEDDED_MODULE( FbxPipeline, m ) {
    InitializePyModule( m );
}
