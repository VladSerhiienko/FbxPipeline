#include <CityHash.h>
#include <SamplerManagerVk.h>

bool apemodevk::SamplerManager::Recreate( apemodevk::GraphicsDevice* pInNode ) {
    pNode = pInNode;
    return nullptr != pNode;
}

uint32_t apemodevk::SamplerManager::GetSamplerIndex( const VkSamplerCreateInfo& samplerCreateInfo ) {
    const auto samplerHash = apemode::CityHash64( samplerCreateInfo );

    auto samplerIt = std::find_if( StoredSamplers.begin( ), StoredSamplers.end( ), [&]( const StoredSampler& storedSampler ) {
        return samplerHash == storedSampler.Hash;
    } );

    if ( samplerIt != StoredSamplers.end( ) )
        return (uint32_t) std::distance( StoredSamplers.begin( ), samplerIt );

    TDispatchableHandle< VkSampler > hSampler;
    if ( hSampler.Recreate( *pNode, samplerCreateInfo ) ) {
        const uint32_t samplerIndex = StoredSamplers.size( );

        StoredSampler storedSampler;
        storedSampler.Hash              = samplerHash;
        storedSampler.pSampler          = hSampler.Release( );
        storedSampler.SamplerCreateInfo = samplerCreateInfo;
        StoredSamplers.push_back( storedSampler );

        return samplerIndex;
    }

    platform::DebugTrace( "Failed to create sampler." );
    platform::DebugBreak( );
    return UINT_ERROR;
}
