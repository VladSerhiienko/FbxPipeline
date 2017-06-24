#pragma once

//
// ThirdParty
// Math
//

#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED
#endif

#include <math.h>

#include <mathfu/matrix.h>
#include <mathfu/vector.h>
#include <mathfu/glsl_mappings.h>

namespace apemodem {

    using namespace mathfu;

    // Left-handed system.
    const float kHandness = -1;

    static const float kPi               = 3.1415926535897932f;
    static const float kSmallNumber      = 1.e-8f;
    static const float kKindaSmallNumber = 1.e-4f;
    static const float kBigNumber        = 3.4e+38f;
    static const float kEulersNumber     = 2.71828182845904523536f;
    static const float kMaxFloat         = 3.402823466e+38f;
    static const float kInversePi        = 0.31830988618f;
    static const float kPiDiv2           = 1.57079632679f;
    static const float kSmallDelta       = 0.00001f;
    static const float k90               = kPiDiv2;
    static const float k180              = kPi;

    template < typename T >
    inline T NormalizedSafe( const T& _unsafeVec, float _safeEps = 1e-5f ) {
        const float length = _unsafeVec.Length( );
        const float invLength = 1.0f / ( length + _safeEps );

        return _unsafeVec * invLength;
    }

    template < typename T >
    inline T NormalizedSafeAndLength( const T& _unsafeVec, float& _outLength, float _safeEps = kSmallNumber ) {
        const float length = _unsafeVec.Length( );
        const float invLength = 1.0f / ( length + _safeEps );

        _outLength = length;
        return _unsafeVec * invLength;
    }

    inline mathfu::vec3 VecFromLatLong( mathfu::vec2 _uv ) {
        const float phi   = _uv.x * 2.0f * kPi;
        const float theta = _uv.y * kPi;

        const float st = sin( theta );
        const float sp = sin( phi );
        const float ct = cos( theta );
        const float cp = cos( phi );

        return {-st * sp, ct, -st * cp};
    }

    inline mathfu::vec2 LatLongFromVec( const mathfu::vec3& _vec ) {
        const float phi   = atan2f( _vec[ 0 ], _vec[ 2 ] );
        const float theta = acosf( _vec[ 1 ] );

        return {( kPi + phi ) * kInversePi * 0.5f, theta * kInversePi};
    }

    template < class T >
    inline auto RadiansToDegrees( T const& RadVal ) -> decltype( RadVal * ( 180.f / kPi ) ) {
        return RadVal * ( 180.f / kPi );
    }

    template < class T >
    inline auto DegreesToRadians( T const& DegVal ) -> decltype( DegVal * ( kPi / 180.f ) ) {
        return DegVal * ( kPi / 180.f );
    }

    inline bool IsNearlyEqual( float A, float B, float ErrorTolerance = kSmallNumber ) {
        return fabsf( A - B ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( mathfu::vec2 const A, mathfu::vec2 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( mathfu::vec3 const A, mathfu::vec3 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance &&
            fabsf( A.z - B.z ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( float Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( mathfu::vec2 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( mathfu::vec3 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance &&
            fabsf( Value.z ) <= ErrorTolerance;
    }

    inline mathfu::vec2 SinCos(float Value ) {
        const float q = ( kInversePi * 0.5f ) * Value;

        const float quotient = ( Value >= 0.0f )
            ? static_cast< float >( static_cast< int32_t >( q + 0.5f ) )
            : static_cast< float >( static_cast< int32_t >( q - 0.5f ) );

        float sign;
        float y = Value - ( 2.0f * kPi ) * quotient;
        if ( y > kPiDiv2 ) {
            y    = kPi - y;
            sign = -1.0f;
        } else if ( y < -kPiDiv2 ) {
            y    = -kPi - y;
            sign = -1.0f;
        } else {
            sign = +1.0f;
        }

        const float y2 = y * y;
        const float p = ( ( ( ( -2.6051615e-07f * y2 + 2.4760495e-05f ) * y2 - 0.0013888378f ) * y2 + 0.041666638f ) * y2 - 0.5f ) * y2 + 1.0f;
        const float n = ( ( ( ( ( -2.3889859e-08f * y2 + 2.7525562e-06f ) * y2 - 0.00019840874f ) * y2 + 0.0083333310f ) * y2 - 0.16666667f ) * y2 + 1.0f );

        return {n * y, sign * p};
    }

    inline mathfu::vec2 SafeNormal( mathfu::vec2 const &V ) {
        const float lengthSquared = V.LengthSquared( );
        return IsNearlyZero( lengthSquared ) ? mathfu::vec2( 0.0f, 0.0f ) : V / sqrtf( lengthSquared );
    }

    inline mathfu::vec3 SafeNormal( mathfu::vec3 const &V ) {
        const float lengthSquared = V.LengthSquared( );
        return IsNearlyZero( lengthSquared ) ? mathfu::vec3( 0.0f, 0.0f, 0.0f ) : V / sqrtf( lengthSquared );
    }

    inline mathfu::vec2 GetRotated( mathfu::vec2 const &V, const float &AngleDeg ) {
        const mathfu::vec2 sinCos = SinCos( DegreesToRadians( AngleDeg ) );
        return mathfu::vec2( sinCos.y * V.x - sinCos.x * V.y, sinCos.x * V.x + sinCos.y * V.y );
    }

    //! \brief platform-specific sign
    inline float Sign(float a)
    {
        return (a >= 0.0f) ? 1.0f : -1.0f;
    }

    inline mathfu::vec2 GetSignVector( mathfu::vec2 const &V ) {
        return mathfu::vec2{Sign( V.x ), Sign( V.y )};
    }

    inline mathfu::vec3 GetSignVector( mathfu::vec3 const &V ) {
        return mathfu::vec3{Sign( V.x ), Sign( V.y ), Sign( V.z )};
    }

    template < class T >
    inline T Square( const T A ) {
        return A * A;
    }

    inline float DistSquared( const mathfu::vec2& V1, const mathfu::vec2& V2 ) {
        return Square( V2.x - V1.x ) + Square( V2.y - V1.y );
    }

    inline float Distance( const mathfu::vec2& V1, const mathfu::vec2& V2 ) {
        return sqrtf( DistSquared( V1, V2 ) );
    }

    inline float GetAbsMax( mathfu::vec2 const V ) {
        return fmaxf( fabsf( V.x ), fabsf( V.y ) );
    }

    template < class T, class U >
    inline T CubicInterp( const T& P0, const T& T0, const T& P1, const T& T1, const U& A ) {
        const U A2 = A * A;
        const U A3 = A2 * A;

        return ( T )( ( ( 2 * A3 ) - ( 3 * A2 ) + 1 ) * P0 ) +
               ( ( A3 - ( 2 * A2 ) + A ) * T0 ) + ( ( A3 - A2 ) * T1 ) +
               ( ( ( -2 * A3 ) + ( 3 * A2 ) ) * P1 );
    }

    template < class T >
    inline T Map( const T& V0, const T& S0, const T& E0, const T& S1, const T& E1 ) {
        return ( V0 - S0 ) / ( E0 - S0 ) * ( E1 - S1 ) + S1;
    }

    template < typename T >
    // http://www.antigrain.com/research/adaptive_bezier/index.html
    inline T BezierInterp( const T& P0, const T& P1, const T& P2, const T& P3, const float A ) {
        const float AA = A * A;
        const float B  = 1.0f - A;
        const float BB = B * B;
        return P0 * B * BB + P1 * 3.0f * A * BB + P2 * 3.0f * AA * B + P3 * AA * A;
    }

    inline float Atan2( float Y, float X ) {
        const float absX       = fabsf( X );
        const float absY       = fabsf( Y );
        const bool  yAbsBigger = ( absY > absX );
        float       t0         = yAbsBigger ? absY : absX;
        float       t1         = yAbsBigger ? absX : absY;

        if ( t0 == 0.f )
            return 0.f;

        float t3 = t1 / t0;
        float t4 = t3 * t3;

        static const float c[ 7 ] = {+7.2128853633444123e-03f,
                                     -3.5059680836411644e-02f,
                                     +8.1675882859940430e-02f,
                                     -1.3374657325451267e-01f,
                                     +1.9856563505717162e-01f,
                                     -3.3324998579202170e-01f,
                                     +1.0f};

        t0 = c[ 0 ];
        t0 = t0 * t4 + c[ 1 ];
        t0 = t0 * t4 + c[ 2 ];
        t0 = t0 * t4 + c[ 3 ];
        t0 = t0 * t4 + c[ 4 ];
        t0 = t0 * t4 + c[ 5 ];
        t0 = t0 * t4 + c[ 6 ];
        t3 = t0 * t3;

        t3 = yAbsBigger ? ( 0.5f * apemodem::kPi ) - t3 : t3;
        t3 = ( X < 0.0f ) ? apemodem::kPi - t3 : t3;
        t3 = ( Y < 0.0f ) ? -t3 : t3;

        return t3;
    }

    inline float CalcHeading( mathfu::vec2 v ) {
        return Atan2( v.x, v.y );
    }
}