//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <CommandQueue.Vulkan.h>
#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <Swapchain.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------

uint64_t const Uint64Max = std::numeric_limits< uint64_t >::max( );

/// -------------------------------------------------------------------------------------------------------------------

bool apemodevk::Swapchain::ExtractSwapchainBuffers( VkImage * OutBufferImgs) {
    _Game_engine_Assert(hSwapchain.IsNotNull(), "Not initialized.");

    uint32_t OutSwapchainBufferCount = 0;
    if (apemode_likely(apemodevk::ResultHandle::Succeeded( vkGetSwapchainImagesKHR(*pNode, hSwapchain, &OutSwapchainBufferCount, nullptr)))) {

        if (OutSwapchainBufferCount > kMaxImgs) {
            DebugBreak();
            return false;
        }

        if (apemode_likely( apemodevk::ResultHandle::Succeeded(vkGetSwapchainImagesKHR(*pNode, hSwapchain, &OutSwapchainBufferCount, OutBufferImgs))))
            return true;
    }

    _Game_engine_Halt("vkGetSwapchainImagesKHR failed.");
    return false;

}

bool apemodevk::Swapchain::ExtractSwapchainBuffers( std::vector< VkImage >& OutSwapchainBufferImgs ) {
    _Game_engine_Assert( hSwapchain.IsNotNull( ), "Not initialized." );

    uint32_t OutSwapchainBufferCount = 0;
    if ( apemode_likely( apemodevk::ResultHandle::Succeeded(
             vkGetSwapchainImagesKHR( *pNode, hSwapchain, &OutSwapchainBufferCount, nullptr ) ) ) ) {
        OutSwapchainBufferImgs.resize( OutSwapchainBufferCount, VkImage( nullptr ) );
        if ( apemode_likely( apemodevk::ResultHandle::Succeeded(
                 vkGetSwapchainImagesKHR( *pNode, hSwapchain, &OutSwapchainBufferCount, OutSwapchainBufferImgs.data( ) ) ) ) )
            return true;
    }

    _Game_engine_Halt( "vkGetSwapchainImagesKHR failed." );
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// Swapchain
/// -------------------------------------------------------------------------------------------------------------------

uint64_t const                           apemodevk::Swapchain::kMaxTimeout            = Uint64Max;
uint32_t const                           apemodevk::Swapchain::kExtentMatchFullscreen = -1;
uint32_t const                           apemodevk::Swapchain::kExtentMatchWindow     = 0;
apemodevk::Swapchain::ModuleHandle const apemodevk::Swapchain::kCurrentExecutable     = nullptr;

apemodevk::Swapchain::Swapchain( ) : pNode( nullptr ), pCmdQueue( nullptr ) {
    static uint16_t sSwapchainNextId = 0;
    Id                               = sSwapchainNextId++;
}

apemodevk::Swapchain::~Swapchain( ) {
    _Aux_DebugTraceFunc;

    if ( pNode ) {
        const bool bOk = pNode->Await( );
        _Game_engine_Assert( bOk, "Failed to wait for device prior work done." );

        for ( auto& hBuffer : hImgs )
            if ( hBuffer ) {
                apemodevk::TDispatchableHandle< VkImage > hImg;
                hImg.Deleter.LogicalDeviceHandle = *pNode;
                hImg.Handle                      = hBuffer;
                hImg.Destroy( );
            }

        /*for (auto & hPresentSemaphore : hPresentSemaphores)
            if (hPresentSemaphore)
            {
                apemodevk::TDispatchableHandle<VkSemaphore> hSemaphore;
                hSemaphore.Deleter.LogicalDeviceHandle = *pNode;
                hSemaphore.Handle = hPresentSemaphore;
                hSemaphore.Destroy ();
            }*/
    }
}

bool apemodevk::Swapchain::RecreateResourceFor( GraphicsDevice& InGraphicsNode,
    uint32_t CmdQueueFamilyId,
#ifdef _WIN32
                                                ModuleHandle InInst,
                                                WindowHandle InWnd,
#endif
                                                uint32_t DesiredColorWidth,
                                                uint32_t DesiredColorHeight ) {
    if ( !InGraphicsNode.IsValid( ) ) {
        _Game_engine_Halt( "Provided logical graphics device is invalid." );
        return false;
    }

    if ( !InWnd ) {
        _Game_engine_Halt( "Provided window handle is null." );
        return false;
    }

    if ( InInst == kCurrentExecutable )
        InInst = GetModuleHandle( NULL );

    pNode = &InGraphicsNode;

    TInfoStruct< VkWin32SurfaceCreateInfoKHR > SurfaceDesc;
    SurfaceDesc->hwnd      = InWnd;
    SurfaceDesc->hinstance = InInst;

    if ( !hSurface.Recreate( InGraphicsNode, SurfaceDesc ) ) {
        _Game_engine_Halt( "Failed to create surface." );
        return false;
    }

    VkBool32 bSupported = false;
    if (ResultHandle::Failed(vkGetPhysicalDeviceSurfaceSupportKHR(InGraphicsNode, CmdQueueFamilyId, hSurface, &bSupported))) {
        _Game_engine_Halt("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed.");
        return false;
    }

    if (false == bSupported) {
        _Game_engine_Halt("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed.");
        return false;
    }

    if ( ResultHandle::Failed( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( InGraphicsNode, hSurface, &SurfaceCaps ) ) ) {
        _Game_engine_Halt( "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed." );
        return false;
    }

    uint32_t SurfaceFormatCount = 0;
    if ( ResultHandle::Succeeded( vkGetPhysicalDeviceSurfaceFormatsKHR( InGraphicsNode, hSurface, &SurfaceFormatCount, nullptr ) ) ) {
        std::vector< VkSurfaceFormatKHR > SurfaceFormats( SurfaceFormatCount );
        if ( ResultHandle::Succeeded( vkGetPhysicalDeviceSurfaceFormatsKHR( InGraphicsNode, hSurface, &SurfaceFormatCount, SurfaceFormats.data( ) ) ) ) {
            const bool bCanChooseAny = SurfaceFormatCount == 1 && SurfaceFormats[ 0 ].format == VK_FORMAT_UNDEFINED;
            eColorFormat = bCanChooseAny ? VK_FORMAT_B8G8R8A8_UNORM : SurfaceFormats[ 0 ].format;
            eColorSpace = SurfaceFormats[ 0 ].colorSpace;
        }
    }

    const bool bMatchesWindow     = DesiredColorWidth == kExtentMatchWindow && DesiredColorHeight == kExtentMatchWindow;
    const bool bMatchesFullscreen = DesiredColorWidth == kExtentMatchFullscreen && DesiredColorHeight == kExtentMatchFullscreen;
    const bool bIsDefined         = !bMatchesWindow && !bMatchesFullscreen;
    _Game_engine_Assert( bIsDefined || bMatchesFullscreen || bMatchesWindow, "Unexpected." );

    ColorExtent.width  = 0;
    ColorExtent.height = 0;

    if ( SurfaceCaps.currentExtent.width == kExtentMatchFullscreen &&
         SurfaceCaps.currentExtent.height == kExtentMatchFullscreen ) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        _Game_engine_Assert( bIsDefined, "Unexpected." );

        if ( bIsDefined ) {
            ColorExtent.width  = DesiredColorWidth;
            ColorExtent.height = DesiredColorHeight;
        }
    } else {
        // If the surface size is defined, the swap chain size must match
        ColorExtent = SurfaceCaps.currentExtent;
    }

    // We fall back to FIFO which is always available.
    ePresentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t PresentModeCount = 0;
    if ( ResultHandle::Succeeded( vkGetPhysicalDeviceSurfacePresentModesKHR( InGraphicsNode, hSurface, &PresentModeCount, nullptr ) ) ) {
        std::vector< VkPresentModeKHR > PresentModes;
        PresentModes.resize( PresentModeCount );

        if ( ResultHandle::Succeeded( vkGetPhysicalDeviceSurfacePresentModesKHR(
                 InGraphicsNode, hSurface, &PresentModeCount, PresentModes.data( ) ) ) ) {
            for ( auto i = 0u; i < PresentModeCount; i++ ) {
                auto& CurrentPresentMode = PresentModes[ i ];
                if ( CurrentPresentMode == VK_PRESENT_MODE_MAILBOX_KHR ) {
                    // If mailbox mode is available, use it, as is the lowest-latency non- tearing mode.
                    ePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }

                // If not, try IMMEDIATE which will usually be available, and is fastest (though it tears).
                if ( ( ePresentMode != VK_PRESENT_MODE_MAILBOX_KHR ) &&
                     ( CurrentPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR ) )
                    ePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We desire to own only 1 image at a time, besides the
    // images being displayed and queued for display.

    ImgCount = std::min<uint32_t>( kMaxImgs, SurfaceCaps.minImageCount + 1 );
    if ( ( SurfaceCaps.maxImageCount > 0 ) && ( SurfaceCaps.maxImageCount < ImgCount) ) {
        // Application must settle for fewer images than desired.
        ImgCount = SurfaceCaps.maxImageCount;
    }

    const bool bSurfaceSupportsIdentity = apemodevk::HasFlagEql( SurfaceCaps.supportedTransforms, VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR );
    eSurfaceTransform = bSurfaceSupportsIdentity ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : SurfaceCaps.currentTransform;

    TInfoStruct< VkSwapchainCreateInfoKHR > SwapchainDesc;
    SwapchainDesc->surface          = hSurface;
    SwapchainDesc->minImageCount    = ImgCount;
    SwapchainDesc->imageFormat      = eColorFormat;
    SwapchainDesc->imageColorSpace  = eColorSpace;
    SwapchainDesc->imageExtent      = ColorExtent;
    SwapchainDesc->imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainDesc->preTransform     = static_cast< VkSurfaceTransformFlagBitsKHR >( eSurfaceTransform );
    SwapchainDesc->compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainDesc->imageArrayLayers = 1;
    SwapchainDesc->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainDesc->presentMode      = ePresentMode;
    SwapchainDesc->oldSwapchain     = hSwapchain;
    SwapchainDesc->clipped          = true;

    // TODO: Save previous here.

    if ( !hSwapchain.Recreate( InGraphicsNode, SwapchainDesc ) ) {
        _Game_engine_Halt( "Failed to create swapchain." );
        return false;
    }

    if ( !ExtractSwapchainBuffers( hImgs ) ) {
        _Game_engine_Halt( "Failed to extract swapchain buffers." );
        return false;
    }

    TInfoStruct< VkImageViewCreateInfo > imgViewCreateInfo;
    imgViewCreateInfo->viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo->viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imgViewCreateInfo->format                          = eColorFormat;
    imgViewCreateInfo->components.r                    = VK_COMPONENT_SWIZZLE_R;
    imgViewCreateInfo->components.g                    = VK_COMPONENT_SWIZZLE_G;
    imgViewCreateInfo->components.b                    = VK_COMPONENT_SWIZZLE_B;
    imgViewCreateInfo->components.a                    = VK_COMPONENT_SWIZZLE_A;
    imgViewCreateInfo->subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imgViewCreateInfo->subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo->subresourceRange.baseMipLevel   = 0;
    imgViewCreateInfo->subresourceRange.layerCount     = 1;
    imgViewCreateInfo->subresourceRange.levelCount     = 1;

    for (uint32_t i = 0;i < ImgCount; ++i) {
        imgViewCreateInfo->image = hImgs[i];
        hImgViews[i].Recreate(*pNode, imgViewCreateInfo);
    }

    return true;
}

uint32_t apemodevk::Swapchain::GetBufferCount( ) const {
    return ImgCount;
}

bool apemodevk::Swapchain::OnFrameMove( apemodevk::RenderPassResources& Resources,
                                        VkSemaphore                     hSemaphore,
                                        VkFence                         hFence,
                                        uint64_t                        Timeout ) {
    _Game_engine_Assert( pNode != nullptr, "Not initialized." );

    uint32_t OutSwapchainBufferIdx = 0xffffffff;

    // AdvancePresentSemaphoreIdx ();
    const auto eImgAcquiredError =
        vkAcquireNextImageKHR( *pNode, hSwapchain, Timeout, hSemaphore, hFence, &OutSwapchainBufferIdx );

    if ( apemode_likely( eImgAcquiredError == ResultHandle::Success || eImgAcquiredError == ResultHandle::Suboptimal ) ) {
        _Game_engine_Assert( eImgAcquiredError == ResultHandle::Success, "Reconfigure." );
        _Game_engine_Assert( Resources.GetFrameCount( ) == GetBufferCount( ), "Should match." );

        Resources.SetWriteFrame( OutSwapchainBufferIdx );
        return true;
    }

    _Game_engine_Assert( eImgAcquiredError == ResultHandle::Timeout,
                         "Failed to acquire swapchain buffer "
                         "(and it`s not because of timeout)." );
    return false;
}

bool apemodevk::Swapchain::OnFramePresent( apemodevk::CommandQueue&        CmdQueue,
                                           apemodevk::RenderPassResources& Resources,
                                           VkSemaphore*                    pWaitSemaphores,
                                           uint32_t                        WaitSemaphoreCount ) {
    _Game_engine_Assert( ( pWaitSemaphores && WaitSemaphoreCount ) || ( !pWaitSemaphores && !WaitSemaphoreCount ),
                         "Missing info." );

    const uint32_t pImgIndices[] = {Resources.GetWriteFrame( )};

    VkResult eSwapchainResult = VK_SUCCESS;

    TInfoStruct< VkPresentInfoKHR > PresentDesc;
    PresentDesc->swapchainCount     = 1;
    PresentDesc->pSwapchains        = hSwapchain;
    PresentDesc->pImageIndices      = pImgIndices;
    PresentDesc->pWaitSemaphores    = pWaitSemaphores;
    PresentDesc->waitSemaphoreCount = WaitSemaphoreCount;
    PresentDesc->pResults           = &eSwapchainResult;

    VkResult ePresentResult = vkQueuePresentKHR( CmdQueue, PresentDesc );
    if ( apemode_likely( ResultHandle::Succeeded( ePresentResult ) ) ) {
        const bool bIsOk = ResultHandle::Succeeded( eSwapchainResult );
        _Game_engine_Assert( bIsOk, "Failed to present to swapchain." );

        return apemode_likely( bIsOk );
    }

    _Game_engine_Halt( "Failed to present." );
    return false;
}