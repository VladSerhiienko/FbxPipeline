#pragma once

#include <MathfuInc.h>
#include <GraphicsDevice.Vulkan.h>
#include <DescriptorPool.Vulkan.h>
#include <BufferPools.Vulkan.h>
#include <ImageLoaderVk.h>

namespace apemodevk {

    class Skybox {
    public:
        VkSampler     pSampler;
        uint32_t      Dimension;
        uint32_t      LevelOfDetail;
        float         Exposure;
        VkImageView   pImgView;
        VkImageLayout eImgLayout;
    };

    class SkyboxRenderer {
    public:
        struct RecreateParameters {
            apemodevk::GraphicsDevice* pNode           = nullptr;        /* Required */
            apemodevk::ShaderCompiler* pShaderCompiler = nullptr;        /* Required */
            VkDescriptorPool           pDescPool       = VK_NULL_HANDLE; /* Required */
            VkRenderPass               pRenderPass     = VK_NULL_HANDLE; /* Required */
            uint32_t                   FrameCount      = 0;              /* Required */
        };

        bool Recreate( RecreateParameters* pParams );

        struct RenderParameters {
            apemodevk::GraphicsDevice* pNode       = nullptr;       /* Required */
            float                      FieldOfView = 0;             /* Required */
            uint32_t                   FrameIndex  = 0;             /* Required */
            apemodem::vec2             Dims;                        /* Required */
            apemodem::vec2             Scale;                       /* Required */
            apemodem::mat4             InvViewMatrix;               /* Required */
            apemodem::mat4             InvProjMatrix;               /* Required */
            apemodem::mat4             ProjBiasMatrix;                  /* Required */
            VkCommandBuffer            pCmdBuffer = VK_NULL_HANDLE; /* Required */
        };

        void Reset( uint32_t FrameIndex );
        bool Render( Skybox* pSkybox, RenderParameters* pParams );
        void Flush( uint32_t FrameIndex );

        static uint32_t const kMaxFrameCount = 3;

        apemodevk::GraphicsDevice*                              pNode = nullptr;
        apemodevk::TDispatchableHandle< VkDescriptorSetLayout > hDescSetLayout;
        apemodevk::TDispatchableHandle< VkPipelineLayout >      hPipelineLayout;
        apemodevk::TDispatchableHandle< VkPipelineCache >       hPipelineCache;
        apemodevk::TDispatchableHandle< VkPipeline >            hPipeline;
        //apemodevk::TDispatchableHandle< VkBuffer >              hVertexBuffer;
        //apemodevk::TDispatchableHandle< VkDeviceMemory >        hVertexBufferMemory;
        apemodevk::TDescriptorSets< kMaxFrameCount >            DescSets;
        apemodevk::HostBufferPool                               BufferPools[ kMaxFrameCount ];
        apemodevk::DescriptorSetPool                            DescSetPools[ kMaxFrameCount ];
    };
}