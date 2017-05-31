#pragma once

struct apemode::GraphicsDevice::PrivateContent : public apemode::ScalableAllocPolicy,
                                              public apemode::NoCopyAssignPolicy
{
    struct NativeDeviceWrapper : public apemode::ScalableAllocPolicy,
                                 public apemode::NoCopyAssignPolicy
    {
        typedef std::vector<float> FloatVector;
        typedef TInfoStruct<VkLayerProperties>::Vector       VkLayerPropertiesVector;
        typedef TInfoStruct<VkQueueFamilyProperties>::Vector VkQueueFamilyPropertiesVector;
        typedef TInfoStruct<VkDeviceQueueCreateInfo>::Vector VkDeviceQueueCreateInfoVector;

        typedef apemode::GraphicsManager   GraphicsEcosystem;
        typedef GraphicsManager::PrivateContent                  GraphicsEcosystemContent;
        typedef GraphicsEcosystemContent::NativeLayerWrapper       NativeLayerWrapper;
        typedef GraphicsEcosystemContent::NativeLayerWrapperVector NativeLayerWrapperVector;
        typedef GraphicsEcosystemContent::LpstrVector              LpstrVector;
        typedef GraphicsManager::PrivateCreateDeviceArgs         CreateArgs;
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

    typedef apemode::TSafeDeleteObjOp<apemode::RenderPassManager> RenderPassManagerDeleter;
    typedef apemode::TSafeDeleteObjOp<apemode::FramebufferManager> FramebufferManagerDeleter;
    typedef apemode::TSafeDeleteObjOp<apemode::PipelineLayoutManager> PipelineLayoutManagerDeleter;
    typedef apemode::TSafeDeleteObjOp<apemode::PipelineStateManager> PipelineStateManagerDeleter;
    typedef apemode::TSafeDeleteObjOp<apemode::ShaderManager> ShaderManagerDeleter;

    NativeDeviceWrapper DeviceHandle;
    std::unique_ptr<apemode::RenderPassManager, RenderPassManagerDeleter> RenderPassManager;
    std::unique_ptr<apemode::FramebufferManager, FramebufferManagerDeleter> FramebufferManager;
    std::unique_ptr<apemode::PipelineLayoutManager, PipelineLayoutManagerDeleter> PipelineLayoutManager;
    std::unique_ptr<apemode::PipelineStateManager, PipelineStateManagerDeleter> PipelineStateManager;
    std::unique_ptr<apemode::ShaderManager, ShaderManagerDeleter> ShaderManager;

    PrivateContent();

};