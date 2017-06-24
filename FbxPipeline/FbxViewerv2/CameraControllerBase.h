#pragma once

#include <MathfuInc.h>

namespace apemode {

    struct CameraControllerBase {
        apemodem::vec3 Target;
        apemodem::vec3 Position;

        virtual void Orbit( apemodem::vec2 _dxdy ) {
        }

        virtual void Dolly( apemodem::vec3 _dzxy ) {
        }

        virtual void Update( float _dt ) {
        }

        virtual apemodem::mat4 ViewMatrix( ) {
            return apemodem::mat4::LookAt( Target, Position, {0, 1, 0}, apemodem::kHandness );
        }

        virtual apemodem::mat4 EnvViewMatrix( ) {
            const apemodem::vec3 forward = apemodem::NormalizedSafe( Target - Position );
            const apemodem::vec3 right   = apemodem::vec3::CrossProduct( {0.0f, 1.0f, 0.0f}, forward ).Normalized( );
            const apemodem::vec3 up      = apemodem::vec3::CrossProduct( forward, right ).Normalized( );

            return apemodem::mat4( apemodem::vec4{right, 0.0f},
                                   apemodem::vec4{up, 0.0f},
                                   apemodem::vec4{forward, 0.0f},
                                   apemodem::vec4{0.0f, 0.0f, 0.0f, 1.0f} );
        }
    };


}