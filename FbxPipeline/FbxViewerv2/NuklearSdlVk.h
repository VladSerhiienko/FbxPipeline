#pragma once

#include <NuklearSdlBase.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsManager.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <PipelineState.Vulkan.h>
#include <PipelineLayout.Vulkan.h>
#include <RenderPass.Vulkan.h>
#include <CommandQueue.Vulkan.h>

#include <TInfoStruct.Vulkan.h>

namespace apemode {

    class NuklearSdlVk : public NuklearSdlBase {
    public:
        struct InitParametersVk : InitParametersBase {
            VkAllocationCallbacks *pAlloc = nullptr;
            VkDevice pDevice = VK_NULL_HANDLE;
            VkPhysicalDevice pPhysicalDevice = VK_NULL_HANDLE;
            VkDescriptorPool pDescPool = VK_NULL_HANDLE;
            VkRenderPass pRenderPass = VK_NULL_HANDLE;
            VkCommandBuffer pCmdBuffer  = VK_NULL_HANDLE;
        };

        struct RenderParametersVk : RenderParametersBase {
            VkCommandBuffer pCmdBuffer = nullptr;
        };

    public:
        VkAllocationCallbacks *pAlloc = nullptr;
        VkDevice pDevice = VK_NULL_HANDLE;
        VkPhysicalDevice pPhysicalDevice = VK_NULL_HANDLE;
        VkDescriptorPool pDescPool = VK_NULL_HANDLE;
        VkRenderPass pRenderPass = VK_NULL_HANDLE;
        VkCommandBuffer pCmdBuffer = VK_NULL_HANDLE;
        apemodevk::TDescriptorSets< 1 >   DescSet;
        apemodevk::TDispatchableHandle< VkSampler >             hFontSampler;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDispatchableHandle< VkImage >               hFontImg;
        apemodevk::TDispatchableHandle< VkImageView >           hFontImgView;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hFontImgMemory;
        apemodevk::TDispatchableHandle< VkBuffer >              hUploadBuffer;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hUploadBufferMemory;
        apemodevk::TDispatchableHandle< VkBuffer >              hVertexBuffer[2];
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hVertexBufferMemory[2];
        apemodevk::TDispatchableHandle< VkBuffer >              hIndexBuffer[2];
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hIndexBufferMemory[2];
        size_t                 VertexBufferSize[2] = {};
        size_t                 IndexBufferSize[2] = {};
        size_t                 FrameIndex = 0;

        //VkRenderPass           RenderPass              = VK_NULL_HANDLE;
        //VkPipelineCache        PipelineCache           = VK_NULL_HANDLE;
        //VkDescriptorPool       DescriptorPool          = VK_NULL_HANDLE;
        //VkCommandBuffer        CommandBuffer           = VK_NULL_HANDLE;
        //size_t                 BufferMemoryAlignment   = 256;
        //VkPipelineCreateFlags  PipelineCreateFlags     = 0;
        //int                    FrameIndex              = 0;
        //VkDescriptorSetLayout  DescSetLayout           = VK_NULL_HANDLE;
        ////VkPipelineLayout       PipelineLayout          = VK_NULL_HANDLE;
        ////VkDescriptorSet        DescriptorSet           = VK_NULL_HANDLE;
        //VkPipeline             Pipeline                = VK_NULL_HANDLE;
        //VkSampler              FontSampler             = VK_NULL_HANDLE;
        //VkDeviceMemory         FontMemory              = VK_NULL_HANDLE;
        //VkImage                FontImage               = VK_NULL_HANDLE;
        //VkImageView            FontView                = VK_NULL_HANDLE;
        //VkDeviceMemory         VertexBufferMemory[ 2 ] = {};
        //VkDeviceMemory         IndexBufferMemory[ 2 ]  = {};
        //VkBuffer               VertexBuffer[ 2 ]       = {};
        //VkBuffer               IndexBuffer[ 2 ]        = {};
        //VkDeviceMemory         UploadBufferMemory      = VK_NULL_HANDLE;
        //VkBuffer               UploadBuffer            = VK_NULL_HANDLE;

        void ( *CheckVkResult )( VkResult err ) = NULL;

    public:
        virtual void Render( RenderParametersBase *render_params ) override;
        virtual void DeviceDestroy( ) override;
        virtual void DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( InitParametersBase *init_params, const void *image, int width, int height ) override;
    };

} // namespace apemode