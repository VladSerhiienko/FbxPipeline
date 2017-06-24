#pragma once

#include <CameraControllerBase.h>

namespace apemode {
    
    struct ModelViewCameraController : CameraControllerBase {
        apemodem::vec3 TargetDst;
        apemodem::vec3 PositionDst;
        apemodem::vec2 OrbitCurr;
        apemodem::vec2 ZRange;

        ModelViewCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 0};
            Position    = {0, 0, -5};
            TargetDst   = {0, 0, 0};
            PositionDst = {0, 0, -5};
            OrbitCurr   = {0, 0};
        }

        void Orbit( apemodem::vec2 _dxdy ) override {
            OrbitCurr += _dxdy;
        }

        void Dolly( apemodem::vec3 _dzxy ) override {
            float                toTargetLen;
            const apemodem::vec3 toTargetNorm = apemodem::NormalizedSafeAndLength( TargetDst - PositionDst, toTargetLen );

            float delta  = toTargetLen * _dzxy.z;
            float newLen = toTargetLen + delta;
            if ( ( ZRange.x < newLen || _dzxy.z < 0.0f ) && ( newLen < ZRange.y || _dzxy.z > 0.0f ) ) {
                PositionDst += toTargetNorm * delta;
            }
        }

        void ConsumeOrbit( float _amount ) {
            apemodem::vec2 consume = OrbitCurr * _amount;
            OrbitCurr -= consume;

            float              toPosLen;
            const apemodem::vec3 toPosNorm = apemodem::NormalizedSafeAndLength( Position - Target, toPosLen );

            apemodem::vec2 ll = apemodem::LatLongFromVec( toPosNorm ) + consume * apemodem::vec2( 1, -1 );
            ll.y            = apemodem::Clamp( ll.y, 0.02f, 0.98f );

            const apemodem::vec3 tmp  = apemodem::VecFromLatLong( ll );
            const apemodem::vec3 diff = ( tmp - toPosNorm ) * toPosLen;

            Position += diff;
            PositionDst += diff;
        }

        void Update( float _dt ) override {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            Target   = apemodem::Lerp( Target, TargetDst, amount );
            Position = apemodem::Lerp( Position, PositionDst, amount );
        }

    };
}