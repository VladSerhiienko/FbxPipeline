#pragma once

#include <MathfuInc.h>

namespace apemode {

    struct CameraControllerBase {
        apemodem::vec3 Target;
        apemodem::vec3 Position;

        virtual void Orbit( apemodem::vec2 _dxdy ) { }
        virtual void Dolly( apemodem::vec3 _dzxy ) { }
        virtual void Update( float _dt ) { }

        virtual apemodem::mat4 ViewMatrix( ) {
            return apemodem::mat4::LookAt( Target, Position, {0, 1, 0}, apemodem::kHandness );
        }

    };


}