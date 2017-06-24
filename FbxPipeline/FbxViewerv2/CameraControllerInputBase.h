#pragma once

#include <Input.h>
#include <MathfuInc.h>

namespace apemode {

    /* Adapts the input for the camera */
    struct CameraControllerInputBase {
        apemodem::vec3 DollyDelta;
        apemodem::vec2 OrbitDelta;

        virtual void Update( float deltaSecs, apemode::Input const& _input, apemodem::vec2 _widthHeight ) {
        }
    };
} // namespace apemode