#pragma once

#include <GraphicsDevice.Vulkan.h>
#include <TInfoStruct.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>

namespace Core
{
    class ShaderBytecode;
    class ShaderReflection;

    class _Graphics_ecosystem_dll_api ShaderObject : public Aux::ScalableAllocPolicy,
                                                     public Aux::NoCopyAssignPolicy
    {
    public:
        std::string                              Id;
        std::string                              MainFn;
        uint64_t                                  BytecodeHash;
        VkShaderStageFlags                        StageFlags;
        ShaderReflection *                        pReflection;
        Core::TDispatchableHandle<VkShaderModule> pShaderModule;
    };

    class _Graphics_ecosystem_dll_api ShaderManager : public Aux::ScalableAllocPolicy,
                                                      public Aux::NoCopyAssignPolicy
    {
        struct PrivateContent;
        friend PrivateContent;
        friend GraphicsDevice;

        PrivateContent * pImpl;

    public:
        ShaderManager();
        ~ShaderManager();

        Core::TDispatchableHandle<VkShaderModule> &&
        GetShaderModule (Core::GraphicsDevice & GraphicsNode,
                         ShaderBytecode const & Bytecode);

        /** Not implemented. */
        ShaderReflection const * Reflect (Core::GraphicsDevice & GraphicsNode,
                                          ShaderBytecode const & Bytecode);
    };
}