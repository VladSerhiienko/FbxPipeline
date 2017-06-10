#pragma once

#include <Platform.Vulkan.h>

//
// ThirdParty
//

#include <SDL.h>
#include <SDL_syswm.h>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// ThirdParty
// Math
//

#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED
#endif

#include <math.h>

#include <mathfu/constants.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

//
// Generated
//

#include <scene_generated.h>

//
//
//

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT 16
#endif
