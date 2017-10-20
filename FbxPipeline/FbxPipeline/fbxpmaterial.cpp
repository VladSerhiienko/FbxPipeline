#include <fbxppch.h>
#include <fbxpstate.h>
#include <scene_generated.h>

std::string FindFile( const char* filepath );
std::string GetFileName( const char* filePath );
void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );

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
    auto& s = apemode::Get( );

    if ( auto c = pScene->GetMaterialCount( ) ) {
        s.materials.reserve( c );
        s.textures.reserve( c * 3 );

        for ( auto i = 0; i < c; ++i ) {
            auto material = pScene->GetMaterial( i );
            const uint32_t id = static_cast< uint32_t >( s.materials.size( ) );

            s.materials.emplace_back( );
            auto& m  = s.materials.back( );
            m.id = id;
            m.nameId = s.PushName( material->GetName( ) );

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
                        case eFbxBool:
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
                        case eFbxBool:
                            m.props.emplace_back(
                                s.PushName( srcProperty.GetNameAsCStr( ) ),
                                apemodefb::EMaterialPropTypeFb_Bool,
                                apemodefb::vec3( static_cast< float >( srcProperty.Get< FbxBool >( ) ), 0, 0 ) );
                            break;
                        case eFbxFloat:
                            m.props.emplace_back(
                                s.PushName( srcProperty.GetNameAsCStr( ) ),
                                apemodefb::EMaterialPropTypeFb_Float,
                                apemodefb::vec3( static_cast< float >( srcProperty.Get< float >( ) ), 0, 0 ) );
                            break;
                        case eFbxDouble:
                            m.props.emplace_back(
                                s.PushName( srcProperty.GetNameAsCStr( ) ),
                                apemodefb::EMaterialPropTypeFb_Float,
                                apemodefb::vec3( static_cast< float >( srcProperty.Get< double >( ) ), 0, 0 ) );
                            break;
                        case eFbxDouble2:
                            m.props.emplace_back(
                                s.PushName( srcProperty.GetNameAsCStr( ) ),
                                apemodefb::EMaterialPropTypeFb_Float2,
                                apemodefb::vec3( static_cast< float >( srcProperty.Get< FbxDouble2 >( )[ 0 ] ),
                                                 static_cast< float >( srcProperty.Get< FbxDouble2 >( )[ 1 ] ),
                                                 0 ) );
                            break;
                        case eFbxDouble3:
                        case eFbxDouble4:
                            m.props.emplace_back(
                                s.PushName( srcProperty.GetNameAsCStr( ) ),
                                apemodefb::EMaterialPropTypeFb_Float3,
                                apemodefb::vec3( static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 0 ] ),
                                                 static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 1 ] ),
                                                 static_cast< float >( srcProperty.Get< FbxDouble3 >( )[ 2 ] ) ) );
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
                            fileUrl = pTexture->GetName( );
                        }

                        s.console->info( "Bound texture: \"{}\" <= \"{}\" <= \"{}\"",
                                         GetFileName( fileUrl.c_str( ) ),
                                         pSrcObj->GetName( ),
                                         srcProperty.GetNameAsCStr( ) );

                        bool found = false;

                        auto fullFilePath = FindFile( fileUrl.c_str( ) );
                        if ( false == fullFilePath.empty( ) ) {
                            s.console->info( "Full path (default): \"{}\"", fullFilePath );
                            s.embedQueue.insert( fullFilePath );
                            found = true;
                        } else {
                            // https://help.sketchfab.com/hc/en-us/articles/202600873-Materials-and-Textures
                            // Anything that is not .JPG or .PNG is converted to .PNG.

                            /* Sketchfab case */
                            fullFilePath = FindFile( ( fileUrl + ".png" ).c_str( ) );
                            if ( false == fullFilePath.empty( ) ) {
                                s.console->info( "Full path (sketchfab): \"{}\"", fullFilePath );
                                s.embedQueue.insert( fullFilePath );
                                found = true;
                            }
                        }

                        if ( false == found ) {
                            s.console->error( "Missing: \"{}\"", GetFileName( fileUrl.c_str( ) ) );
                            s.missingQueue.insert( GetFileName( fileUrl.c_str( ) ) );
                        } else {
                            s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                                     s.PushName( pTexture->GetName( ) ),
                                                     s.PushName( GetFileName( fullFilePath.c_str( ) ) ),
                                                     (apemodefb::EBlendMode) pTexture->GetBlendMode( ),
                                                     (apemodefb::EWrapMode) pTexture->GetWrapModeU( ),
                                                     (apemodefb::EWrapMode) pTexture->GetWrapModeV( ),
                                                     (float) pTexture->GetTranslationU( ),
                                                     (float) pTexture->GetTranslationV( ),
                                                     (float) pTexture->GetScaleU( ),
                                                     (float) pTexture->GetScaleV( ) );

                            m.props.emplace_back( s.PushName( srcProperty.GetNameAsCStr( ) ),
                                                  apemodefb::EMaterialPropTypeFb_Texture,
                                                  apemodefb::vec3( static_cast< float >( s.textures.back( ).id( ) ), 0, 0 ) );

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
    auto& s = apemode::Get( );
    if ( const auto c = node->GetMaterialCount( ) ) {
        n.materialIds.reserve( c );
        for ( auto i = 0; i < c; ++i ) {
            auto material = node->GetMaterial( i );
            auto nameId   = s.PushName( material->GetName( ) );
            assert( s.materialDict.find( nameId ) != s.materialDict.end( ) );
            n.materialIds.push_back( s.materialDict[ nameId ] );
        }
    }
}