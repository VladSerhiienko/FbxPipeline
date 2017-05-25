#pragma once

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

//
// ThirdParty
//

#include <SDL.h>
#include <SDL_syswm.h>

#include <stdio.h>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// ThirdParty
// Math
//

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

#include <ArrayUtils.h>

//
//
//

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT 16
#endif
