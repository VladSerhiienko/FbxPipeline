#pragma once
#include <NuklearSdlBase.h>

namespace apemode {

    class NuklearSdlVk : public NuklearSdlBase {
    public:
        struct InitParametersVk {
            VkAllocationCallbacks *allocator          = nullptr;
            VkPhysicalDevice       gpu                = VK_NULL_HANDLE;
            VkDevice               device             = VK_NULL_HANDLE;
            VkRenderPass           render_pass        = VK_NULL_HANDLE;
            VkPipelineCache        pipeline_cache     = VK_NULL_HANDLE;
            VkDescriptorPool       descriptor_pool    = VK_NULL_HANDLE;
            void ( *check_vk_result )( VkResult err ) = nullptr;
        };

    public:
        VkAllocationCallbacks *Allocator               = NULL;
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