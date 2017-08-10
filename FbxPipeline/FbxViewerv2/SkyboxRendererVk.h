#pragma once

#include <MathfuInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ImageLoaderVk.h>

namespace apemodevk {

    class Skybox {
    public:
        apemodevk::LoadedImage * pSkyboxImg;
    }; 

    class SkyboxRenderer {
    public:
        struct SkyboxUpdateParameters {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        void UpdateSkybox( Skybox* pSkybox, const SkyboxUpdateParameters* pParams );

        struct SkyboxRenderParameters {
            apemodevk::GraphicsDevice* pNode = nullptr;             /* Required */
            apemodem::vec2             Dims;                        /* Required */
            apemodem::vec2             Scale;                       /* Required */
            uint32_t                   FrameIndex = 0;              /* Required */
            VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
            apemodem::mat4             EnvMatrix;                  /* Required */
            apemodem::mat4             ProjMatrix;                  /* Required */
        };

        void Reset( uint32_t FrameIndex );
        void RenderSkybox( Skybox* pSkybox, const SkyboxRenderParameters* pParams );
        void Flush( uint32_t FrameIndex );

        static uint32_t const kMaxFrameCount = 3;

        apemodevk::GraphicsDevice* pNode = nullptr;
        apemodevk::TDescriptorSets< kMaxFrameCount >            DescSets;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        apemodevk::TDispatchableHandle< VkBuffer >              hVertexBuffer;
        apemodevk::TDispatchableHandle< VkDeviceMemory >        hVertexBufferMemory;
    };
}