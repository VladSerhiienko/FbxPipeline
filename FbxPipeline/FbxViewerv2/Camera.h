#pragma once

#include <MathfuInc.h>

namespace apemode {

    struct BasicCamera {
        apemodem::mat4 ViewMatrix;
        apemodem::mat4 ProjMatrix;
    };
}