//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineState.Vulkan.h>
#include <Shader.Vulkan.h>

#include <CityHash.h>

#define _Get_hspv_bytecode(Name) Name##_spv
#define _Get_hspv_bytecode_length(Name) _Get_array_length(_Get_hspv_bytecode(Name))
#define _Get_hspv(Name) _Get_hspv_bytecode(Name), _Get_hspv_bytecode_length(Name)

struct Core::ShaderManager::PrivateContent : public Aux::ScalableAllocPolicy,
                                             public Aux::NoCopyAssignPolicy
{
};

Core::ShaderManager::ShaderManager () : pImpl (new PrivateContent ())
{
}

Core::ShaderManager::~ShaderManager ()
{
    Aux::TSafeDeleteObj (pImpl);
}

Core::TDispatchableHandle<VkShaderModule> &&
Core::ShaderManager::GetShaderModule (Core::GraphicsDevice & GraphicsNode,
                                      ShaderBytecode const & Bytecode)
{
    Core::TInfoStruct<VkShaderModuleCreateInfo> ShaderModuleDesc;
    ShaderModuleDesc->pCode    = reinterpret_cast<uint32_t const *> (Bytecode.Bytecode.data ());
    ShaderModuleDesc->codeSize = Bytecode.Bytecode.size ();

    Core::TDispatchableHandle<VkShaderModule> hShaderModule;
    hShaderModule.Recreate (GraphicsNode, ShaderModuleDesc);

    return std::move (hShaderModule);
}

Core::ShaderReflection const * Core::ShaderManager::Reflect (Core::GraphicsDevice & GraphicsNode,
                                                             ShaderBytecode const & Bytecode)
{
    return nullptr;
}