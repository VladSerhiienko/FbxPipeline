#pragma once

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace apemode {

    class App;

    /**
     * @class AppState
     * @brief Contains members allowed for global access across the App classes
     * @note All the subsystems, that can potentially can be used as separate modules
     *       must avoid the usage of these members.
     */
    class AppState {
    public:
        std::shared_ptr< spdlog::logger >   msvcLogger;    /* Outputs to Visual Studio Output window */
        std::shared_ptr< spdlog::logger >   consoleLogger; /* Prints to console */
        std::unique_ptr< cxxopts::Options > appOptions;    /* User parameters */

        /**
         * @return Application state instance.
         * @note Returns null before the application creation, or after its destruction.
         */
        static AppState* GetCurrentState( );

    private:
        friend App;
        AppState( );
        ~AppState( );
    };
}
