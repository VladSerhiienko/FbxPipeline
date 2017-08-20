#include <AppState.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

apemode::AppState* gState = nullptr;

apemode::AppState* apemode::AppState::GetCurrentState( ) {
    return gState;
}

/* TODO Move to OS utilities */
static std::string GetExecutableFileName( ) {
    char szFileName[ 1024 ] = {0};

#ifdef _WINDOWS_ /* TODO For other OSs empty string will be returned */
    GetModuleFileNameA( NULL, szFileName, 1024 );
#endif

    return szFileName;
}

apemode::AppState::AppState( )
    : msvcLogger( spdlog::create< spdlog::sinks::msvc_sink_mt >( "apemode/msvc" ) )
    , consoleLogger( spdlog::create< spdlog::sinks::stdout_sink_mt >( "apemode/stdout" ) )
    , appOptions( new cxxopts::Options( GetExecutableFileName( ) ) ) {
    assert( nullptr == gState && "Single instance, controlled by AppContent (App)." );
    gState = this;

#ifdef _DEBUG
    consoleLogger->set_level( spdlog::level::debug );
    msvcLogger->set_level( spdlog::level::debug );
#endif
}

apemode::AppState::~AppState( ) {
    assert( nullptr != gState );
    gState = nullptr;
}
