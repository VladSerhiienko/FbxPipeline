#include <ImageLoaderVk.h>
#include <BufferPools.Vulkan.h>
#include <QueuePools.Vulkan.h>

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

std::unique_ptr< apemodevk::LoadedImage > apemodevk::ImageLoader::LoadImageFromData(
    apemodevk::GraphicsDevice* pNode, const std::vector< uint8_t >& InFileContent, EImageFileFormat eFileFormat ) {
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
                LodePNGStateWrapper( ) {
                    lodepng_state_init( &state );
                }
                ~LodePNGStateWrapper( ) {
                    lodepng_state_cleanup( &state );
                }
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

                apemodevk::InitializeStruct(loadedImage->imageCreateInfo );
                loadedImage->imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
                loadedImage->imageCreateInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
                loadedImage->imageCreateInfo.extent.width  = imageWidth;
                loadedImage->imageCreateInfo.extent.height = imageHeight;
                loadedImage->imageCreateInfo.extent.depth  = 1;
                loadedImage->imageCreateInfo.mipLevels     = 1;
                loadedImage->imageCreateInfo.arrayLayers   = 1;
                loadedImage->imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
                loadedImage->imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
                loadedImage->imageCreateInfo.usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                loadedImage->imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
                loadedImage->imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

                if ( false == loadedImage->hImg.Recreate( *pNode, *pNode, loadedImage->imageCreateInfo ) ) {
                    DebugBreak( );
                    return nullptr;
                }

                auto fontImgAllocInfo = loadedImage->hImg.GetMemoryAllocateInfo( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
                if ( false == loadedImage->hImgMemory.Recreate( *pNode, fontImgAllocInfo ) ) {
                    DebugBreak( );
                    return nullptr;
                }

                if ( false == loadedImage->hImg.BindMemory( loadedImage->hImgMemory, 0 ) ) {
                    DebugBreak( );
                    return nullptr;
                }

                InitializeStruct( loadedImage->imageViewCreateInfo );
                loadedImage->imageViewCreateInfo.image                       = loadedImage->hImg;
                loadedImage->imageViewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
                loadedImage->imageViewCreateInfo.format                      = VK_FORMAT_R8G8B8A8_UNORM;
                loadedImage->imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                loadedImage->imageViewCreateInfo.subresourceRange.levelCount = 1;
                loadedImage->imageViewCreateInfo.subresourceRange.layerCount = 1;

                if ( false == loadedImage->hImgView.Recreate( *pNode, loadedImage->imageViewCreateInfo ) ) {
                    DebugBreak( );
                    return nullptr;
                }

                const uint32_t imageByteSize = imageWidth * imageHeight * 4;

                apemodevk::HostBufferPool hostBufferPool;
                hostBufferPool.Recreate( *pNode, *pNode, &pNode->AdapterProps.limits, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, false );

                auto imageBufferSuballocResult = hostBufferPool.Suballocate( pImageBytes, imageByteSize );
                hostBufferPool.Flush( );     /* Unmap buffers and flush all memory ranges */
                lodepng_free( pImageBytes ); /* Free decoded PNG since it is no longer needed */

                auto acquiredQueue = pNode->GetQueuePool( )->Acquire( false, VK_QUEUE_TRANSFER_BIT, true );
                if ( nullptr == acquiredQueue.pQueue ) {
                    while ( nullptr == acquiredQueue.pQueue ) {
                        acquiredQueue = pNode->GetQueuePool( )->Acquire( false, VK_QUEUE_TRANSFER_BIT, false );
                    }
                }

                auto acquiredCmdBuffer = pNode->GetCommandBufferPool( )->Acquire( false, acquiredQueue.QueueFamilyId );
                assert( acquiredCmdBuffer.pCmdBuffer != nullptr );

                if ( VK_SUCCESS != CheckedCall( vkResetCommandPool( *pNode, acquiredCmdBuffer.pCmdPool, 0 ) ) ) {
                    return nullptr;
                }

                VkCommandBufferBeginInfo cmdBufferBeginInfo;
                InitializeStruct( cmdBufferBeginInfo );
                cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                if ( VK_SUCCESS != CheckedCall( vkBeginCommandBuffer( acquiredCmdBuffer.pCmdBuffer, &cmdBufferBeginInfo ) ) ) {
                    return nullptr;
                }

                VkImageMemoryBarrier copyBarrier;
                InitializeStruct( copyBarrier );
                copyBarrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                copyBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
                copyBarrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                copyBarrier.srcQueueFamilyIndex         = acquiredQueue.QueueFamilyId;
                copyBarrier.dstQueueFamilyIndex         = acquiredQueue.QueueFamilyId;
                copyBarrier.image                       = loadedImage->hImg;
                copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyBarrier.subresourceRange.levelCount = 1;
                copyBarrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                                      VK_PIPELINE_STAGE_HOST_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      0,
                                      0,
                                      NULL,
                                      0,
                                      NULL,
                                      1,
                                      &copyBarrier );

                VkBufferImageCopy region;
                InitializeStruct( region );
                region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.layerCount = 1;
                region.imageExtent.width           = imageWidth;
                region.imageExtent.height          = imageHeight;
                region.imageExtent.depth           = 1;
                region.bufferOffset                = imageBufferSuballocResult.dynamicOffset;
                region.bufferImageHeight           = 0; /* Tightly packed according to the imageExtent */
                region.bufferRowLength             = 0; /* Tightly packed according to the imageExtent */

                vkCmdCopyBufferToImage( acquiredCmdBuffer.pCmdBuffer,
                                        imageBufferSuballocResult.descBufferInfo.buffer,
                                        loadedImage->hImg,
                                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                        1,
                                        &region );

                VkImageMemoryBarrier useBarrier;
                InitializeStruct( useBarrier );
                useBarrier.srcAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
                useBarrier.dstAccessMask               = VK_ACCESS_SHADER_READ_BIT;
                useBarrier.oldLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                useBarrier.newLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                useBarrier.srcQueueFamilyIndex         = acquiredQueue.QueueFamilyId;
                useBarrier.dstQueueFamilyIndex         = acquiredQueue.QueueFamilyId;
                useBarrier.image                       = loadedImage->hImg;
                useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                useBarrier.subresourceRange.levelCount = 1;
                useBarrier.subresourceRange.layerCount = 1;

                vkCmdPipelineBarrier( acquiredCmdBuffer.pCmdBuffer,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                      0,
                                      0,
                                      NULL,
                                      0,
                                      NULL,
                                      1,
                                      &useBarrier );

                vkEndCommandBuffer( acquiredCmdBuffer.pCmdBuffer );

                VkSubmitInfo submitInfo;
                InitializeStruct( submitInfo );
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers    = &acquiredCmdBuffer.pCmdBuffer;

                /* Reset signaled fence */
                if ( VK_SUCCESS == vkGetFenceStatus( *pNode, acquiredQueue.pFence ) )
                    if ( VK_SUCCESS != CheckedCall( vkResetFences( *pNode, 1, &acquiredQueue.pFence ) ) ) {
                        return nullptr;
                    }

                if ( VK_SUCCESS != CheckedCall( vkQueueSubmit( acquiredQueue.pQueue, 1, &submitInfo, acquiredQueue.pFence ) ) ) {
                    return nullptr;
                }

                /* Ensure the image can be used right away */
                if ( VK_SUCCESS != CheckedCall( vkWaitForFences( *pNode, 1, &acquiredQueue.pFence, true, UINT64_MAX ) ) ) {
                    return nullptr;
                }

                acquiredCmdBuffer.pFence = acquiredQueue.pFence;
                pNode->GetCommandBufferPool( )->Release( acquiredCmdBuffer );
                pNode->GetQueuePool( )->Release( acquiredQueue );

                return std::move( loadedImage );
            }
        } break;
    }

    return nullptr;
}
