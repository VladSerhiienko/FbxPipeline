#pragma once

struct Core::GraphicsDevice::PrivateContent : public Aux::ScalableAllocPolicy,
                                              public Aux::NoCopyAssignPolicy
{
    struct NativeDeviceWrapper : public Aux::ScalableAllocPolicy,
                                 public Aux::NoCopyAssignPolicy
    {
        typedef std::vector<float> FloatVector;
        typedef TInfoStruct<VkLayerProperties>::Vector       VkLayerPropertiesVector;
        typedef TInfoStruct<VkQueueFamilyProperties>::Vector VkQueueFamilyPropertiesVector;
        typedef TInfoStruct<VkDeviceQueueCreateInfo>::Vector VkDeviceQueueCreateInfoVector;

        typedef Core::GraphicsHeterogeneousMultiadapterEcosystem   GraphicsEcosystem;
        typedef GraphicsEcosystem::PrivateContent                  GraphicsEcosystemContent;
        typedef GraphicsEcosystemContent::NativeLayerWrapper       NativeLayerWrapper;
        typedef GraphicsEcosystemContent::NativeLayerWrapperVector NativeLayerWrapperVector;
        typedef GraphicsEcosystemContent::LpstrVector              LpstrVector;
        typedef GraphicsEcosystem::PrivateCreateDeviceArgs         CreateArgs;
        typedef VkFormatProperties                                 VkFormatPropertiesArray[VK_FORMAT_RANGE_SIZE];

        TDispatchableHandle<VkDevice>               LogicalDeviceHandle;
        VkPhysicalDevice                            AdapterHandle;
        VkPhysicalDeviceProperties                  AdapterProps;
        VkQueueFamilyPropertiesVector               QueueProps;
        VkDeviceQueueCreateInfoVector               QueueReqs;
        VkPhysicalDeviceMemoryProperties            MemoryProps;
        VkPhysicalDeviceFeatures                    Features;
        FloatVector                                 QueuePrioritiesStorage;
        VkFormatPropertiesArray                     FormatProperties;
        NativeLayerWrapperVector                    LayerWrappers;
        LpstrVector                                 PresentLayers;
        std::vector<VkExtensionProperties> DeviceExtensionProps;

        bool ScanDeviceQueues();
        bool ScanDeviceLayerProperties();
        bool ScanFormatProperties();
        NativeLayerWrapper & GetUnnamedLayer();

        bool SupportsGraphics(uint32_t QueueFamilyId);
        bool SupportsCompute(uint32_t QueueFamilyId);
        bool SupportsSparseBinding(uint32_t QueueFamilyId);
        bool SupportsTransfer(uint32_t QueueFamilyId);

    public:
        NativeDeviceWrapper();
        bool Recreate(CreateArgs * Args);
    };

    typedef Aux::TSafeDeleteObjOp<Core::RenderPassManager> RenderPassManagerDeleter;
    typedef Aux::TSafeDeleteObjOp<Core::FramebufferManager> FramebufferManagerDeleter;
    typedef Aux::TSafeDeleteObjOp<Core::RootSignatureManager> RootSignatureManagerDeleter;
    typedef Aux::TSafeDeleteObjOp<Core::PipelineStateManager> PipelineStateManagerDeleter;
    typedef Aux::TSafeDeleteObjOp<Core::ShaderManager> ShaderManagerDeleter;

    NativeDeviceWrapper DeviceHandle;
    std::unique_ptr<Core::RenderPassManager, RenderPassManagerDeleter> RenderPassManager;
    std::unique_ptr<Core::FramebufferManager, FramebufferManagerDeleter> FramebufferManager;
    std::unique_ptr<Core::RootSignatureManager, RootSignatureManagerDeleter> RootSignatureManager;
    std::unique_ptr<Core::PipelineStateManager, PipelineStateManagerDeleter> PipelineStateManager;
    std::unique_ptr<Core::ShaderManager, ShaderManagerDeleter> ShaderManager;

    PrivateContent();

};