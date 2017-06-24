#pragma once

#include <MathfuInc.h>

namespace apemode {

    /* Adapts the input for the camera */
    struct CameraMouseInput {
        CameraMouseInput( )
            : Delta( 0.0f )
            , Scroll( 0.0f )
            , PrevPosition( std::numeric_limits< float >::max( ) )
            , PrevScroll( std::numeric_limits< float >::max( ) ) {
        }

        void Update( apemodem::vec3 _mxyz, apemodem::vec2 _widthHeight ) {
            if ( PrevScroll == std::numeric_limits< float >::max( ) ) {
                PrevScroll   = _mxyz.z;
                PrevPosition = _mxyz.xy( );
            }

            // Delta movement.
            Delta = ( _mxyz.xy( ) - PrevPosition ) / _widthHeight;
            PrevPosition = _mxyz.xy( );

            // Scroll.
            Scroll = _mxyz.z - PrevScroll;
            PrevScroll = _mxyz.z;
        }

        apemodem::vec2 Delta;
        apemodem::vec2 PrevPosition;
        float        Scroll;
        float        PrevScroll;
    };
}