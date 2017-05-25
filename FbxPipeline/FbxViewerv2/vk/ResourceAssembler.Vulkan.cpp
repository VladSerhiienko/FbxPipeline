//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <ResourceAssembler.Vulkan.h>

std::shared_ptr<Core::ResourceAssembler> Core::ResourceAssembler::MakeNewUnique ()
{
    return std::shared_ptr<Core::ResourceAssembler> (new ResourceAssembler ());
}

std::unique_ptr<Core::ResourceAssembler> Core::ResourceAssembler::MakeNullUnique ()
{
    return std::unique_ptr<Core::ResourceAssembler> (new ResourceAssembler ());
}