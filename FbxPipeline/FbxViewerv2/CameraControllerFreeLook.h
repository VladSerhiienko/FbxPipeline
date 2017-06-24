#pragma once

#include <CameraControllerBase.h>

namespace apemode {
    
    struct FreeLookCameraController : CameraControllerBase {
        apemodem::vec3 TargetDst;
        apemodem::vec3 PositionDst;
        apemodem::vec2 OrbitCurr;
        apemodem::vec2 ZRange;

        FreeLookCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 5};
            Position    = {0, 0, 0};
            TargetDst   = {0, 0, 5};
            PositionDst = {0, 0, 0};
            OrbitCurr   = {0, 0};
        }

        void Orbit( apemodem::vec2 _dxdy ) override {
            OrbitCurr += _dxdy;
        }

        void Dolly( apemodem::vec3 _dxyz ) override {
            float toTargetLen;
            const apemodem::vec3 toTargetNorm = apemodem::NormalizedSafeAndLength( TargetDst - PositionDst, toTargetLen );
            const apemodem::vec3 right = apemodem::vec3::CrossProduct( {0.0f, 1.0f, 0.0f}, toTargetNorm ); /* Already normalized */
            const apemodem::vec3 up = apemodem::vec3::CrossProduct( toTargetNorm, right ); /* Already normalized */

            float deltaZ = toTargetLen * _dxyz.z;
            TargetDst += toTargetNorm * deltaZ;
            PositionDst += toTargetNorm * deltaZ;

            float deltaX = toTargetLen * _dxyz.x;
            TargetDst += right * deltaX;
            PositionDst += right * deltaX;

            float deltaY = toTargetLen * _dxyz.y;
            TargetDst += up * deltaY;
            PositionDst += up * deltaY;
        }

        void ConsumeOrbit( float _amount ) {
            float toPosLen;
            const apemodem::vec3 toPosNorm = apemodem::NormalizedSafeAndLength( Position - Target, toPosLen );
            apemodem::vec2 ll = apemodem::LatLongFromVec( toPosNorm );

            apemodem::vec2 consume = OrbitCurr * _amount;
            OrbitCurr -= consume;

            consume.y *= ( ll.y < 0.02 && consume.y < 0 ) || ( ll.y > 0.98 && consume.y > 0 ) ? 0 : -1;
            ll += consume;

            const apemodem::vec3 tmp  = apemodem::VecFromLatLong( ll );
            apemodem::vec3 diff = ( tmp - toPosNorm ) * toPosLen;

            Target += diff;
            TargetDst += diff;

            const apemodem::vec3 dstDiff = apemodem::NormalizedSafe( TargetDst - PositionDst );
            TargetDst = PositionDst + dstDiff * ( ZRange.y - ZRange.x ) * 0.1f;
        }

        void Update( float _dt ) override {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            Target   = apemodem::Lerp( Target, TargetDst, amount );
            Position = apemodem::Lerp( Position, PositionDst, amount );
        }

    };
}