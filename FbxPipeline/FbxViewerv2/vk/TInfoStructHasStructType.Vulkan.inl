#pragma once

#ifdef _Define_has_struct_type
#undef _Define_has_struct_type
#endif

#define _Define_has_struct_type(T)                                                                 \
    template <>                                                                                    \
    struct ResolveFor<T>                                                                           \
    {                                                                                              \
        static const bool bHas = true;                                                             \
    }

_Define_has_struct_type(VkCommandBufferBeginInfo);
_Define_has_struct_type(VkRenderPassBeginInfo);
_Define_has_struct_type(VkImageMemoryBarrier);
_Define_has_struct_type(VkFenceCreateInfo);
_Define_has_struct_type(VkSemaphoreCreateInfo);
_Define_has_struct_type(VkSubmitInfo);
_Define_has_struct_type(VkPresentInfoKHR);
_Define_has_struct_type(VkSwapchainCreateInfoKHR);
_Define_has_struct_type(VkImageViewCreateInfo);
_Define_has_struct_type(VkImageCreateInfo);
_Define_has_struct_type(VkMemoryAllocateInfo);
_Define_has_struct_type(VkSamplerCreateInfo);
_Define_has_struct_type(VkBufferCreateInfo);
_Define_has_struct_type(VkDescriptorSetLayoutCreateInfo);
_Define_has_struct_type(VkRenderPassCreateInfo);
_Define_has_struct_type(VkShaderModuleCreateInfo);
_Define_has_struct_type(VkPipelineDynamicStateCreateInfo);
_Define_has_struct_type(VkGraphicsPipelineCreateInfo);
_Define_has_struct_type(VkPipelineVertexInputStateCreateInfo);
_Define_has_struct_type(VkPipelineInputAssemblyStateCreateInfo);
_Define_has_struct_type(VkPipelineRasterizationStateCreateInfo);
_Define_has_struct_type(VkPipelineColorBlendStateCreateInfo);
_Define_has_struct_type(VkPipelineDepthStencilStateCreateInfo);
_Define_has_struct_type(VkPipelineViewportStateCreateInfo);
_Define_has_struct_type(VkPipelineMultisampleStateCreateInfo);
_Define_has_struct_type(VkPipelineShaderStageCreateInfo);
_Define_has_struct_type(VkPipelineLayoutCreateInfo);
_Define_has_struct_type(VkPipelineCacheCreateInfo);
_Define_has_struct_type(VkDescriptorPoolCreateInfo);
_Define_has_struct_type(VkDescriptorSetAllocateInfo);
_Define_has_struct_type(VkWriteDescriptorSet);
_Define_has_struct_type(VkFramebufferCreateInfo);
_Define_has_struct_type(VkCommandPoolCreateInfo);
_Define_has_struct_type(VkCommandBufferAllocateInfo);
_Define_has_struct_type(VkApplicationInfo);
_Define_has_struct_type(VkInstanceCreateInfo);
_Define_has_struct_type(VkDebugReportCallbackCreateInfoEXT);
_Define_has_struct_type(VkDeviceQueueCreateInfo);
_Define_has_struct_type(VkDeviceCreateInfo);
_Define_has_struct_type(VkWin32SurfaceCreateInfoKHR);
_Define_has_struct_type(VkCommandBufferInheritanceInfo);
_Define_has_struct_type(VkMappedMemoryRange);

#undef _Define_has_struct_type
