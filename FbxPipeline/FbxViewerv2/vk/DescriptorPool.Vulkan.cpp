//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <DescriptorPool.Vulkan.h>

#include <CommandQueue.Vulkan.h>
#include <RootSignature.Vulkan.h>

apemode::DescriptorPool::DescriptorPool () : pGraphicsNode (nullptr)
{
}

apemode::DescriptorPool::~DescriptorPool()
{
}

uint32_t apemode::DescriptorPool::GetAvailableSetCount () const
{
    return DescSetCounter;
}

uint32_t apemode::DescriptorPool::GetAvailableDescCount (VkDescriptorType DescType) const
{
    _Game_engine_Assert (DescPoolCounters[ DescType ].type == DescType,
                         "Desc type mismatch.");

    return DescPoolCounters[ DescType ].descriptorCount;
}

bool apemode::DescriptorPool::RecreateResourcesFor (GraphicsDevice & GraphicsNode,
                                                 uint32_t         MaxSets,
                                                 uint32_t         MaxSamplerCount,
                                                 uint32_t         MaxCombinedImgCount,
                                                 uint32_t         MaxSampledImgCount,
                                                 uint32_t         MaxStorageImgCount,
                                                 uint32_t         MaxUniformTexelBufferCount,
                                                 uint32_t         MaxStorageTexelBufferCount,
                                                 uint32_t         MaxUniformBufferCount,
                                                 uint32_t         MaxStorageBufferCount,
                                                 uint32_t         MaxDynamicUniformBufferCount,
                                                 uint32_t         MaxDynamicStorageBufferCount,
                                                 uint32_t         MaxInputAttachmentCount)
{
    DescSetCounter = MaxSets;

    // This array is used for creating descriptor pool.
    VkDescriptorPoolSize LclSubpoolSizes[ VK_DESCRIPTOR_TYPE_RANGE_SIZE ];

    apemode::ZeroMemory (DescPoolCounters);
    apemode::ZeroMemory (LclSubpoolSizes);

    uint32_t DescTypeCounter       = 0;
    uint32_t SubpoolSizeCounter    = 0;
    uint32_t LclSubpoolSizeCounter = 0;
    for (const auto MaxDescTypeCount : { MaxSamplerCount,
                                         MaxCombinedImgCount,
                                         MaxSampledImgCount,
                                         MaxStorageImgCount,
                                         MaxUniformTexelBufferCount,
                                         MaxStorageTexelBufferCount,
                                         MaxUniformBufferCount,
                                         MaxStorageBufferCount,
                                         MaxDynamicUniformBufferCount,
                                         MaxDynamicStorageBufferCount,
                                         MaxInputAttachmentCount })
    {
        VkDescriptorType DescType = static_cast<VkDescriptorType> (DescTypeCounter);

        DescPoolCounters[ SubpoolSizeCounter ].descriptorCount = MaxDescTypeCount;
        DescPoolCounters[ SubpoolSizeCounter ].type            = DescType;

        if (_Game_engine_Unlikely (MaxDescTypeCount))
        {
            LclSubpoolSizes[ LclSubpoolSizeCounter ] = DescPoolCounters[ SubpoolSizeCounter ];
            ++LclSubpoolSizeCounter;
        }

        ++SubpoolSizeCounter;
        ++DescTypeCounter;
    }

    // TOFIX Does it make sense creating empty descriptor pool?
    // TOFIX Is it required by certain API functions just to provide a valid (even empty) pool?
    _Game_engine_Assert (LclSubpoolSizeCounter && MaxSets, "Empty descriptor pool.");

    TInfoStruct<VkDescriptorPoolCreateInfo> DescPoolDesc;
    DescPoolDesc->maxSets       = MaxSets;
    DescPoolDesc->pPoolSizes    = LclSubpoolSizes;
    DescPoolDesc->poolSizeCount = LclSubpoolSizeCounter;

    if (_Game_engine_Likely (hDescPool.Recreate (GraphicsNode, DescPoolDesc)))
    {
        pGraphicsNode = &GraphicsNode;
        return true;
    }

    return false;
}

apemode::DescriptorPool::operator VkDescriptorPool() const
{
    return hDescPool;
}

apemode::DescriptorSet::DescriptorSet() : pDescPool(nullptr)
                                     , pRootSign(nullptr)
                                     , pGraphicsNode(nullptr)
{
}

apemode::DescriptorSet::~DescriptorSet()
{
    if (pDescPool)
    {
    }
}

bool apemode::DescriptorSet::RecreateResourcesFor (GraphicsDevice & GraphicsNode,
                                                DescriptorPool & DescPool,
                                                RootSignature &  RootSign,
                                                uint32_t         DescSetLayoutIndex)
{
    if (DescPool.DescSetCounter >= 1)
    {
        TInfoStruct<VkDescriptorSetAllocateInfo> AllocInfo;
        AllocInfo->pSetLayouts        = &RootSign.DescSetLayouts[ DescSetLayoutIndex ];
        AllocInfo->descriptorPool     = DescPool;
        AllocInfo->descriptorSetCount = 1;

        if (_Game_engine_Likely (hDescSet.Recreate (GraphicsNode, DescPool, AllocInfo)))
        {
            pDescPool     = &DescPool;
            pRootSign     = &RootSign;
            pGraphicsNode = &GraphicsNode;

            --DescPool.DescSetCounter;
            return true;
        }
    }

    return false;
}

void apemode::DescriptorSet::BindTo (apemode::CommandList & CmdList,
                                  VkPipelineBindPoint PipelineBindPoint,
                                  uint32_t            DynamicOffsetCount,
                                  const uint32_t *    DynamicOffsets)
{
    _Game_engine_Assert (pRootSign != nullptr && hDescSet.IsNotNull (), "Not initialized.");
    if (_Game_engine_Likely (pRootSign != nullptr && hDescSet.IsNotNull ()))
    {
        vkCmdBindDescriptorSets (CmdList,
                                 PipelineBindPoint,
                                 *pRootSign,
                                 0,
                                 1,
                                 hDescSet,
                                 DynamicOffsetCount,
                                 DynamicOffsets);
    }
}

apemode::DescriptorSet::operator VkDescriptorSet() const
{
    return hDescSet;
}

apemode::DescriptorSetUpdater::DescriptorSetUpdater ()
{
}

void apemode::DescriptorSetUpdater::Reset (uint32_t MaxWrites, uint32_t MaxCopies)
{
    pGraphicsNode = nullptr;

    BufferInfos.clear ();
    ImgInfos.clear ();
    Writes.clear ();
    Copies.clear ();

    BufferInfos.reserve (MaxWrites);
    ImgInfos.reserve (MaxWrites);
    Writes.reserve (MaxWrites);
    Copies.reserve (MaxCopies);
}

bool apemode::DescriptorSetUpdater::SetGraphicsNode (GraphicsDevice const & GraphicsNode)
{
    if (_Game_engine_Likely (pGraphicsNode))
    {
        _Game_engine_Assert (pGraphicsNode == &GraphicsNode,
                             "Descriptor sets of different devices.");
        return pGraphicsNode == &GraphicsNode;
    }

    pGraphicsNode = &GraphicsNode;
    return true;
}

bool apemode::DescriptorSetUpdater::WriteUniformBuffer (DescriptorSet const & DescSet,
                                                     VkBuffer              Buffer,
                                                     uint32_t              Offset,
                                                     uint32_t              Range,
                                                     uint32_t              Binding,
                                                     uint32_t              Count)
{
    if (DescSet.pDescPool->GetAvailableDescCount (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) < Count)
    {
        _Game_engine_Halt ("Reserve more.");
        return false;
    }

    if (!SetGraphicsNode (*DescSet.pGraphicsNode))
    {
        return false;
    }

    uintptr_t BufferInfoIdx = BufferInfos.size ();
    auto &    Write         = apemode::PushBackAndGet(Writes);
    auto &    BufferInfo    = apemode::PushBackAndGet(BufferInfos);

    BufferInfo.buffer = Buffer;
    BufferInfo.offset = Offset;
    BufferInfo.range  = Range;

    Write                 = TInfoStruct<VkWriteDescriptorSet> ();
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    Write.descriptorCount = Count;
    Write.dstBinding      = Binding;
    Write.dstSet          = DescSet;
    Write.pBufferInfo     = reinterpret_cast<const VkDescriptorBufferInfo *> (BufferInfoIdx);

    return true;
}

bool apemode::DescriptorSetUpdater::WriteCombinedImgSampler (DescriptorSet const & DescSet,
                                                          VkImageView           ImgView,
                                                          VkImageLayout         ImgLayout,
                                                          VkSampler             Sampler,
                                                          uint32_t              Binding,
                                                          uint32_t              Count)
{
    if (DescSet.pDescPool->GetAvailableDescCount(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) < Count)
    {
        _Game_engine_Halt ("Reserve more.");
        return false;
    }

    if (!SetGraphicsNode(*DescSet.pGraphicsNode))
    {
        return false;
    }

    uintptr_t ImgInfoIdx = ImgInfos.size();
    auto &    Write      = apemode::PushBackAndGet(Writes);
    auto &    ImgInfo    = apemode::PushBackAndGet(ImgInfos);

    ImgInfo.sampler     = Sampler;
    ImgInfo.imageView   = ImgView;
    ImgInfo.imageLayout = ImgLayout;

    Write                 = TInfoStruct<VkWriteDescriptorSet>();
    Write.dstSet          = DescSet;
    Write.dstBinding      = Binding;
    Write.descriptorCount = Count;
    Write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Write.pImageInfo      = reinterpret_cast<const VkDescriptorImageInfo *>(ImgInfoIdx);

    return true;
}

void apemode::DescriptorSetUpdater::Flush()
{
    for (auto & Write : Writes)
    {
        if (Write.pBufferInfo)
        {
            uintptr_t BufferInfoIdx = reinterpret_cast<const uintptr_t>(Write.pBufferInfo);
            Write.pBufferInfo = &BufferInfos[BufferInfoIdx];
        }
        else if (Write.pImageInfo)
        {
            uintptr_t ImgInfoIdx = reinterpret_cast<const uintptr_t>(Write.pImageInfo);
            Write.pImageInfo = &ImgInfos[ImgInfoIdx];
        }
    }

    _Game_engine_Assert (pGraphicsNode != nullptr,
                         "No writes or copies were submitted.");

    if (_Game_engine_Likely (pGraphicsNode != nullptr))
    {
        vkUpdateDescriptorSets (*pGraphicsNode,
                                _Get_collection_length_u (Writes),
                                Writes.empty () ? nullptr : Writes.data (),
                                _Get_collection_length_u (Copies),
                                Copies.empty () ? nullptr : Copies.data ());
    }
}
