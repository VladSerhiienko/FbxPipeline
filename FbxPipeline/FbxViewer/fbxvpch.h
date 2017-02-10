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
#include <type_traits>
#include <vector>

//
// ThirdParty
//


#include <SDL.h>
#include <SDL_syswm.h>
#include <bgfx/platform.h>
#include <bgfx/bgfx.h>

#include <stdio.h>
#include <bx/thread.h>
#include <bx/handlealloc.h>
#include <bx/readerwriter.h>
#include <bx/crtimpl.h>

#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/util.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

//
// Generated
//

#include <scene_generated.h>

//
//
//

#include <ArrayUtils.h>
