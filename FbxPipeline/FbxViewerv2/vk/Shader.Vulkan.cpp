//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineState.Vulkan.h>
#include <Shader.Vulkan.h>

#include <CityHash.h>

#define _Get_hspv_bytecode(Name) Name##_spv
#define _Get_hspv_bytecode_length(Name) _Get_array_length(_Get_hspv_bytecode(Name))
#define _Get_hspv(Name) _Get_hspv_bytecode(Name), _Get_hspv_bytecode_length(Name)

struct apemodevk::ShaderManager::PrivateContent : public apemodevk::ScalableAllocPolicy,
                                             public apemodevk::NoCopyAssignPolicy
{
};

apemodevk::ShaderManager::ShaderManager () : pImpl (new PrivateContent ())
{
}

apemodevk::ShaderManager::~ShaderManager ()
{
    apemodevk::TSafeDeleteObj (pImpl);
}

apemodevk::TDispatchableHandle<VkShaderModule> &&
apemodevk::ShaderManager::GetShaderModule (apemodevk::GraphicsDevice & GraphicsNode,
                                      ShaderBytecode const & Bytecode)
{
    apemodevk::TInfoStruct<VkShaderModuleCreateInfo> ShaderModuleDesc;
    ShaderModuleDesc->pCode    = reinterpret_cast<uint32_t const *> (Bytecode.Bytecode.data ());
    ShaderModuleDesc->codeSize = Bytecode.Bytecode.size ();

    apemodevk::TDispatchableHandle<VkShaderModule> hShaderModule;
    hShaderModule.Recreate (GraphicsNode, ShaderModuleDesc);

    return std::move (hShaderModule);
}

apemodevk::ShaderReflection const * apemodevk::ShaderManager::Reflect (apemodevk::GraphicsDevice & GraphicsNode,
                                                             ShaderBytecode const & Bytecode)
{
    return nullptr;
}