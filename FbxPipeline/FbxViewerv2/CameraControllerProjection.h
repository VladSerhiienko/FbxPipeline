#pragma once
#include <MathfuInc.h>

namespace apemode {

    struct CameraProjectionController {
        inline apemodem::mat4 ProjMatrix( float _fieldOfViewDegs, float _width, float _height, float _nearZ, float _farZ ) {
            /* https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/ */
            const apemodem::mat4 kProjBias{{1.f, 0.0f, 0.0f, 0.0f},
                                         {0.f, -1.f, 0.0f, 0.0f}, /* Flip y */
                                         {0.f, 0.0f, 0.5f, 0.5f}, /* Z (-1, 1) to (0, 1) */
                                         {0.f, 0.0f, 0.0f, 1.0f}};

            const float fovRads      = apemodem::DegreesToRadians( _fieldOfViewDegs );
            const float aspectWOverH = _width / _height;
            return kProjBias * apemodem::mat4::Perspective( fovRads, aspectWOverH, _nearZ, _farZ, apemodem::kHandness );
        }
    };
}