#pragma once
#include <MathfuInc.h>

namespace apemode {

    struct CameraProjectionController {

        /* https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/ */
        const apemodem::mat4 kProjBias{{1.f, 0.0f, 0.0f, 0.0f},
                                       {0.f, -1.f, 0.0f, 0.0f}, /* Y -> -Y (flipped) */
                                       {0.f, 0.0f, 0.5f, 0.5f}, /* Z (-1, 1) -> (-0.5, 0.5) -> (0, 1)  */
                                       {0.f, 0.0f, 0.0f, 1.0f}};

        inline apemodem::mat4 ProjMatrix( float _fieldOfViewDegs, float _width, float _height, float _nearZ, float _farZ ) {
            const float fovRads = apemodem::DegreesToRadians( _fieldOfViewDegs );
            const float aspectWOverH = _width / _height;
            return kProjBias * apemodem::mat4::Perspective( fovRads, aspectWOverH, _nearZ, _farZ, apemodem::kHandness );
        }
    };
}