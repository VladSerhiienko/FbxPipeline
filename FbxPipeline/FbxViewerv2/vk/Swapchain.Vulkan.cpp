//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <Swapchain.Vulkan.h>

#include <CommandQueue.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <RenderPassResources.Vulkan.h>
#include <TInfoStruct.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------

uint64_t const Uint64Max = std::numeric_limits<uint64_t>::max();

/// -------------------------------------------------------------------------------------------------------------------

static bool ExtractSwapchainBuffers (apemode::GraphicsDevice &          InGraphicsNode,
                                     VkSwapchainKHR                  hSwapchain,
                                     std::vector<VkImage> & OutSwapchainBufferImgs)
{
    _Game_engine_Assert (InGraphicsNode.IsValid () && hSwapchain, "Not initialized.");

    uint32_t OutSwapchainBufferCount = 0;
    if (apemode_likely (apemode::ResultHandle::Succeeded (vkGetSwapchainImagesKHR (
            InGraphicsNode, hSwapchain, &OutSwapchainBufferCount, nullptr))))
    {
        OutSwapchainBufferImgs.resize (OutSwapchainBufferCount, VkImage (nullptr));
        if (apemode_likely (apemode::ResultHandle::Succeeded (
                vkGetSwapchainImagesKHR (InGraphicsNode,
                                         hSwapchain,
                                         &OutSwapchainBufferCount,
                                         OutSwapchainBufferImgs.data ()))))
        {
            return true;
        }
    }

    _Game_engine_Halt ("vkGetSwapchainImagesKHR failed.");
    OutSwapchainBufferImgs.resize (0);
    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// Swapchain
/// -------------------------------------------------------------------------------------------------------------------

uint64_t const                      apemode::Swapchain::kMaxTimeout            = Uint64Max;
uint32_t const                      apemode::Swapchain::kExtentMatchFullscreen = -1;
uint32_t const                      apemode::Swapchain::kExtentMatchWindow     = 0;
apemode::Swapchain::ModuleHandle const apemode::Swapchain::kCurrentExecutable     = nullptr;

apemode::Swapchain::Swapchain () : pNode (nullptr), pCmdQueue (nullptr)
{
    static uint16_t sSwapchainNextId = 0;
    Id = sSwapchainNextId++;
}

apemode::Swapchain::~Swapchain()
{
    _Aux_DebugTraceFunc;

    if (pNode)
    {
        const bool bOk = pNode->Await ();
        _Game_engine_Assert (bOk, "Failed to wait for device prior work done.");

        for (auto & hBuffer : hBuffers)
            if (hBuffer)
            {
                apemode::TDispatchableHandle<VkImage> hImg;
                hImg.Deleter.LogicalDeviceHandle = *pNode;
                hImg.Handle = hBuffer;
                hImg.Destroy ();
            }

        /*for (auto & hPresentSemaphore : hPresentSemaphores)
            if (hPresentSemaphore)
            {
                apemode::TDispatchableHandle<VkSemaphore> hSemaphore;
                hSemaphore.Deleter.LogicalDeviceHandle = *pNode;
                hSemaphore.Handle = hPresentSemaphore;
                hSemaphore.Destroy ();
            }*/
    }
}

bool apemode::Swapchain::RecreateResourceFor (GraphicsDevice & InGraphicsNode,
                                           CommandQueue &   InCmdQueue,
#ifdef _WIN32
                                           ModuleHandle InInst,
                                           WindowHandle InWnd,
#endif
                                           uint32_t DesiredColorWidth,
                                           uint32_t DesiredColorHeight)
{
    if (!InGraphicsNode.IsValid ())
    {
        _Game_engine_Halt ("Provided logical graphics device is invalid.");
        return false;
    }

    if (!InCmdQueue.hCmdQueue.IsNotNull () || InCmdQueue.pNode != &InGraphicsNode)
    {
        _Game_engine_Halt ("Provided command queue is incompatible.");
        return false;
    }

    if (!InWnd)
    {
        _Game_engine_Halt ("Provided window handle is null.");
        return false;
    }

    if (InInst == kCurrentExecutable)
        InInst = GetModuleHandle(NULL);

    pNode = &InGraphicsNode;
    pCmdQueue     = &InCmdQueue;

    TInfoStruct<VkWin32SurfaceCreateInfoKHR> SurfaceDesc;
    SurfaceDesc->hwnd      = InWnd;
    SurfaceDesc->hinstance = InInst;

    if (!hSurface.Recreate (InGraphicsNode, SurfaceDesc))
    {
        _Game_engine_Halt ("Failed to create surface.");
        return false;
    }

    if (!SupportsPresenting(InGraphicsNode, InCmdQueue))
    {
        _Game_engine_Error ("Provided command queue does not support presenting.");
        return false;
    }

    if (ResultHandle::Failed(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        InGraphicsNode, hSurface, &SurfaceCaps
        )))
    {
        _Game_engine_Halt("vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed.");
        return false;
    }

    uint32_t SurfaceFormatCount = 0;
    if (ResultHandle::Succeeded (vkGetPhysicalDeviceSurfaceFormatsKHR (
            InGraphicsNode, hSurface, &SurfaceFormatCount, nullptr)))
    {
        std::vector<VkSurfaceFormatKHR> SurfaceFormats (SurfaceFormatCount);
        if (ResultHandle::Succeeded (vkGetPhysicalDeviceSurfaceFormatsKHR (
                InGraphicsNode, hSurface, &SurfaceFormatCount, SurfaceFormats.data ())))
        {
            const bool bCanChooseAny = SurfaceFormatCount == 1 && SurfaceFormats[0].format == VK_FORMAT_UNDEFINED;
            eColorFormat = bCanChooseAny ? VK_FORMAT_B8G8R8A8_UNORM : SurfaceFormats[0].format;
            eColorSpace  = SurfaceFormats[ 0 ].colorSpace;
        }
    }

    const bool bMatchesWindow     = DesiredColorWidth == kExtentMatchWindow && DesiredColorHeight == kExtentMatchWindow;
    const bool bMatchesFullscreen = DesiredColorWidth == kExtentMatchFullscreen && DesiredColorHeight == kExtentMatchFullscreen;
    const bool bIsDefined         = !bMatchesWindow && !bMatchesFullscreen;
    _Game_engine_Assert(bIsDefined || bMatchesFullscreen || bMatchesWindow, "Unexpected.");

    ColorExtent.width = 0;
    ColorExtent.height = 0;

    if (SurfaceCaps.currentExtent.width == kExtentMatchFullscreen &&
        SurfaceCaps.currentExtent.height == kExtentMatchFullscreen)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        _Game_engine_Assert(bIsDefined, "Unexpected.");

        if (bIsDefined)
        {
            ColorExtent.width = DesiredColorWidth;
            ColorExtent.height = DesiredColorHeight;
        }
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        ColorExtent = SurfaceCaps.currentExtent;
    }

    // We fall back to FIFO which is always available.
    ePresentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t PresentModeCount = 0;
    if (ResultHandle::Succeeded(vkGetPhysicalDeviceSurfacePresentModesKHR(
        InGraphicsNode, hSurface, &PresentModeCount, nullptr
        )))
    {
        std::vector<VkPresentModeKHR> PresentModes;
        PresentModes.resize(PresentModeCount);

        if (ResultHandle::Succeeded(vkGetPhysicalDeviceSurfacePresentModesKHR(
            InGraphicsNode, hSurface, &PresentModeCount, PresentModes.data()
            )))
        {
            for (auto i = 0u; i < PresentModeCount; i++)
            {
                auto & CurrentPresentMode = PresentModes[i];
                if (CurrentPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    // If mailbox mode is available, use it, as is the lowest-latency non- tearing mode.
                    ePresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                    break;
                }

                // If not, try IMMEDIATE which will usually be available, and is fastest (though it tears).
                if ((ePresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && 
                    (CurrentPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR))
                    ePresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of VkImage's to use in the swap chain.
    // We desire to own only 1 image at a time, besides the
    // images being displayed and queued for display.

    uint32_t BufferCount = SurfaceCaps.minImageCount + 1;
    if ((SurfaceCaps.maxImageCount > 0) && (SurfaceCaps.maxImageCount < BufferCount))
    {
        // Application must settle for fewer images than desired.
        BufferCount = SurfaceCaps.maxImageCount;
    }

    const bool bSurfaceSupportsIdentity
        = apemode::HasFlagEql (SurfaceCaps.supportedTransforms,
                           VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);

    eSurfaceTransform = bSurfaceSupportsIdentity ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                                 : SurfaceCaps.currentTransform;

    TInfoStruct<VkSwapchainCreateInfoKHR> SwapchainDesc;
    SwapchainDesc->surface         = hSurface;
    SwapchainDesc->minImageCount   = BufferCount;
    SwapchainDesc->imageFormat     = eColorFormat;
    SwapchainDesc->imageColorSpace = eColorSpace;
    SwapchainDesc->imageExtent     = ColorExtent;
    SwapchainDesc->imageUsage      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainDesc->preTransform    = static_cast<VkSurfaceTransformFlagBitsKHR> (eSurfaceTransform);
    SwapchainDesc->compositeAlpha  = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainDesc->imageArrayLayers = 1;
    SwapchainDesc->imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainDesc->presentMode      = ePresentMode;
    SwapchainDesc->oldSwapchain     = hSwapchain;
    SwapchainDesc->clipped          = true;

    if (!hSwapchain.Recreate (InGraphicsNode, SwapchainDesc))
    {
        _Game_engine_Halt ("Failed to create swapchain.");
        return false;
    }

    if (!ExtractSwapchainBuffers (InGraphicsNode, hSwapchain, hBuffers))
    {
        _Game_engine_Halt ("Failed to extract swapchain buffers.");
        return false;
    }

    /*hPresentSemaphores.resize (hBuffers.size (), nullptr);
    for (auto & hPresentSemaphore : hPresentSemaphores)
    {
        apemode::TDispatchableHandle<VkSemaphore> hSemaphore;
        if (!hSemaphore.Recreate (InGraphicsNode, apemode::TInfoStruct<VkSemaphoreCreateInfo> ()))
        {
            _Game_engine_Halt ("Failed to create preseting semaphore.");
            return false;
        }

        hPresentSemaphore = hSemaphore.Release ();
    }*/

    return true;
}

bool apemode::Swapchain::SupportsPresenting (apemode::GraphicsDevice const & InGraphicsNode,
                                          CommandQueue const &         InCmdQueue) const
{
    _Game_engine_Assert (hSurface.IsNotNull (), "Not initialized.");
    _Game_engine_Assert (InGraphicsNode.IsValid (), "Not initialized.");
    _Game_engine_Assert (InCmdQueue.hCmdQueue.IsNotNull (), "Not initialized.");

    if (hSurface.IsNotNull () && InCmdQueue.hCmdQueue.IsNotNull ())
    {
        VkBool32 bIsSupported = false;
        const auto eSupport = vkGetPhysicalDeviceSurfaceSupportKHR (InGraphicsNode,
                                                                    InCmdQueue.QueueId,
                                                                    hSurface,
                                                                    &bIsSupported);

        _Game_engine_Assert (ResultHandle::Succeeded (eSupport),
                             "Failed to get surface support.");
        return
            ResultHandle::Succeeded (eSupport) &&
            ResultHandle::Succeeded (bIsSupported);
    }

    return false;
}

uint32_t apemode::Swapchain::GetBufferCount () const
{
    return _Get_collection_length_u (hBuffers);
}

//VkSemaphore apemode::Swapchain::GetPresentSemaphore ()
//{
//    return hPresentSemaphores[ PresentSemapforeIdx ];
//}
//
//VkSemaphore apemode::Swapchain::GetNextPresentSemaphore ()
//{
//    const uint32_t Count = _Get_collection_length_u (hPresentSemaphores);
//    return hPresentSemaphores[ (PresentSemapforeIdx + 1) % Count ];
//}
//
//void apemode::Swapchain::AdvancePresentSemaphoreIdx ()
//{
//    const uint32_t Count = _Get_collection_length_u (hPresentSemaphores);
//    PresentSemapforeIdx  = (PresentSemapforeIdx + 1) % Count;
//}

bool apemode::Swapchain::OnFrameMove (apemode::RenderPassResources & Resources,
                                   VkSemaphore                 hSemaphore,
                                   VkFence                     hFence,
                                   uint64_t                    Timeout)
{
    _Game_engine_Assert(pNode != nullptr, "Not initialized.");

    uint32_t OutSwapchainBufferIdx = 0xffffffff;

    //AdvancePresentSemaphoreIdx ();
    const auto eImgAcquiredError = vkAcquireNextImageKHR (
        *pNode, hSwapchain, Timeout, hSemaphore, hFence, &OutSwapchainBufferIdx);

    if (apemode_likely (eImgAcquiredError == ResultHandle::Success ||
                             eImgAcquiredError == ResultHandle::Suboptimal))
    {
        _Game_engine_Assert (eImgAcquiredError == ResultHandle::Success, "Reconfigure.");
        _Game_engine_Assert (Resources.GetFrameCount () == GetBufferCount (), "Should match.");

        Resources.SetWriteFrame (OutSwapchainBufferIdx);
        return true;
    }

    _Game_engine_Assert (eImgAcquiredError == ResultHandle::Timeout,
                         "Failed to acquire swapchain buffer "
                         "(and it`s not because of timeout).");
    return false;
}

bool apemode::Swapchain::OnFramePresent (apemode::CommandQueue &        CmdQueue,
                                      apemode::RenderPassResources & Resources,
                                      VkSemaphore *               pWaitSemaphores,
                                      uint32_t                    WaitSemaphoreCount)
{
    _Game_engine_Assert ((pWaitSemaphores && WaitSemaphoreCount)
                             || (!pWaitSemaphores && !WaitSemaphoreCount),
                         "Missing info.");

    const uint32_t pImgIndices[] = { Resources.GetWriteFrame () };

    VkResult eSwapchainResult = VK_SUCCESS;

    TInfoStruct<VkPresentInfoKHR> PresentDesc;
    PresentDesc->swapchainCount     = 1;
    PresentDesc->pSwapchains        = hSwapchain;
    PresentDesc->pImageIndices      = pImgIndices;
    PresentDesc->pWaitSemaphores    = pWaitSemaphores;
    PresentDesc->waitSemaphoreCount = WaitSemaphoreCount;
    PresentDesc->pResults           = &eSwapchainResult;

    VkResult ePresentResult = vkQueuePresentKHR (CmdQueue, PresentDesc);
    if (apemode_likely (ResultHandle::Succeeded (ePresentResult)))
    {
        const bool bIsOk = ResultHandle::Succeeded (eSwapchainResult);
        _Game_engine_Assert (bIsOk, "Failed to present to swapchain.");

        return apemode_likely (bIsOk);
    }

    _Game_engine_Halt ("Failed to present.");
    return false;
}