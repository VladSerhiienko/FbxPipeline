#include <gli/gli.hpp>
#include <ImageLoaderVk.h>

std::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromData(
    apemodevk::GraphicsDevice* pNode, const std::vector< uint8_t >& InFileContent ) {
    auto textureGli = gli::load( (const char*) InFileContent.data( ), InFileContent.size( ) );

    if ( false == textureGli.empty( ) ) {
        auto loadedImage = std::make_unique< LoadedImage >( );

        return std::move( loadedImage );
    }

    return nullptr;
}
