#pragma once

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>

namespace apemode {

    struct AppContent;
    struct AppState {
        std::shared_ptr< spdlog::logger > msvcLogger;
        std::shared_ptr< spdlog::logger > consoleLogger;
        std::unique_ptr< cxxopts::Options > appOptions;
        AppState * GetCurrentState();

    private:
        friend AppContent;
        AppState();
        ~AppState();

    };
}
