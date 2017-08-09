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

    class NuklearRendererSdlVk : public NuklearRendererSdlBase {
    public:
        struct InitParametersVk : InitParametersBase {
            VkAllocationCallbacks *pAlloc          = nullptr;        /* Null is ok */
            VkDevice               pDevice         = VK_NULL_HANDLE; /* Required */
            VkPhysicalDevice       pPhysicalDevice = VK_NULL_HANDLE; /* Required */
            VkDescriptorPool       pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass           pRenderPass     = VK_NULL_HANDLE; /* Required */

            /* User either provides a command buffer or a queue (with family id).
             * In case the command buffer is null, it will be allocated
             * and submitted to the queue and synchonized.
             */

            VkCommandBuffer pCmdBuffer    = VK_NULL_HANDLE; /* Optional (for uploading font img) */
            VkQueue         pQueue        = VK_NULL_HANDLE; /* Optional (for uploading font img) */
            uint32_t        queueFamilyId = 0;              /* Optional (for uploading font img) */
            uint32_t        FrameCount    = 0;              /* Required, swapchain img count typically */
        };

        struct RenderParametersVk : RenderParametersBase {
            VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE; /* Required */
            uint32_t        FrameIndex = 0;              /* Required */
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
        uint32_t                                         VertexBufferSize[ kMaxFrameCount ] = {0};
        uint32_t                                         IndexBufferSize[ kMaxFrameCount ]  = {0};

    public:
        virtual bool  Render( RenderParametersBase *render_params ) override;
        virtual void  DeviceDestroy( ) override;
        virtual bool  DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( InitParametersBase *init_params, const void *image, int width, int height ) override;
    };

} // namespace apemode