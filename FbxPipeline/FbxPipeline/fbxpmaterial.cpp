#include <fbxppch.h>
#include <fbxpstate.h>
#include <scene_generated.h>

void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );

template < typename TFbxMaterial >
void ExportMaterial( FbxSurfaceMaterial* material, fbxp::Material& m ) {
    (void) material;
    (void) m;
}

template <>
void ExportMaterial< FbxSurfaceLambert >( FbxSurfaceMaterial* material, fbxp::Material& m ) {
    auto& s  = fbxp::Get( );
    auto  mm = static_cast< FbxSurfaceLambert* >( material );

    m.props.reserve( 12 );

    for ( auto& p : {mm->AmbientFactor, mm->DiffuseFactor, mm->DisplacementFactor, mm->BumpFactor, mm->EmissiveFactor, mm->TransparencyFactor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Scalar,
                              fbxp::fb::vec3( static_cast< float >( p.Get( ) ), 0, 0 ) );
    }

    for ( auto& p : {mm->Ambient, mm->Diffuse, mm->DisplacementColor, mm->Bump, mm->Emissive, mm->TransparentColor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Color,
                              fbxp::fb::vec3( static_cast< float >( p.Get( )[ 0 ] ),
                                              static_cast< float >( p.Get( )[ 1 ] ),
                                              static_cast< float >( p.Get( )[ 2 ] ) ) );
    }
}

template <>
void ExportMaterial< FbxSurfacePhong >( FbxSurfaceMaterial* material, fbxp::Material& m ) {
    m.props.reserve( 16 );

    ExportMaterial< FbxSurfaceLambert >( material, m );

    auto& s  = fbxp::Get( );
    auto  mm = static_cast< FbxSurfacePhong* >( material );

    for ( auto& p : {mm->SpecularFactor, mm->ReflectionFactor} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Scalar,
                              fbxp::fb::vec3( static_cast< float >( p.Get( ) ), 0, 0 ) );
    }

    for ( auto& p : {mm->Specular, mm->Reflection} ) {
        m.props.emplace_back( s.PushName( p.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Color,
                              fbxp::fb::vec3( static_cast< float >( p.Get( )[ 0 ] ),
                                              static_cast< float >( p.Get( )[ 1 ] ),
                                              static_cast< float >( p.Get( )[ 2 ] ) ) );
    }
}

void ExportVideo( std::string const& pn, fbxp::Material& m, FbxProperty& pp, FbxVideo* v ) {
    auto& s = fbxp::Get( );

    std::string url = v->GetFileName( );
    if ( url.empty( ) ) {
        url = v->GetUrl( );
    }
    if (!url.empty()) {
        std::string folder, file;
        SplitFilename(url, folder, file);

        s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                 s.PushName( v->GetName( ) ),
                                 s.PushName( file ),
                                 fbxp::fb::EBlendMode::EBlendMode_Over,
                                 fbxp::fb::EWrapMode::EWrapMode_Clamp,
                                 fbxp::fb::EWrapMode::EWrapMode_Clamp,
                                 (float) 0,
                                 (float) 0,
                                 (float) 1,
                                 (float) 1 );

        m.props.emplace_back( s.PushName( pp.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Video,
                              fbxp::fb::vec3( static_cast< float >( s.textures.back( ).id( ) ), 0, 0 ) );

        s.console->info("Found video \"{}\" (\"{}\") (\"{}\")", v->GetName(), file, pp.GetName().Buffer());

    }
}

void ExportTexture( std::string const& pn, fbxp::Material& m, FbxProperty& pp, FbxTexture* t ) {
    auto& s = fbxp::Get( );
    std::string url = t->GetUrl( );

    if ( url.empty( ) ) {
        if ( auto ft = FbxCast< FbxFileTexture >( t ) )
            url = ft->GetFileName( );
    }

    if ( !url.empty( ) ) {
        std::string folder, file;
        SplitFilename( url, folder, file );

        s.textures.emplace_back( (uint32_t) s.textures.size( ),
                                 s.PushName( t->GetName( ) ),
                                 s.PushName( file ),
                                 (fbxp::fb::EBlendMode) t->GetBlendMode( ),
                                 (fbxp::fb::EWrapMode) t->GetWrapModeU( ),
                                 (fbxp::fb::EWrapMode) t->GetWrapModeV( ),
                                 (float) t->GetTranslationU( ),
                                 (float) t->GetTranslationV( ),
                                 (float) t->GetScaleU( ),
                                 (float) t->GetScaleV( ) );

        m.props.emplace_back( s.PushName( pp.GetName( ).Buffer( ) ),
                              fbxp::fb::EMaterialPropTypeFb_Texture,
                              fbxp::fb::vec3( static_cast< float >( s.textures.back( ).id( ) ), 0, 0 ) );

        s.console->info( "Found texture \"{}\" (\"{}\") (\"{}\")", t->GetName( ), file, pp.GetName( ).Buffer( ) );
    }
}

void ExportTextures( std::string const& pn, fbxp::Material& m, FbxProperty& pp ) {
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

void ExportMaterials( FbxScene* scene ) {
    auto& s = fbxp::Get( );

    if ( auto c = scene->GetMaterialCount( ) ) {
        s.materials.reserve( c );
        s.textures.reserve( c * 3 );

        for ( auto i = 0; i < c; ++i ) {
            auto material = scene->GetMaterial( i );
            const uint32_t id = static_cast< uint32_t >( s.materials.size( ) );

            s.materials.emplace_back( );
            auto& m  = s.materials.back( );
            m.id = id;
            m.nameId = s.PushName( material->GetName( ) );

            s.materialDict[ m.nameId ] = id;
            s.console->info( "Found material \"{}\"", material->GetName( ) );

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

void ExportMaterials( FbxNode* node, fbxp::Node& n ) {
    auto& s = fbxp::Get( );
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