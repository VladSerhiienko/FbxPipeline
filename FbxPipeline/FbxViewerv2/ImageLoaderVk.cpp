#include <ImageLoaderVk.h>

#ifndef LODEPNG_COMPILE_ALLOCATORS
#define LODEPNG_COMPILE_ALLOCATORS
#endif

void* lodepng_malloc( size_t size ) {
    return malloc( size );
}

void* lodepng_realloc( void* ptr, size_t new_size ) {
    return realloc( ptr, new_size );
}

void lodepng_free( void* ptr ) {
    free( ptr );
}

#include <lodepng.h>
#include <lodepng_util.h>
#include <gli/gli.hpp>

std::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromData( apemodevk::GraphicsDevice* pNode, const std::vector< uint8_t >& InFileContent, EImageFileFormat eFileFormat ) {
    switch ( eFileFormat ) {
        case apemodevk::ImageLoader::eImageFileFormat_DDS:
        case apemodevk::ImageLoader::eImageFileFormat_KTX: {
            auto textureGli = gli::load( (const char*) InFileContent.data( ), InFileContent.size( ) );

            if ( false == textureGli.empty( ) ) {
                auto loadedImage = std::make_unique< LoadedImage >( );

                return std::move( loadedImage );
            }
        } break;
        case apemodevk::ImageLoader::eImageFileFormat_PNG: {

            /* Ensure no leaks */
            struct LodePNGStateWrapper {
                LodePNGState state;
                LodePNGStateWrapper( ) { lodepng_state_init( &state ); }
                ~LodePNGStateWrapper( ) { lodepng_state_cleanup( &state ); }
            } stateWrapper;

            uint8_t* pImageBytes = nullptr;
            uint32_t imageHeight = 0;
            uint32_t imageWidth  = 0;

            /* Load png file here from memory buffer */
            if ( 0 == lodepng_decode( &pImageBytes,
                                      &imageWidth,
                                      &imageHeight,
                                      &stateWrapper.state,
                                      InFileContent.data( ),
                                      InFileContent.size( ) ) ) {
                auto loadedImage = std::make_unique< LoadedImage >( );

                apemodevk::TDispatchableHandle<VkImage> hImg;
                VkImageCreateInfo imageCreateInfo;
                apemodevk::InitializeStruct(imageCreateInfo);

                /* Clean-up allocated memory, lodepng wont handle it */
                lodepng_free( pImageBytes );
                return std::move( loadedImage );
            }
        } break;
    }

    return nullptr;
}
