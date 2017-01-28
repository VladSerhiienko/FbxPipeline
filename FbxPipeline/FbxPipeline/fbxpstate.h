#pragma once

#include <fbxppch.h>
#include <scene_generated.h>

namespace fbxp {
    struct State {
        fbxsdk::FbxManager*               manager = nullptr;
        fbxsdk::FbxScene*                 scene   = nullptr;
        std::shared_ptr< spdlog::logger > console;
        flatbuffers::FlatBufferBuilder    builder;
        cxxopts::Options                  options;
        std::string                       fileName;
        std::string                       folderPath;
        std::map< uint64_t, std::string > nameLookup;
        std::vector< fb::TransformFb >    transforms;

        State( );
        ~State( );

        bool Initialize( );
        void Release( );
        bool Load( const char* fileName );
        bool Finish( );
        uint64_t PushName( std::string const& name );

        friend State& GetState( );
    };
}