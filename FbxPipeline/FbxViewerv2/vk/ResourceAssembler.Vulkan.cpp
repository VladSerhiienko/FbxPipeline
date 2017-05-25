//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <ResourceAssembler.Vulkan.h>

std::shared_ptr<apemode::ResourceAssembler> apemode::ResourceAssembler::MakeNewUnique ()
{
    return std::shared_ptr<apemode::ResourceAssembler> (new ResourceAssembler ());
}

std::unique_ptr<apemode::ResourceAssembler> apemode::ResourceAssembler::MakeNullUnique ()
{
    return std::unique_ptr<apemode::ResourceAssembler> (new ResourceAssembler ());
}