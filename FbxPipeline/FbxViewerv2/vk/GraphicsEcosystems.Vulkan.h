#pragma once

#include <GraphicsDevice.Vulkan.h>

namespace apemode
{
    class GraphicsEcosystem
        : public apemode::ScalableAllocPolicy,
          public apemode::NoCopyAssignPolicy
    {
    public:
        struct PrivateContent;
        struct PrivateCreateDeviceArgs;

        // TODO: 
        //      Add flags: allow multiple devices, prefer discrete GPUs, etc.
        enum EFlags
        {
            kFlag_None
        };

    public:
        GraphicsEcosystem ();
        ~GraphicsEcosystem ();

    public:
        inline GraphicsDevice * GetPrimaryGraphicsNode () { return PrimaryNode.get(); }
        inline GraphicsDevice * GetSecondaryGraphicsNode () { return SecondaryNode.get(); }
        inline GraphicsDevice & GetPrimaryGraphicsNodeByRef () { return *PrimaryNode; }
        inline GraphicsDevice & GetSecondaryGraphicsNodeByRef () { return *SecondaryNode; }

    public:
       bool RecreateGraphicsNodes(EFlags Flags = kFlag_None);

    private:
        typedef apemode::TSafeDeleteObjOp<PrivateContent> PrivateContentDeleter;
        typedef std::unique_ptr<PrivateContent, PrivateContentDeleter> PrivateContentUqPtr;

        friend GraphicsDevice;
        friend PrivateContent;

        // NOTE: 
        //      The destruction order is important here.
        //      Devices should be deleted before the instance is.

        PrivateContent *                pContent;
        std::unique_ptr<GraphicsDevice> PrimaryNode;
        std::unique_ptr<GraphicsDevice> SecondaryNode;
    };
}

_Game_engine_Define_enum_flag_operators(apemode::GraphicsEcosystem::EFlags);
