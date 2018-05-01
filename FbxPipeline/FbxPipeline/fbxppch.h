#pragma once

#ifdef _WIN32

#if defined( FBXPIPELINE_EXPORT )
#define FBXPIPELINE_API __declspec( dllexport )
#else
#define FBXPIPELINE_API __declspec( dllimport )
#endif // FBXPIPELINE_EXPORT

#else

#define FBXPIPELINE_API

#endif // _WIN32

//
// STL
//

#include <iostream>
#include <memory>

//
// ThirdParty
//

#include <flatbuffers/flatbuffers.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// ThirdParty
// Math
//

#ifdef _WIN32
#ifndef MATHFU_COMPILE_WITHOUT_SIMD_SUPPORT
// TODO: Fix the issue and remove the definition.
// NOTE: In Release configuration crashes, probably because of unaligned memory and SSE instructions.
#define MATHFU_COMPILE_WITHOUT_SIMD_SUPPORT 1
#endif
#endif

#include <math.h>
#include <mathfu/constants.h>
#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

namespace mathfu {
    template <>
    bool InRange< vec2 >( vec2 val, vec2 range_start, vec2 range_end );
    template <>
    bool InRange< vec3 >( vec3 val, vec3 range_start, vec3 range_end );
}

//
// FBX SDK
//

#include <fbxsdk.h>

