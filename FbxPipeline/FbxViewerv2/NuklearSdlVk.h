#pragma once

#include <NuklearSdlBase.h>

#include <CommandQueue.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <PipelineLayout.Vulkan.h>
#include <PipelineState.Vulkan.h>
#include <RenderPass.Vulkan.h>

#include <TInfoStruct.Vulkan.h>

namespace apemode {

    class NuklearSdlVk : public NuklearSdlBase {
    public:
        struct InitParametersVk : InitParametersBase {
            VkAllocationCallbacks *pAlloc          = nullptr;
            VkDevice               pDevice         = VK_NULL_HANDLE;
            VkPhysicalDevice       pPhysicalDevice = VK_NULL_HANDLE;
            VkDescriptorPool       pDescPool       = VK_NULL_HANDLE;
            VkRenderPass           pRenderPass     = VK_NULL_HANDLE;
            VkCommandBuffer        pCmdBuffer      = VK_NULL_HANDLE;
            uint32_t               FrameCount      = 0;
        };

        struct RenderParametersVk : RenderParametersBase {
            VkCommandBuffer pCmdBuffer = nullptr;
            uint32_t        FrameIndex = 0;
        };

    public:
        static uint32_t const kMaxFrameCount = 3;

        VkAllocationCallbacks *                                 pAlloc          = nullptr;
        VkDevice                                                pDevice         = VK_NULL_HANDLE;
        VkPhysicalDevice                                        pPhysicalDevice = VK_NULL_HANDLE;
        VkDescriptorPool                                        pDescPool       = VK_NULL_HANDLE;
        VkRenderPass                                            pRenderPass     = VK_NULL_HANDLE;
        VkCommandBuffer                                         pCmdBuffer      = VK_NULL_HANDLE;
        apemodevk::TDescriptorSets< 1 >                         DescSet;
        apemodevk::TDispatchableHandle< VkSampler >             hFontSampler;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDispatchableHandle< VkImage >               hFontImg;
        apemodevk::TDispatchableHandle< VkImageView >           hFontImgView;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hFontImgMemory;
        apemodevk::TDispatchableHandle< VkBuffer >              hUploadBuffer;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hUploadBufferMemory;

        apemodevk::TDispatchableHandle< VkBuffer >       hVertexBuffer[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkDeviceMemory > hVertexBufferMemory[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkBuffer >       hIndexBuffer[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkDeviceMemory > hIndexBufferMemory[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkBuffer >       hUniformBuffer[ kMaxFrameCount ];
        apemodevk::TDispatchableHandle< VkDeviceMemory > hUniformBufferMemory[ kMaxFrameCount ];
        size_t                                           VertexBufferSize[ kMaxFrameCount ] = {};
        size_t                                           IndexBufferSize[ kMaxFrameCount ]  = {};

    public:
        virtual void Render( RenderParametersBase *render_params ) override;
        virtual void DeviceDestroy( ) override;
        virtual void DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( InitParametersBase *init_params, const void *image, int width, int height ) override;
    };

} // namespace apemode