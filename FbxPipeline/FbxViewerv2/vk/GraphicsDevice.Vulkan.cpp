//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <GraphicsDevice.Vulkan.h>
#include <GraphicsManager.Vulkan.h>

#include <RenderPass.Vulkan.h>
#include <Framebuffer.Vulkan.h>
#include <PipelineLayout.Vulkan.h>
#include <PipelineState.Vulkan.h>
#include <Shader.Vulkan.h>

//#include <TDataHandle.h>

#include <TInfoStruct.Vulkan.h>
#include <ResultHandle.Vulkan.h>
#include <NativeDispatchableHandles.Vulkan.h>
#include <SystemAllocationCallbacks.Vulkan.h>

#include <GraphicsManager.KnownExtensions.Vulkan.h>
#include <GraphicsManager.PrivateContent.Vulkan.h>
#include <GraphicsManager.PrivateCreateDeviceArgs.Vulkan.h>
#include <GraphicsDevice.PrivateContent.Vulkan.h>

/// -------------------------------------------------------------------------------------------------------------------
/// GraphicsDevice PrivateContent
/// -------------------------------------------------------------------------------------------------------------------

apemode::GraphicsDevice::PrivateContent::PrivateContent()
    : RenderPassManager(new apemode::RenderPassManager())
    , FramebufferManager(new apemode::FramebufferManager())
    , PipelineLayoutManager(new apemode::PipelineLayoutManager())
    , ShaderManager(new apemode::ShaderManager())
{
}

apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::NativeLayerWrapper &
apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::GetUnnamedLayer()
{
    _Game_engine_Assert(LayerWrappers.front().IsUnnamedLayer(), "vkEnumerateInstanceExtensionProperties failed.");
    return LayerWrappers.front();
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsGraphics(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return apemode::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_GRAPHICS_BIT);
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsCompute(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return apemode::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_COMPUTE_BIT);
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsSparseBinding(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return apemode::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_SPARSE_BINDING_BIT);
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::SupportsTransfer(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < QueueProps.size(), "Out of range.");
    return apemode::HasFlagEql(QueueProps[QueueFamilyId]->queueFlags, VK_QUEUE_TRANSFER_BIT);
}

bool apemode::GraphicsManager::PrivateContent::NativeLayerWrapper::IsValidDeviceLayer() const
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

apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::NativeDeviceWrapper()
{
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanDeviceQueues()
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
            auto & QueueReq = apemode::PushBackAndGet( QueueReqs);
            QueueReq->pNext = NULL;
            QueueReq->queueFamilyIndex = static_cast<uint32_t>(std::distance(QueueProps.data(), &QueueProp));
            QueueReq->queueCount = QueueProp.queueCount;
            QueueReq->pQueuePriorities = QueuePrioritiesStorage.data() + QueuePrioritiesAssigned;

            QueuePrioritiesAssigned += QueueProp.queueCount;
        });
    }

    return true;
}

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanDeviceLayerProperties ()
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

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::ScanFormatProperties()
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

bool apemode::GraphicsDevice::PrivateContent::NativeDeviceWrapper::Recreate(CreateArgs * Args)
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

std::unique_ptr<apemode::GraphicsDevice> apemode::GraphicsDevice::MakeNewUnique ()
{
    return std::unique_ptr<GraphicsDevice>(new GraphicsDevice());
}

std::unique_ptr<apemode::GraphicsDevice> apemode::GraphicsDevice::MakeNullUnique ()
{
    return std::unique_ptr<GraphicsDevice>(nullptr);
}

apemode::GraphicsDevice::GraphicsDevice()
    : pContent(new PrivateContent())
    , pGraphicsEcosystem(nullptr)
{
    _Game_engine_Assert(pContent != nullptr, "Out of system memory");
}

apemode::GraphicsDevice::~GraphicsDevice()
{
    apemode::TSafeDeleteObj(pContent);
    _Aux_DebugTraceFunc;
}

apemode::GraphicsDevice::operator VkDevice() const
{
    return pContent->DeviceHandle.LogicalDeviceHandle;
}

apemode::GraphicsDevice::operator VkPhysicalDevice() const
{
    return pContent->DeviceHandle.AdapterHandle;
}

apemode::GraphicsDevice::operator VkInstance() const
{
    return pGraphicsEcosystem->pContent->InstanceHandle;
}

bool apemode::GraphicsDevice::IsValid() const
{
    return pContent->DeviceHandle.LogicalDeviceHandle.IsNotNull();
}

bool apemode::GraphicsDevice::Await ()
{
    _Game_engine_Assert (IsValid (), "Must be valid.");

    ResultHandle eDeviceWaitIdleOk = vkDeviceWaitIdle (*this);
    _Game_engine_Assert (eDeviceWaitIdleOk.Succeeded (), "Failed to wait for device.");

    return eDeviceWaitIdleOk.Succeeded ();
}

apemode::GraphicsManager & apemode::GraphicsDevice::GetGraphicsEcosystem()
{
    return *pGraphicsEcosystem;
}

apemode::RenderPassManager & apemode::GraphicsDevice::GetDefaultRenderPassManager()
{
    return *pContent->RenderPassManager;
}

apemode::FramebufferManager & apemode::GraphicsDevice::GetDefaultFramebufferManager()
{
    return *pContent->FramebufferManager;
}

apemode::PipelineLayoutManager & apemode::GraphicsDevice::GetDefaultPipelineLayoutManager()
{
    return *pContent->PipelineLayoutManager;
}

apemode::PipelineStateManager & apemode::GraphicsDevice::GetDefaultPipelineStateManager()
{
    return *pContent->PipelineStateManager;
}

apemode::ShaderManager & apemode::GraphicsDevice::GetDefaultShaderManager()
{
    return *pContent->ShaderManager;
}

uint32_t apemode::GraphicsDevice::GetQueueFamilyCount()
{
    return _Get_collection_length_u (pContent->DeviceHandle.QueueReqs);
}

uint32_t apemode::GraphicsDevice::GetQueueCountInQueueFamily(uint32_t QueueFamilyId)
{
    _Game_engine_Assert(QueueFamilyId < GetQueueFamilyCount(), "Out of range.");
    return pContent->DeviceHandle.QueueReqs[QueueFamilyId]->queueCount;
}

bool
apemode::GraphicsDevice::RecreateResourcesFor(void * pDeviceCreateArgs)
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

apemode::GraphicsDevice::operator PrivateContent&()
{
    return *pContent;
}
