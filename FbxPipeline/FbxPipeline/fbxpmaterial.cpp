#include <fbxppch.h>
#include <fbxpstate.h>
#include <scene_generated.h>

std::string FindFile( const char* filepath );
std::string GetFileName( const char* filePath );
std::string ReplaceExtension( const char* path, const char* extension );

template < typename TPropertyCallbackFn, typename TObjectCallbackFn >
void ScanConnectedSrc( FbxObject*          pPropObj,
                       FbxProperty         srcProperty,
                       TPropertyCallbackFn propertyCallback,
                       TObjectCallbackFn   objectCallback ) {
    while ( srcProperty.IsValid( ) ) {

        /* Properties can have inner properties, but in this case I'm not sure what property object is. */
        int srcPropertyCount = srcProperty.GetSrcPropertyCount( );
        for ( int srcPropIndex = 0; srcPropIndex < srcPropertyCount; ++srcPropIndex ) {
            ScanConnectedSrc( pPropObj, srcProperty.GetSrcProperty( srcPropIndex ), propertyCallback, objectCallback );
        }

        const int srcObjCount = srcProperty.GetSrcObjectCount( );
        propertyCallback( srcProperty, srcObjCount );

        for ( int srcObjIndex = 0; srcObjIndex < srcObjCount; ++srcObjIndex ) {
            auto pSrcObj = srcProperty.GetSrcObject( srcObjIndex );
            objectCallback( srcProperty, pSrcObj, srcObjIndex );
        }

        srcProperty = pPropObj->GetNextProperty( srcProperty );
    }
}


void ExportMaterials( FbxScene* pScene ) {
    auto& s = apemode::State::Get( );

    if ( auto c = pScene->GetMaterialCount( ) ) {
        s.materials.reserve( c );
        s.textures.reserve( c * 3 );

        for ( auto i = 0; i < c; ++i ) {
            auto material = pScene->GetMaterial( i );
            const uint32_t id = static_cast< uint32_t >( s.materials.size( ) );

            s.materials.emplace_back( );
            auto& m  = s.materials.back( );
            m.id = id;
            m.nameId = s.PushValue( material->GetName( ) );

            s.materialDict[ m.nameId ] = id;
            s.console->info( "Material: \"{}\"", material->GetName( ) );

            /*
            https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures

            Diffuse / Albedo / Base Color: 'diffuse', 'albedo', 'basecolor'
            Metalness: 'metalness', 'metallic', 'metal', 'm'
            Specular: 'specular', 'spec', 's'
            Specular F0: 'specularf0', 'f0'
            Roughness: 'roughness', 'rough', 'r'
            Glossiness: 'glossiness', 'glossness', 'gloss', 'g', 'glossy'
            AO: 'ambient occlusion', 'ao', 'occlusion', 'lightmap', 'diffuseintensity'
            Cavity: 'cavity'
            Normal Map: 'normal', 'nrm', 'normalmap'
            Bump Map: 'bump', 'bumpmap', 'heightmap'
            Emission: 'emission', 'emit', 'emissive'
            Transparency: 'transparency', 'transparent', 'opacity', 'mask', 'alpha'
            */

            ScanConnectedSrc(
                material,
                material->GetFirstProperty( ),
                [&]( FbxProperty& srcProperty, int srcObjCount ) {

                    bool supported = false;
                    switch ( srcProperty.GetPropertyDataType( ).GetType( ) ) {
                        case eFbxString:
                        case eFbxBool:
                        case eFbxInt:
                        case eFbxUInt:
                        case eFbxShort:
                        case eFbxUShort:
                        case eFbxFloat:
                        case eFbxDouble:
                        case eFbxDouble2:
                        case eFbxDouble3:
                        case eFbxDouble4:
                            supported = true;
                            break;
                    }

                    auto srcPropertyName = srcProperty.GetName( );
                    auto srcPropertyTypeName = srcProperty.GetPropertyDataType( ).GetName( );
                    spdlog::level::level_enum level = supported ? spdlog::level::info : spdlog::level::warn;
                    const char* supportedTag = supported ? "supported" : "unsupported";

                    s.console->log( level,
                                    "Prop {}: {}: {} ({})",
                                    supportedTag,
                                    srcProperty.GetNameAsCStr( ),
                                    srcPropertyTypeName,
                                    srcObjCount );

                    switch ( srcProperty.GetPropertyDataType( ).GetType( ) ) {
                        case eFbxString:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( srcProperty.Get< FbxString >( ).Buffer( ) ) );
                            break;
                        case eFbxBool:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( srcProperty.Get< FbxBool >( ) ) );
                            break;
                        case eFbxInt:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( srcProperty.Get< int32_t >( ) ) );
                            break;
                        case eFbxUInt:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< int32_t >( srcProperty.Get< uint32_t >( ) ) ) );
                            break;
                        case eFbxShort:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< int32_t >( srcProperty.Get< int16_t >( ) ) ) );
                            break;
                        case eFbxUShort:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< int32_t >( srcProperty.Get< uint16_t >( ) ) ) );
                            break;
                        case eFbxFloat:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( srcProperty.Get< float >( ) ) );
                            break;
                        case eFbxDouble:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< float >( srcProperty.Get< double >( ) ) ) );
                            break;
                        case eFbxDouble2:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< float >( srcProperty.Get< FbxDouble2 >( )[ 0 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble2 >( )[ 1 ] ) ) );
                            break;
                        case eFbxDouble3:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 0 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 1 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 2 ] ) ) );
                            break;
                        case eFbxDouble4:
                            m.properties.emplace_back(
                                s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                s.PushValue( static_cast< float >( srcProperty.Get< FbxDouble4 >( )[ 0 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble4 >( )[ 1 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble4 >( )[ 2 ] ),
                                             static_cast< float >( srcProperty.Get< FbxDouble4 >( )[ 3 ] ) ) );
                            break;

                        default:
                            break;
                    }
                },
                [&]( FbxProperty& srcProperty, FbxObject* pSrcObj, int srcObjIndex ) {
                    if ( auto pTexture = FbxCast< FbxTexture >( pSrcObj ) ) {
                        std::string fileUrl = pTexture->GetUrl( ).Buffer();

                        if ( fileUrl.empty( ) ) {
                            if ( auto pFileTexture = FbxCast< FbxFileTexture >( pTexture ) )
                                fileUrl = pFileTexture->GetFileName( );
                        }

                        if ( fileUrl.empty( ) ) {
                            if ( auto pFileTexture = FbxCast< FbxFileTexture >( pTexture ) )
                                fileUrl = pFileTexture->GetRelativeFileName( );
                        }

                        if ( fileUrl.empty( ) ) {
                            fileUrl = pTexture->GetName( );
                        }

                        s.console->info( "Bound texture: \"{}\" <= \"{}\" <= \"{}\"",
                                         GetFileName( fileUrl.c_str( ) ),
                                         pSrcObj->GetName( ),
                                         srcProperty.GetNameAsCStr( ) );

                        uint32_t fileId = std::numeric_limits< uint32_t >::max( );

                        auto fullFilePath = FindFile( fileUrl.c_str( ) );
                        if ( false == fullFilePath.empty( ) ) {
                            s.console->info( "Full path (default): \"{}\"", fullFilePath );
                            fileId = s.EmbedFile( fullFilePath );
                        } else {
                            // https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures
                            // Anything that is not .JPG or .PNG is converted to .PNG.

                            /* Sketchfab case */
                            fullFilePath = FindFile( ( fileUrl + ".png" ).c_str( ) );
                            if ( false == fullFilePath.empty( ) ) {
                                s.console->info( "Full path (sketchfab): \"{}\"", fullFilePath );
                                fileId = s.EmbedFile( fullFilePath );
                            } else {

                                fullFilePath = FindFile( ReplaceExtension( fileUrl.c_str( ), ".png" ).c_str( ) );
                                if ( false == fullFilePath.empty( ) ) {
                                    s.console->info( "Full path (sketchfab): \"{}\"", fullFilePath );
                                    fileId = s.EmbedFile( fullFilePath );
                                }
                            }
                        }

                        if ( fileId == std::numeric_limits< uint32_t >::max( ) ) {
                            s.console->error( "Missing: \"{}\"", GetFileName( fileUrl.c_str( ) ) );
                        } else {
                            s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                                     s.PushValue( pTexture->GetName( ) ),
                                                     fileId,
                                                     s.PushValue( pTexture->GetTextureType( ).Buffer( ) ),
                                                     (apemodefb::EBlendModeFb) pTexture->GetBlendMode( ),
                                                     (apemodefb::EWrapModeFb) pTexture->GetWrapModeU( ),
                                                     (apemodefb::EWrapModeFb) pTexture->GetWrapModeV( ),
                                                     (float) pTexture->GetTranslationU( ),
                                                     (float) pTexture->GetTranslationV( ),
                                                     (float) pTexture->GetScaleU( ),
                                                     (float) pTexture->GetScaleV( ),
                                                     pTexture->GetCroppingBottom( ),
                                                     pTexture->GetCroppingLeft( ),
                                                     pTexture->GetCroppingRight( ),
                                                     pTexture->GetCroppingTop( ),
                                                     (float) pTexture->GetRotationU( ),
                                                     (float) pTexture->GetRotationV( ),
                                                     (float) pTexture->GetRotationW( ),
                                                     pTexture->GetSwapUV( ),
                                                     pTexture->GetWipeMode( ),
                                                     pTexture->GetPremultiplyAlpha( ),
                                                     (apemodefb::EAlphaSourceFb) pTexture->GetAlphaSource( ),
                                                     (apemodefb::ETextureUseFb) pTexture->GetTextureUse( ),
                                                     (apemodefb::EMappingTypeFb) pTexture->GetMappingType( ),
                                                     (apemodefb::EPlanarMappingNormalFb) pTexture->GetPlanarMappingNormal( ) );

                            m.textureProperties.emplace_back( s.PushValue( srcProperty.GetNameAsCStr( ) ),
                                                              s.textures.back( ).id( ) );

                            s.console->info( "Found texture \"{}\" (\"{}\") (\"{}\")",
                                             pTexture->GetName( ),
                                             GetFileName( fullFilePath.c_str( ) ).c_str( ),
                                             srcProperty.GetName( ).Buffer( ) );
                        }
                    }
                } );
        }
    }
}

void ExportMaterials( FbxNode* node, apemode::Node& n ) {
    auto& s = apemode::State::Get( );
    if ( const auto c = node->GetMaterialCount( ) ) {
        n.materialIds.reserve( c );
        for ( auto i = 0; i < c; ++i ) {
            auto material = node->GetMaterial( i );
            auto nameId   = s.PushValue( material->GetName( ) );
            assert( s.materialDict.find( nameId ) != s.materialDict.end( ) );
            n.materialIds.push_back( s.materialDict[ nameId ] );
        }
    }
}