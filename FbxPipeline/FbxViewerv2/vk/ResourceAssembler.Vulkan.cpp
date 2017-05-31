//#include <GameEngine.GraphicsEcosystem.Precompiled.h>
#include <ResourceAssembler.Vulkan.h>

std::shared_ptr<apemodevk::ResourceAssembler> apemodevk::ResourceAssembler::MakeNewUnique ()
{
    return std::shared_ptr<apemodevk::ResourceAssembler> (new ResourceAssembler ());
}

std::unique_ptr<apemodevk::ResourceAssembler> apemodevk::ResourceAssembler::MakeNullUnique ()
{
    return std::unique_ptr<apemodevk::ResourceAssembler> (new ResourceAssembler ());
}