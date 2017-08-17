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

#if 1
            const apemodem::vec3 forward = apemodem::NormalizedSafe( Target - Position );
            const apemodem::vec3 right   = apemodem::vec3::CrossProduct( {0.0f, 1.0f, 0.0f}, forward ).Normalized( );
            const apemodem::vec3 up      = apemodem::vec3::CrossProduct( forward, right ).Normalized( );

            return apemodem::mat4(  apemodem::vec4{right, 0.0f},
                                    apemodem::vec4{up, 0.0f},
                                    apemodem::vec4{forward, 0.0f},
                                    apemodem::vec4{0.0f, 0.0f, 0.0f, 1.0f} );

#else

            apemodem::vec3 axes[ 4 ];
            apemodem::vec3 up{0, 1, 0};

            // Notice that y-axis is always the same regardless of handedness.
            axes[ 2 ] = ( Target - Position ).Normalized( ); 
            axes[ 0 ] = apemodem::vec3::CrossProduct( up, axes[ 2 ] ).Normalized( );
            axes[ 1 ] = apemodem::vec3::CrossProduct( axes[ 2 ], axes[ 0 ] ); 
            axes[ 3 ] = apemodem::vec3( apemodem::kHandness * apemodem::vec3::DotProduct( axes[ 0 ], Position ),
                                        -apemodem::vec3::DotProduct( axes[ 1 ], Position ),
                                        apemodem::kHandness * apemodem::vec3::DotProduct( axes[ 2 ], Position ) );

            // Default calculation is left-handed (i.e. handedness=-1).
            // Negate x and z axes for right-handed (i.e. handedness=+1) case.
            const float neg = -apemodem::kHandness;
            axes[ 0 ] *= neg;
            axes[ 2 ] *= neg;

#if 0
            const apemodem::vec4 column0( axes[ 0 ][ 0 ], axes[ 1 ][ 0 ], axes[ 2 ][ 0 ], 0 );
            const apemodem::vec4 column1( axes[ 0 ][ 1 ], axes[ 1 ][ 1 ], axes[ 2 ][ 1 ], 0 );
            const apemodem::vec4 column2( axes[ 0 ][ 2 ], axes[ 1 ][ 2 ], axes[ 2 ][ 2 ], 0 );
            const apemodem::vec4 column3( axes[ 3 ], 1 );
#else
            const apemodem::vec4 column0( axes[ 0 ], 0 );
            const apemodem::vec4 column1( axes[ 1 ], 0 );
            const apemodem::vec4 column2( axes[ 2 ], 0 );
            const apemodem::vec4 column3( axes[ 3 ], 1 );
#endif

            return apemodem::mat4( column0, column1, column2, column3 );
#endif
        }
    };


}