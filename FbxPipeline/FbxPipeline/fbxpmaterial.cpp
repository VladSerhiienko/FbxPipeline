#include <fbxppch.h>
#include <fbxpstate.h>
#include <scene_generated.h>

std::string FindFile( const char* filepath );
std::string GetFileName( const char* filePath );
void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );

template < typename TFbxMaterial >
void ExportMaterial( FbxSurfaceMaterial* material, apemode::Material& m ) {
    (void) material;
    (void) m;
}

template <>
void ExportMaterial< FbxSurfaceLambert >( FbxSurfaceMaterial* material, apemode::Material& m ) {
    auto& s  = apemode::Get( );
    auto  mm = static_cast< FbxSurfaceLambert* >( material );

    m.props.reserve( 12 );

    for ( auto& p : {mm->AmbientFactor, mm->DiffuseFactor, mm->DisplacementFactor, mm->BumpFactor, mm->EmissiveFactor, mm->TransparencyFactor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Scalar,
                              apemodefb::vec3( static_cast< float >( p.Get( ) ), 0, 0 ) );
    }

    for ( auto& p : {mm->Ambient, mm->Diffuse, mm->DisplacementColor, mm->Bump, mm->Emissive, mm->TransparentColor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Color,
                              apemodefb::vec3( static_cast< float >( p.Get( )[ 0 ] ),
                                              static_cast< float >( p.Get( )[ 1 ] ),
                                              static_cast< float >( p.Get( )[ 2 ] ) ) );
    }
}

template <>
void ExportMaterial< FbxSurfacePhong >( FbxSurfaceMaterial* material, apemode::Material& m ) {
    m.props.reserve( 16 );

    ExportMaterial< FbxSurfaceLambert >( material, m );

    auto& s  = apemode::Get( );
    auto  mm = static_cast< FbxSurfacePhong* >( material );

    for ( auto& p : {mm->SpecularFactor, mm->ReflectionFactor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Scalar,
                              apemodefb::vec3( static_cast< float >( p.Get( ) ), 0, 0 ) );
    }

    for ( auto& p : {mm->Specular, mm->Reflection} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Color,
                              apemodefb::vec3( static_cast< float >( p.Get( )[ 0 ] ),
                                              static_cast< float >( p.Get( )[ 1 ] ),
                                              static_cast< float >( p.Get( )[ 2 ] ) ) );
    }
}

void ExportVideo( std::string const& pn, apemode::Material& m, FbxProperty& pp, FbxVideo* v ) {
    auto& s = apemode::Get( );

    std::string url = v->GetFileName( );
    if ( url.empty( ) ) {
        url = v->GetUrl( );
    }
    if ( !url.empty( ) ) {
        s.embedQueue.insert( FindFile( url.c_str( ) ) );

        s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                 s.PushName( v->GetName( ) ),
                                 s.PushName( GetFileName( url.c_str( ) ) ),
                                 apemodefb::EBlendMode::EBlendMode_Over,
                                 apemodefb::EWrapMode::EWrapMode_Clamp,
                                 apemodefb::EWrapMode::EWrapMode_Clamp,
                                 (float) 0,
                                 (float) 0,
                                 (float) 1,
                                 (float) 1 );

        m.props.emplace_back( s.PushName( pp.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Video,
                              apemodefb::vec3( static_cast< float >( s.textures.back( ).id( ) ), 0, 0 ) );

        s.console->info( "Found video \"{}\" (\"{}\") (\"{}\")",
                         v->GetName( ),
                         GetFileName( url.c_str( ) ).c_str( ),
                         pp.GetName( ).Buffer( ) );
    }
}

void ExportTexture( std::string const& pn, apemode::Material& m, FbxProperty& pp, FbxTexture* t ) {
    auto& s = apemode::Get( );
    std::string url = t->GetUrl( );

    if ( url.empty( ) ) {
        if ( auto ft = FbxCast< FbxFileTexture >( t ) )
            url = ft->GetFileName( );
    }

    if ( !url.empty( ) ) {
        s.embedQueue.insert( FindFile( url.c_str( ) ) );

        s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                 s.PushName( t->GetName( ) ),
                                 s.PushName( GetFileName( url.c_str( ) ) ),
                                 (apemodefb::EBlendMode) t->GetBlendMode( ),
                                 (apemodefb::EWrapMode) t->GetWrapModeU( ),
                                 (apemodefb::EWrapMode) t->GetWrapModeV( ),
                                 (float) t->GetTranslationU( ),
                                 (float) t->GetTranslationV( ),
                                 (float) t->GetScaleU( ),
                                 (float) t->GetScaleV( ) );

        m.props.emplace_back( s.PushName( pp.GetName( ).Buffer( ) ),
                              apemodefb::EMaterialPropTypeFb_Texture,
                              apemodefb::vec3( static_cast< float >( s.textures.back( ).id( ) ), 0, 0 ) );

        s.console->info( "Found texture \"{}\" (\"{}\") (\"{}\")",
                         t->GetName( ),
                         GetFileName( url.c_str( ) ).c_str( ),
                         pp.GetName( ).Buffer( ) );
    }
}

void ExportTextures( std::string const& pn, apemode::Material& m, FbxProperty& pp ) {
    if ( const int ltc = pp.GetSrcObjectCount< FbxLayeredTexture >( ) ) {
        for ( int j = 0; j < ltc; j++ )
            if ( FbxLayeredTexture* lt = pp.GetSrcObject< FbxLayeredTexture >( j ) ) {
                int lc = lt->GetSrcObjectCount< FbxTexture >( );
                for ( int k = 0; k < lc; k++ ) {
                    ExportTexture( pn, m, pp, lt->GetSrcObject< FbxTexture >( k ) );
                }
            }
    }
    if ( const int tc = pp.GetSrcObjectCount< FbxTexture >( ) ) {
        for ( int j = 0; j < tc; j++ ) {
            ExportTexture( pn, m, pp, pp.GetSrcObject< FbxTexture >( j ) );
        }
    }
    if ( const int vc = pp.GetSrcObjectCount< FbxVideo >( ) ) {
        for ( int j = 0; j < vc; j++ ) {
            ExportVideo( pn, m, pp, pp.GetSrcObject< FbxVideo >( j ) );
        }
    }
}

template < typename TPropertyCallbackFn, typename TObjectCallbackFn >
void ScanConnectedSrc( FbxObject*          pPropObj,
                       FbxProperty         srcProperty,
                       TPropertyCallbackFn propertyCallback,
                       TObjectCallbackFn   objectCallback ) {
    while ( srcProperty.IsValid( ) ) {

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

            auto srcProperty = material->GetFirstProperty( );
            ScanConnectedSrc( material,
                              material->GetFirstProperty( ),
                              [&]( FbxProperty& srcProperty, int srcObjCount ) {
                                  auto     srcPropertyName     = srcProperty.GetName( );
                                  auto     srcPropertyTypeName = srcProperty.GetPropertyDataType( ).GetName( );
                                  EFbxType srcPropertyType     = srcProperty.GetPropertyDataType( ).GetType( );

                                  s.console->info( "Prop: {}: {} ({})",
                                                   srcProperty.GetNameAsCStr( ),
                                                   srcPropertyTypeName,
                                                   (int) srcPropertyType );
                              },
                              [&]( FbxProperty& srcProperty, FbxObject* pSrcObj, int srcObjIndex ) {

                                  if ( auto pTexture = FbxCast< FbxTexture >( pSrcObj ) ) {
                                      std::string fileUrl = pTexture->GetUrl( );

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
                                          s.console->info( "Full path (default case): \"{}\"", fullFilePath );
                                          s.embedQueue.insert( fullFilePath );
                                          found = true;
                                      } else {
                                          /* Sketchfab case */
                                          fullFilePath = FindFile( ( fileUrl + ".png" ).c_str( ) );
                                          if ( false == fullFilePath.empty( ) ) {
                                              s.console->info( "Full path (sketchfab, png): \"{}\"", fullFilePath );
                                              s.embedQueue.insert( fullFilePath );
                                              found = true;
                                          }
                                      }

                                      if ( false == found ) {
                                          s.console->error( "Missing: \"{}\"", GetFileName( fileUrl.c_str( ) ) );
                                          s.missingQueue.insert( GetFileName( fileUrl.c_str( ) ) );
                                      }
                                  }

                                  if ( auto pVideo = FbxCast< FbxVideo >( pSrcObj ) ) {
                                      std::string fileUrl = pVideo->GetUrl( );

                                      if ( fileUrl.empty( ) ) {
                                          fileUrl = pVideo->GetFileName( );
                                      }

                                      if ( fileUrl.empty( ) ) {
                                          fileUrl = pVideo->GetName( );
                                      }

                                      s.console->info( "Bound video: \"{}\" <= \"{}\" <= \"{}\"",
                                                       GetFileName( fileUrl.c_str( ) ),
                                                       pSrcObj->GetName( ),
                                                       srcProperty.GetNameAsCStr( ) );

                                      auto fullFilePath = FindFile( fileUrl.c_str( ) );
                                      if ( false == fullFilePath.empty( ) ) {
                                          s.embedQueue.insert( fullFilePath );
                                      } else {
                                          s.console->error( "Missing: \"{}\"", GetFileName( fileUrl.c_str( ) ) );
                                          s.missingQueue.insert( GetFileName( fileUrl.c_str( ) ) );
                                      }
                                  }
                              } );

            const uint32_t id = static_cast< uint32_t >( s.materials.size( ) );

            s.materials.emplace_back( );
            auto& m  = s.materials.back( );
            m.id = id;
            m.nameId = s.PushName( material->GetName( ) );

            s.materialDict[ m.nameId ] = id;
            s.console->info( "Material: \"{}\"", material->GetName( ) );

            if ( material->GetClassId( ).Is( FbxSurfaceLambert::ClassId ) )
                ExportMaterial< FbxSurfaceLambert >( material, m );
            if ( material->GetClassId( ).Is( FbxSurfacePhong::ClassId ) )
                ExportMaterial< FbxSurfacePhong >( material, m );

            using S = FbxSurfaceMaterial;
            for ( auto pn : {S::sAmbient,
                             S::sAmbientFactor,
                             S::sBump,
                             S::sBumpFactor,
                             S::sDiffuse,
                             S::sDiffuseFactor,
                             S::sDisplacementColor,
                             S::sDisplacementFactor,
                             S::sEmissive,
                             S::sEmissiveFactor,
                             S::sNormalMap,
                             S::sReflection,
                             S::sReflectionFactor,
                             S::sShininess,
                             S::sSpecular,
                             S::sSpecularFactor,
                             S::sTransparencyFactor,
                             S::sTransparentColor,
                             S::sVectorDisplacementColor,
                             S::sVectorDisplacementFactor} ) {
                ExportTextures( pn, m, material->FindProperty( pn ) );
            }
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