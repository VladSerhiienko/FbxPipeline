#include <fbxppch.h>
#include <fbxpstate.h>

inline fbxp::fb::vec3 cast( FbxDouble3 const& d ) {
    return fbxp::fb::vec3{ static_cast< float >( d.mData[ 0 ] ), static_cast< float >( d.mData[ 1 ] ), static_cast< float >( d.mData[ 2 ] )};
}

void SplitFilename( const std::string& filePath, std::string& parentFolderName, std::string& fileName );

template < typename TFbxMaterial >
void ExportMaterial( FbxSurfaceMaterial* material, fbxp::Material& m ) {
}

template <>
void ExportMaterial< FbxSurfaceLambert >( FbxSurfaceMaterial* material, fbxp::Material& m ) {
    auto& s  = fbxp::GetState( );
    auto  mm = static_cast< FbxSurfaceLambert* >( material );

    m.props.reserve( 12 );

    for ( auto& p : {mm->AmbientFactor,
                     mm->DiffuseFactor,
                     mm->DisplacementFactor,
                     mm->BumpFactor,
                     mm->EmissiveFactor,
                     mm->TransparencyFactor} ) {
        m.props.emplace_back( );
        m.props.back( ).type   = fbxp::Material::Prop::eScalar;
        m.props.back( ).nameId = s.PushName( p.GetName( ).Buffer( ) );
        m.props.back( ).scalar = static_cast< float >( p.Get( ) );
    }

    for ( auto& p : {mm->Ambient, mm->Diffuse, mm->DisplacementColor, mm->Bump, mm->Emissive, mm->TransparentColor} ) {
        m.props.emplace_back( );
        m.props.back( ).type   = fbxp::Material::Prop::eColor;
        m.props.back( ).nameId = s.PushName( p.GetName( ).Buffer( ) );
        m.props.back( ).color[ 0 ] = static_cast< float >( p.Get( )[ 0 ] );
        m.props.back( ).color[ 1 ] = static_cast< float >( p.Get( )[ 1 ] );
        m.props.back( ).color[ 2 ] = static_cast< float >( p.Get( )[ 2 ] );
    }
}

template <>
void ExportMaterial< FbxSurfacePhong >( FbxSurfaceMaterial* material, fbxp::Material& m ) {
    m.props.reserve( 16 );

    ExportMaterial< FbxSurfaceLambert >( material, m );

    auto& s  = fbxp::GetState( );
    auto  mm = static_cast< FbxSurfacePhong* >( material );

    for ( auto& p : {mm->SpecularFactor, mm->ReflectionFactor} ) {
        m.props.emplace_back( );
        m.props.back( ).type   = fbxp::Material::Prop::eScalar;
        m.props.back( ).nameId = s.PushName( p.GetName( ).Buffer( ) );
        m.props.back( ).scalar = static_cast< float >( p.Get( ) );
    }

    for ( auto& p : {mm->Specular, mm->Reflection} ) {
        m.props.emplace_back( );
        m.props.back( ).type   = fbxp::Material::Prop::eColor;
        m.props.back( ).nameId = s.PushName( p.GetName( ).Buffer( ) );
        m.props.back( ).color[ 0 ] = static_cast< float >( p.Get( )[ 0 ] );
        m.props.back( ).color[ 1 ] = static_cast< float >( p.Get( )[ 1 ] );
        m.props.back( ).color[ 2 ] = static_cast< float >( p.Get( )[ 2 ] );
    }
}

void ExportTexture( std::string const& PropName, fbxp::Material& m, FbxProperty& Prop, FbxTexture* pTexture ) {
    auto& s  = fbxp::GetState( );
    std::string url = pTexture->GetUrl( );

    if ( url.empty( ) ) {
        if ( auto f = FbxCast< FbxFileTexture >( pTexture ) )
            url = f->GetFileName( );
    }

    if ( !url.empty( ) ) {
        std::string folder, file;
        SplitFilename( url, folder, file );

        s.textures.push_back( fbxp::fb::TextureFb( (uint32_t) s.textures.size( ),
                                                   s.PushName( pTexture->GetName( ) ),
                                                   s.PushName( file ),
                                                   (fbxp::fb::EBlendMode) pTexture->GetBlendMode( ),
                                                   (fbxp::fb::EWrapMode) pTexture->GetWrapModeU( ),
                                                   (fbxp::fb::EWrapMode) pTexture->GetWrapModeV( ),
                                                   (float) pTexture->GetTranslationU( ),
                                                   (float) pTexture->GetTranslationV( ),
                                                   (float) pTexture->GetScaleU( ),
                                                   (float) pTexture->GetScaleV( ) ) );
        m.props.emplace_back( );

        auto& p     = m.props.back( );
        p.type      = fbxp::Material::Prop::eTexture;
        p.textureId = s.textures.back( ).id( );
        p.nameId    = s.PushName( Prop.GetName( ).Buffer( ) );

        s.console->info( "Found texture \"{}\" (\"{}\") ({})", pTexture->GetName( ), file, Prop.GetName( ).Buffer( ) );
    }
}

void ExportTextures( std::string const& PropName, fbxp::Material & m, FbxProperty& Prop ) {
    //FbxEmb
    int LayeredTextureCount = Prop.GetSrcObjectCount<FbxLayeredTexture>();
    if (LayeredTextureCount > 0)
    {
        for (int j = 0; j < LayeredTextureCount; j++)
            if (FbxLayeredTexture * pLayeredTexture = Prop.GetSrcObject<FbxLayeredTexture>(j))
            {
                int LayerCount = pLayeredTexture->GetSrcObjectCount<FbxTexture>();
                for (int k = 0; k < LayerCount; k++)
                {
                    ExportTexture(
                        PropName,
                        m,
                        Prop,
                        pLayeredTexture->GetSrcObject<FbxTexture>(k));
                }
            }
    }
    else
    {
        int TextureCount = Prop.GetSrcObjectCount<FbxTexture>();
        for (int j = 0; j < TextureCount; j++)
        {
            ExportTexture( PropName, m, Prop, Prop.GetSrcObject<FbxTexture>(j));
        }
    }
}

void ExportMaterials( FbxScene* scene ) {

    auto& s = fbxp::GetState( );
    if ( auto c = scene->GetMaterialCount( ) ) {
        for ( auto i = 0; i < c; ++i ) {
            auto material = scene->GetMaterial( i );
            auto id       = s.materials.size( );

            s.materials.emplace_back( );
            auto& m  = s.materials.back( );
            m.id     = id;
            m.nameId = s.PushName( material->GetName( ) );

            s.console->info( "Found material \"{}\"", material->GetName( ) );

            if ( material->GetClassId( ).Is( FbxSurfaceLambert::ClassId ) )
                ExportMaterial< FbxSurfaceLambert >( material, m );
            if ( material->GetClassId( ).Is( FbxSurfacePhong::ClassId ) )
                ExportMaterial< FbxSurfacePhong >( material, m );

            for ( auto propertyName : {FbxSurfaceMaterial::sAmbient,
                                       FbxSurfaceMaterial::sAmbientFactor,
                                       FbxSurfaceMaterial::sBump,
                                       FbxSurfaceMaterial::sBumpFactor,
                                       FbxSurfaceMaterial::sMultiLayer,
                                       FbxSurfaceMaterial::sDiffuse,
                                       FbxSurfaceMaterial::sDiffuseFactor,
                                       FbxSurfaceMaterial::sDisplacementColor,
                                       FbxSurfaceMaterial::sDisplacementFactor,
                                       FbxSurfaceMaterial::sEmissive,
                                       FbxSurfaceMaterial::sEmissiveFactor,
                                       FbxSurfaceMaterial::sNormalMap,
                                       FbxSurfaceMaterial::sReflection,
                                       FbxSurfaceMaterial::sReflectionFactor,
                                       FbxSurfaceMaterial::sShininess,
                                       FbxSurfaceMaterial::sSpecular,
                                       FbxSurfaceMaterial::sSpecularFactor,
                                       FbxSurfaceMaterial::sTransparencyFactor,
                                       FbxSurfaceMaterial::sTransparentColor,
                                       FbxSurfaceMaterial::sVectorDisplacementColor,
                                       FbxSurfaceMaterial::sVectorDisplacementFactor} ) {
                ExportTextures( propertyName, m, material->FindProperty( propertyName ) );
            }
        }
    }
}

void ExportMaterials( FbxNode* node, fbxp::Node& n ) {
}