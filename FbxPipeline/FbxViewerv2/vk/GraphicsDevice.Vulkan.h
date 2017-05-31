#pragma once

#include <GraphicsManager.Vulkan.h>

namespace apemodevk {
    class Swapchain;
    class CommandQueue;
    class ResourceReference;
    class RenderPassManager;
    class PipelineLayoutManager;
    class PipelineStateManager;
    class ShaderManager;
    class FramebufferManager;
    class GraphicsManager;

    class GraphicsDevice : public apemodevk::ScalableAllocPolicy, public apemodevk::NoCopyAssignPolicy {
    public:
        friend Swapchain;
        friend ResourceReference;
        friend GraphicsManager;

        typedef std::vector< float >                           FloatVector;
        typedef TInfoStruct< VkLayerProperties >::Vector       VkLayerPropertiesVector;
        typedef TInfoStruct< VkQueueFamilyProperties >::Vector VkQueueFamilyPropertiesVector;
        typedef TInfoStruct< VkDeviceQueueCreateInfo >::Vector VkDeviceQueueCreateInfoVector;
        typedef GraphicsManager::NativeLayerWrapper            NativeLayerWrapper;
        typedef GraphicsManager::NativeLayerWrapperVector      NativeLayerWrapperVector;
        typedef GraphicsManager::LpstrVector                   LpstrVector;
        typedef VkFormatProperties                             VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        static std::unique_ptr< GraphicsDevice > MakeNewUnique( );
        static std::unique_ptr< GraphicsDevice > MakeNullUnique( );

        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( VkPhysicalDevice AdapterHandle, GraphicsManager &GraphicsEcosystem );

        bool IsValid( ) const;
        bool Await( );

        uint32_t GetQueueFamilyCount( );
        uint32_t GetQueueCountInQueueFamily( uint32_t QueueFamilyId );

        GraphicsManager &      GetGraphicsEcosystem( );
        ShaderManager &        GetDefaultShaderManager( );
        RenderPassManager &    GetDefaultRenderPassManager( );
        FramebufferManager &   GetDefaultFramebufferManager( );
        PipelineStateManager & GetDefaultPipelineStateManager( );
        PipelineLayoutManager &GetDefaultPipelineLayoutManager( );
        NativeLayerWrapper &   GetUnnamedLayer( );

        bool SupportsGraphics( uint32_t QueueFamilyId ) const;
        bool SupportsCompute( uint32_t QueueFamilyId ) const;
        bool SupportsSparseBinding( uint32_t QueueFamilyId ) const;
        bool SupportsTransfer( uint32_t QueueFamilyId ) const;
        bool SupportsPresenting( uint32_t QueueFamilyId, VkSurfaceKHR hSurface ) const;

        bool ScanDeviceQueues( );
        bool ScanDeviceLayerProperties( );
        bool ScanFormatProperties( );

        operator VkDevice( ) const;
        operator VkPhysicalDevice( ) const;
        operator VkInstance( ) const;

        GraphicsManager *                                   pGraphicsEcosystem;
        TDispatchableHandle< VkDevice >                     LogicalDeviceHandle;
        VkPhysicalDevice                                    AdapterHandle;
        VkPhysicalDeviceProperties                          AdapterProps;
        VkQueueFamilyPropertiesVector                       QueueProps;
        VkDeviceQueueCreateInfoVector                       QueueReqs;
        VkPhysicalDeviceMemoryProperties                    MemoryProps;
        VkPhysicalDeviceFeatures                            Features;
        FloatVector                                         QueuePrioritiesStorage;
        VkFormatPropertiesArray                             FormatProperties;
        NativeLayerWrapperVector                            LayerWrappers;
        LpstrVector                                         PresentLayers;
        std::vector< VkExtensionProperties >                DeviceExtensionProps;
        std::unique_ptr< apemodevk::RenderPassManager >     RenderPassManager;
        std::unique_ptr< apemodevk::FramebufferManager >    FramebufferManager;
        std::unique_ptr< apemodevk::PipelineLayoutManager > PipelineLayoutManager;
        std::unique_ptr< apemodevk::PipelineStateManager >  PipelineStateManager;
        std::unique_ptr< apemodevk::ShaderManager >         ShaderManager;
    };
}