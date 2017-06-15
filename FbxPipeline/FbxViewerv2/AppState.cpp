#include "AppState.h"

apemode::AppState* gState = nullptr;

apemode::AppState* apemode::AppState::GetCurrentState( ) {
    return gState;
}

static std::string GetExecutableFileName( ) {
    char szFileName[ 1024 ];
    GetModuleFileNameA( NULL, szFileName, 1024 );
    return szFileName;
}

apemode::AppState::AppState( )
    : msvcLogger( spdlog::create< spdlog::sinks::msvc_sink_st >( "apemode/msvc" ) )
    , consoleLogger( spdlog::create< spdlog::sinks::stdout_sink_st >( "apemode/stdout" ) )
    , appOptions( new cxxopts::Options( GetExecutableFileName( ) ) ) {
    assert( nullptr == gState && "Single instance, controlled by AppContent (App)." );
    gState = this;
}

apemode::AppState::~AppState( ) {
    assert( nullptr != gState );
    gState = nullptr;
}
