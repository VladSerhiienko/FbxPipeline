//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsEcosystems.Vulkan.h>

#include <RenderPass.Vulkan.h>
#include <Framebuffer.Vulkan.h>
#include <RootSignature.Vulkan.h>
#include <PipelineState.Vulkan.h>
#include <Shader.Vulkan.h>

//#include <TDataHandle.h>

#include <TInfoStruct.Vulkan.h>
#include <ResultHandle.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <SystemAllocationCallbacks.Vulkan.h>

#include <GraphicsEcosystem.KnownExtensions.Vulkan.h>
#include <GraphicsEcosystem.PrivateContent.Vulkan.h>
#include <GraphicsEcosystem.PrivateCreateDeviceArgs.Vulkan.h>
#include <GraphicsDevice.PrivateContent.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

Core::GraphicsDevice::PrivateContent::PrivateContent()
    : RenderPassManager(new Core::RenderPassManager())
    , FramebufferManager(new Core::FramebufferManager())
    , RootSignatureManager(new Core::RootSignatureManager())
    , ShaderManager(new Core::ShaderManager())
{
}

Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::NativeLayerWrapper &
Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::GetUnnamedLayer()
{
    _Game_engine_Assert(LayerWrappers.front().IsUnnamedLayer(), "vkEnumerateInstanceExtensionProperties failed.");
    return LayerWrappers.front();
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsGraphics(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return Aux::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_GRAPHICS_BIT);
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsCompute(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return Aux::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_COMPUTE_BIT);
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsSparseBinding(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return Aux::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_SPARSE_BINDING_BIT);
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsTransfer(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return Aux::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_TRANSFER_BIT);
}

bool Core::GraphicsHeterogeneousMultiadapterEcosystem::PrivateContent::NativeLayerWrapper::IsValidDeviceLayer() const
{
    if (IsUnnamedLayer())
    {
        auto const KnownExtensionBeginIt = Vulkan::KnownDeviceExtensions;
        auto const KnownExtensionEndIt   = Vulkan::KnownDeviceExtensions + Vulkan::KnownDeviceExtensionCount;

        auto Counter = [this](char const * KnownExtension) 
        {
            auto const ExtensionBeginIt = Extensions.begin();
            auto const ExtensionEndIt   = Extensions.end();

            auto Finder = [&](VkExtensionProperties const & Extension) 
            {
                static const int eStrCmp_EqualStrings = 0;
                return strcmp(KnownExtension, Extension.extensionName) == eStrCmp_EqualStrings;
            };

            auto const FoundExtensionIt  = std::find_if(ExtensionBeginIt, ExtensionEndIt, Finder);
            auto const bIsExtensionFound = FoundExtensionIt != ExtensionEndIt;
            _Game_engine_Assert(bIsExtensionFound, "Extension '%s' was not found.", KnownExtension);

            return bIsExtensionFound;
        };

        size_t KnownExtensionsFound = std::count_if(KnownExtensionBeginIt, KnownExtensionEndIt, Counter);
        return KnownExtensionsFound == Vulkan::KnownDeviceExtensionCount;
    }

    return true;
}

Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::NativeDeviceWrapper()
{
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanDeviceQueues()
{
    uint32_t QueuesFound = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(AdapterHandle, &QueuesFound, NULL);

    if (QueuesFound != 0)
    {
        QueueProps.resize(QueuesFound);
        QueueReqs.reserve(QueuesFound);

        vkGetPhysicalDeviceQueueFamilyProperties(AdapterHandle, &QueuesFound, QueueProps.data());

        uint32_t TotalQueuePrioritiesCount = 0;
        std::for_each(QueueProps.begin(), QueueProps.end(), [&](VkQueueFamilyProperties & QueueProp)
        {
            TotalQueuePrioritiesCount += QueueProp.queueCount;
        });

        uint32_t QueuePrioritiesAssigned = 0;
        QueuePrioritiesStorage.resize(TotalQueuePrioritiesCount);
        std::for_each(QueueProps.begin(), QueueProps.end(), [&](VkQueueFamilyProperties & QueueProp)
        {
            auto & QueueReq = QueueReqs.push_back();
            QueueReq->pNext = NULL;
            QueueReq->queueFamilyIndex = static_cast<uint32_t>(std::distance(QueueProps.data(), &QueueProp));
            QueueReq->queueCount = QueueProp.queueCount;
            QueueReq->pQueuePriorities = QueuePrioritiesStorage.data() + QueuePrioritiesAssigned;

            QueuePrioritiesAssigned += QueueProp.queueCount;
        });
    }

    return true;
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanDeviceLayerProperties ()
{
    ResultHandle ErrorHandle;

    uint32_t ExtPropCount  = 0;
    VkBool32 swapchainExtFound       = 0;
    uint32_t enabled_extension_count = 0;

    char * extension_names[ 64 ];
    memset (extension_names, 0, sizeof (extension_names));

    ErrorHandle = vkEnumerateDeviceExtensionProperties (AdapterHandle, NULL, &ExtPropCount, NULL);
    if (ErrorHandle && ExtPropCount)
    {
        DeviceExtensionProps.resize (ExtPropCount);
        ErrorHandle = vkEnumerateDeviceExtensionProperties (
            AdapterHandle, NULL, &ExtPropCount, DeviceExtensionProps.data ());

        return ErrorHandle;
    }

    return false;
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanFormatProperties()
{
    VkFormat NativeFormatIt = VK_FORMAT_UNDEFINED;
    for (; NativeFormatIt < VK_FORMAT_RANGE_SIZE; )
    {
        const VkFormat NativeFormat = NativeFormatIt;
        vkGetPhysicalDeviceFormatProperties(AdapterHandle, NativeFormat, &FormatProperties[NativeFormat]);
        NativeFormatIt = static_cast<VkFormat>(NativeFormatIt + 1);
    }

    return true;
}

bool Core::GraphicsDevice::PrivateContent::NativeDeviceWrapper::Recreate(CreateArgs * Args)
{
    _Game_engine_Assert(Args != nullptr, "Ecosystem is required in case of Vulkan.");

    // Physical device is required to create a related logical device.
    // Likewise, vulkan instance is required for physical device.
    if (Args != nullptr && Args->AdapterHandle)
    {
        AdapterHandle = Args->AdapterHandle;
        vkGetPhysicalDeviceProperties (AdapterHandle, &AdapterProps);
        vkGetPhysicalDeviceMemoryProperties (AdapterHandle, &MemoryProps);
        vkGetPhysicalDeviceFeatures (AdapterHandle, &Features);

        if (ScanDeviceQueues () && ScanFormatProperties () && ScanDeviceLayerProperties ())
        {
            std::vector<const char *> DeviceExtNames;
            DeviceExtNames.reserve (DeviceExtensionProps.size ());
            std::transform (
                DeviceExtensionProps.begin (),
                DeviceExtensionProps.end (),
                std::back_inserter (DeviceExtNames),
                [&](VkExtensionProperties const & ExtProp) { return ExtProp.extensionName; });

            VkPhysicalDeviceFeatures Features;
            vkGetPhysicalDeviceFeatures(AdapterHandle, &Features);

            TInfoStruct<VkDeviceCreateInfo> DeviceDesc;
            DeviceDesc->pEnabledFeatures        = &Features;
            DeviceDesc->queueCreateInfoCount    = _Get_collection_length_u (QueueReqs);
            DeviceDesc->pQueueCreateInfos       = QueueReqs.data ();
            DeviceDesc->enabledLayerCount       = _Get_collection_length_u (PresentLayers);
            DeviceDesc->ppEnabledLayerNames     = PresentLayers.data ();
            DeviceDesc->enabledExtensionCount   = _Get_collection_length_u (DeviceExtNames);
            DeviceDesc->ppEnabledExtensionNames = DeviceExtNames.data ();

            const auto bOk = LogicalDeviceHandle.Recreate(AdapterHandle, DeviceDesc);
            _Game_engine_Assert(bOk, "vkCreateDevice failed.");

            return bOk;
        }
    }

    return false;
}

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice
/// -------------------------------------------------------------------------------------------------------------------

std::unique_ptr<Core::GraphicsDevice> Core::GraphicsDevice::MakeNewUnique ()
{
    return std::unique_ptr<GraphicsDevice>(new GraphicsDevice());
}

std::unique_ptr<Core::GraphicsDevice> Core::GraphicsDevice::MakeNullUnique ()
{
    return std::unique_ptr<GraphicsDevice>(nullptr);
}

Core::GraphicsDevice::GraphicsDevice()
    : pContent(new PrivateContent())
    , pGraphicsEcosystem(nullptr)
{
    _Game_engine_Assert(pContent != nullptr, "Out of system memory");
}

Core::GraphicsDevice::~GraphicsDevice()
{
    Aux::TSafeDeleteObj(pContent);
    _Aux_DebugTraceFunc;
}

Core::GraphicsDevice::operator VkDevice() const
{
    return pContent->DeviceHandle.LogicalDeviceHandle;
}

Core::GraphicsDevice::operator VkPhysicalDevice() const
{
    return pContent->DeviceHandle.AdapterHandle;
}

Core::GraphicsDevice::operator VkInstance() const
{
    return pGraphicsEcosystem->pContent->InstanceHandle;
}

bool Core::GraphicsDevice::IsValid() const
{
    return pContent->DeviceHandle.LogicalDeviceHandle.IsNotNull();
}

bool Core::GraphicsDevice::Await ()
{
    _Game_engine_Assert (IsValid (), "Must be valid.");

    ResultHandle eDeviceWaitIdleOk = vkDeviceWaitIdle (*this);
    _Game_engine_Assert (eDeviceWaitIdleOk.Succeeded (), "Failed to wait for device.");

    return eDeviceWaitIdleOk.Succeeded ();
}

Core::GraphicsHeterogeneousMultiadapterEcosystem & Core::GraphicsDevice::GetGraphicsEcosystem()
{
    return *pGraphicsEcosystem;
}

Core::RenderPassManager & Core::GraphicsDevice::GetDefaultRenderPassManager()
{
    return *pContent->RenderPassManager;
}

Core::FramebufferManager & Core::GraphicsDevice::GetDefaultFramebufferManager()
{
    return *pContent->FramebufferManager;
}

Core::RootSignatureManager & Core::GraphicsDevice::GetDefaultRootSignatureManager()
{
    return *pContent->RootSignatureManager;
}

Core::PipelineStateManager & Core::GraphicsDevice::GetDefaultPipelineStateManager()
{
    return *pContent->PipelineStateManager;
}

Core::ShaderManager & Core::GraphicsDevice::GetDefaultShaderManager()
{
    return *pContent->ShaderManager;
}

uint32_t Core::GraphicsDevice::GetQueueFamilyCount()
{
    return _Get_collection_length_u (pContent->DeviceHandle.QueueReqs);
}

uint32_t Core::GraphicsDevice::GetQueueCountInQueueFamily(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < GetQueueFamilyCount(), "Out of range.");
    return pContent->DeviceHandle.QueueReqs[QueueFamilyId]->queueCount;
}

bool
Core::GraphicsDevice::RecreateResourcesFor(void * pDeviceCreateArgs)
{
    typedef PrivateContent::NativeDeviceWrapper::CreateArgs DeviceCreateArgs;

    _Game_engine_Assert(pDeviceCreateArgs != nullptr, "Ecosystem is required in case of Vulkan.");
    if (auto Args = reinterpret_cast<DeviceCreateArgs *>(pDeviceCreateArgs))
    {
        pGraphicsEcosystem = &Args->GraphicsEcosystem;

        const auto bOk = pContent->DeviceHandle.Recreate(Args);
        _Game_engine_Assert(bOk, "Vulkan initialization failed.");

        return bOk;
    }

    return false;
}

Core::GraphicsDevice::operator PrivateContent&()
{
    return *pContent;
}
