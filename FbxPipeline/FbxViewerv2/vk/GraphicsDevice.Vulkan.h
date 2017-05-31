#pragma once

#include <GraphicsManager.Vulkan.h>

namespace apemode {
    class Swapchain;
    class CommandQueue;
    class ResourceReference;
    class RenderPassManager;
    class PipelineLayoutManager;
    class PipelineStateManager;
    class ShaderManager;
    class FramebufferManager;
    class GraphicsManager;

    class GraphicsDevice : public apemode::ScalableAllocPolicy, public apemode::NoCopyAssignPolicy {
    public:
        static std::unique_ptr< GraphicsDevice > MakeNewUnique( );
        static std::unique_ptr< GraphicsDevice > MakeNullUnique( );

    public:
        GraphicsDevice( );
        ~GraphicsDevice( );

        bool RecreateResourcesFor( VkPhysicalDevice AdapterHandle, GraphicsManager &GraphicsEcosystem );

        operator VkDevice( ) const;
        operator VkPhysicalDevice( ) const;
        operator VkInstance( ) const;

        bool     IsValid( ) const;
        bool     Await( );
        uint32_t GetQueueFamilyCount( );
        uint32_t GetQueueCountInQueueFamily( uint32_t QueueFamilyId );
        GraphicsManager &GetGraphicsEcosystem( );

        ShaderManager &        GetDefaultShaderManager( );
        RenderPassManager &    GetDefaultRenderPassManager( );
        FramebufferManager &   GetDefaultFramebufferManager( );
        PipelineStateManager & GetDefaultPipelineStateManager( );
        PipelineLayoutManager &GetDefaultPipelineLayoutManager( );

        typedef std::vector< float >                           FloatVector;
        typedef TInfoStruct< VkLayerProperties >::Vector       VkLayerPropertiesVector;
        typedef TInfoStruct< VkQueueFamilyProperties >::Vector VkQueueFamilyPropertiesVector;
        typedef TInfoStruct< VkDeviceQueueCreateInfo >::Vector VkDeviceQueueCreateInfoVector;

        typedef GraphicsManager::NativeLayerWrapper       NativeLayerWrapper;
        typedef GraphicsManager::NativeLayerWrapperVector NativeLayerWrapperVector;
        typedef GraphicsManager::LpstrVector              LpstrVector;
        typedef VkFormatProperties                        VkFormatPropertiesArray[ VK_FORMAT_RANGE_SIZE ];

        TDispatchableHandle< VkDevice >      LogicalDeviceHandle;
        VkPhysicalDevice                     AdapterHandle;
        VkPhysicalDeviceProperties           AdapterProps;
        VkQueueFamilyPropertiesVector        QueueProps;
        VkDeviceQueueCreateInfoVector        QueueReqs;
        VkPhysicalDeviceMemoryProperties     MemoryProps;
        VkPhysicalDeviceFeatures             Features;
        FloatVector                          QueuePrioritiesStorage;
        VkFormatPropertiesArray              FormatProperties;
        NativeLayerWrapperVector             LayerWrappers;
        LpstrVector                          PresentLayers;
        std::vector< VkExtensionProperties > DeviceExtensionProps;

        bool                ScanDeviceQueues( );
        bool                ScanDeviceLayerProperties( );
        bool                ScanFormatProperties( );
        NativeLayerWrapper &GetUnnamedLayer( );

        bool SupportsGraphics( uint32_t QueueFamilyId );
        bool SupportsCompute( uint32_t QueueFamilyId );
        bool SupportsSparseBinding( uint32_t QueueFamilyId );
        bool SupportsTransfer( uint32_t QueueFamilyId );

    private:
        struct PrivateContent;

    private:
        friend Swapchain;
        friend ResourceReference;
        friend PrivateContent;
        friend GraphicsManager;

    private:
        operator PrivateContent &( );

        typedef apemode::TSafeDeleteObjOp< apemode::RenderPassManager >     RenderPassManagerDeleter;
        typedef apemode::TSafeDeleteObjOp< apemode::FramebufferManager >    FramebufferManagerDeleter;
        typedef apemode::TSafeDeleteObjOp< apemode::PipelineLayoutManager > PipelineLayoutManagerDeleter;
        typedef apemode::TSafeDeleteObjOp< apemode::PipelineStateManager >  PipelineStateManagerDeleter;
        typedef apemode::TSafeDeleteObjOp< apemode::ShaderManager >         ShaderManagerDeleter;

        std::unique_ptr< apemode::RenderPassManager, RenderPassManagerDeleter >         RenderPassManager;
        std::unique_ptr< apemode::FramebufferManager, FramebufferManagerDeleter >       FramebufferManager;
        std::unique_ptr< apemode::PipelineLayoutManager, PipelineLayoutManagerDeleter > PipelineLayoutManager;
        std::unique_ptr< apemode::PipelineStateManager, PipelineStateManagerDeleter >   PipelineStateManager;
        std::unique_ptr< apemode::ShaderManager, ShaderManagerDeleter >                 ShaderManager;

        GraphicsManager *pGraphicsEcosystem;
    };
}