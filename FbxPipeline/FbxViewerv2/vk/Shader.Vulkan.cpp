//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <PipelineState.Vulkan.h>
#include <Shader.Vulkan.h>

#include <CityHash.h>

#define _Get_hspv_bytecode(Name) Name##_spv
#define _Get_hspv_bytecode_length(Name) _Get_array_length(_Get_hspv_bytecode(Name))
#define _Get_hspv(Name) _Get_hspv_bytecode(Name), _Get_hspv_bytecode_length(Name)

struct apemode::ShaderManager::PrivateContent : public apemode::ScalableAllocPolicy,
                                             public apemode::NoCopyAssignPolicy
{
};

apemode::ShaderManager::ShaderManager () : pImpl (new PrivateContent ())
{
}

apemode::ShaderManager::~ShaderManager ()
{
    apemode::TSafeDeleteObj (pImpl);
}

apemode::TDispatchableHandle<VkShaderModule> &&
apemode::ShaderManager::GetShaderModule (apemode::GraphicsDevice & GraphicsNode,
                                      ShaderBytecode const & Bytecode)
{
    apemode::TInfoStruct<VkShaderModuleCreateInfo> ShaderModuleDesc;
    ShaderModuleDesc->pCode    = reinterpret_cast<uint32_t const *> (Bytecode.Bytecode.data ());
    ShaderModuleDesc->codeSize = Bytecode.Bytecode.size ();

    apemode::TDispatchableHandle<VkShaderModule> hShaderModule;
    hShaderModule.Recreate (GraphicsNode, ShaderModuleDesc);

    return std::move (hShaderModule);
}

apemode::ShaderReflection const * apemode::ShaderManager::Reflect (apemode::GraphicsDevice & GraphicsNode,
                                                             ShaderBytecode const & Bytecode)
{
    return nullptr;
}