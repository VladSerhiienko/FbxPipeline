#pragma once

#include <NuklearSdlBase.h>

#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <GraphicsEcosystems.Vulkan.h>
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
            VkAllocationCallbacks *pAlloc      = nullptr;
            GraphicsDevice *       pDevice     = nullptr;
            DescriptorPool *       pDescPool   = nullptr;
            RenderPass *           pRenderPass = nullptr;
        };

        struct RenderParametersVk : RenderParametersBase {
            CommandList *pCmdBuffer = nullptr;
        };

    public:
        VkAllocationCallbacks *pAlloc          = nullptr;
        GraphicsDevice *       pDevice         = nullptr;
        DescriptorPool *       pDescPool       = nullptr;
        RenderPass *           pRenderPass     = nullptr;
        CommandList *          pCmdBuffer      = nullptr;
        const PipelineState *  pPipelineState  = nullptr;
        const PipelineLayout * pPipelineLayout = nullptr;

        TDispatchableHandle< VkShaderModule > hVertexShaderModule;
        TDispatchableHandle< VkShaderModule > hFragmentShaderModule;
        TDispatchableHandle< VkSampler >      hFontSampler;

        VkPhysicalDevice       Gpu                     = VK_NULL_HANDLE;
        VkDevice               Device                  = VK_NULL_HANDLE;
        VkRenderPass           RenderPass              = VK_NULL_HANDLE;
        VkPipelineCache        PipelineCache           = VK_NULL_HANDLE;
        VkDescriptorPool       DescriptorPool          = VK_NULL_HANDLE;
        VkCommandBuffer        CommandBuffer           = VK_NULL_HANDLE;
        size_t                 BufferMemoryAlignment   = 256;
        VkPipelineCreateFlags  PipelineCreateFlags     = 0;
        int                    FrameIndex              = 0;
        VkDescriptorSetLayout  DescriptorSetLayout     = VK_NULL_HANDLE;
        VkPipelineLayout       PipelineLayout          = VK_NULL_HANDLE;
        VkDescriptorSet        DescriptorSet           = VK_NULL_HANDLE;
        VkPipeline             Pipeline                = VK_NULL_HANDLE;
        VkSampler              FontSampler             = VK_NULL_HANDLE;
        VkDeviceMemory         FontMemory              = VK_NULL_HANDLE;
        VkImage                FontImage               = VK_NULL_HANDLE;
        VkImageView            FontView                = VK_NULL_HANDLE;
        VkDeviceMemory         VertexBufferMemory[ 2 ] = {};
        VkDeviceMemory         IndexBufferMemory[ 2 ]  = {};
        size_t                 VertexBufferSize[ 2 ]   = {};
        size_t                 IndexBufferSize[ 2 ]    = {};
        VkBuffer               VertexBuffer[ 2 ]       = {};
        VkBuffer               IndexBuffer[ 2 ]        = {};
        VkDeviceMemory         UploadBufferMemory      = VK_NULL_HANDLE;
        VkBuffer               UploadBuffer            = VK_NULL_HANDLE;

        void ( *CheckVkResult )( VkResult err ) = NULL;

    public:
        virtual void Render( RenderParametersBase *render_params ) override;
        virtual void DeviceDestroy( ) override;
        virtual void DeviceCreate( InitParametersBase *init_params ) override;
        virtual void *DeviceUploadAtlas( const void *image, int width, int height ) override;
    };

} // namespace apemode