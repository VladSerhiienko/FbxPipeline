#include <SkyboxRendererVk.h>

struct FrameUniformBuffer {
    apemodem::mat4 projMarix;
    apemodem::mat4 envMarix;
    apemodem::vec4 params0;
    apemodem::vec4 params1;
};

void apemodevk::SkyboxRenderer::UpdateSkybox(Skybox * pSkybox, const SkyboxUpdateParameters * pParams)
{
}

void apemodevk::SkyboxRenderer::Reset(uint32_t FrameIndex)
{
}

void apemodevk::SkyboxRenderer::RenderSkybox(Skybox * pSkybox, const SkyboxRenderParameters * pParams)
{
}

void apemodevk::SkyboxRenderer::Flush(uint32_t FrameIndex)
{
}
