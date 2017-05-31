#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace apemodevk
{
    class ShaderBytecode;
    class ShaderReflection;

    class ShaderObject : public apemodevk::ScalableAllocPolicy,
                                                     public apemodevk::NoCopyAssignPolicy
    {
    public:
        std::string                              Id;
        std::string                              MainFn;
        uint64_t                                  BytecodeHash;
        VkShaderStageFlags                        StageFlags;
        ShaderReflection *                        pReflection;
        apemodevk::TDispatchableHandle<VkShaderModule> pShaderModule;
    };

    class ShaderManager : public apemodevk::ScalableAllocPolicy,
                                                      public apemodevk::NoCopyAssignPolicy
    {
        struct PrivateContent;
        friend PrivateContent;
        friend GraphicsDevice;

        PrivateContent * pImpl;

    public:
        ShaderManager();
        ~ShaderManager();

        apemodevk::TDispatchableHandle<VkShaderModule> &&
        GetShaderModule (apemodevk::GraphicsDevice & GraphicsNode,
                         ShaderBytecode const & Bytecode);

        /** Not implemented. */
        ShaderReflection const * Reflect (apemodevk::GraphicsDevice & GraphicsNode,
                                          ShaderBytecode const & Bytecode);
    };
}