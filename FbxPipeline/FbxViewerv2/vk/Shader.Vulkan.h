#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemode
{
    class ShaderBytecode;
    class ShaderReflection;

    class ShaderObject : public apemode::ScalableAllocPolicy,
                                                     public apemode::NoCopyAssignPolicy
    {
    public:
        std::string                              Id;
        std::string                              MainFn;
        uint64_t                                  BytecodeHash;
        VkShaderStageFlags                        StageFlags;
        ShaderReflection *                        pReflection;
        apemode::TDispatchableHandle<VkShaderModule> pShaderModule;
    };

    class ShaderManager : public apemode::ScalableAllocPolicy,
                                                      public apemode::NoCopyAssignPolicy
    {
        struct PrivateContent;
        friend PrivateContent;
        friend GraphicsDevice;

        PrivateContent * pImpl;

    public:
        ShaderManager();
        ~ShaderManager();

        apemode::TDispatchableHandle<VkShaderModule> &&
        GetShaderModule (apemode::GraphicsDevice & GraphicsNode,
                         ShaderBytecode const & Bytecode);

        /** Not implemented. */
        ShaderReflection const * Reflect (apemode::GraphicsDevice & GraphicsNode,
                                          ShaderBytecode const & Bytecode);
    };
}