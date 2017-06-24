#pragma once

#include <CameraControllerInputBase.h>

namespace apemode {

    /* Adapts the input for the camera */
    struct MouseKeyboardCameraControllerInput : CameraControllerInputBase {
        apemodem::vec2 DeltaPosition;
        apemodem::vec2 PrevPosition;

        MouseKeyboardCameraControllerInput( ) : DeltaPosition( 0.0f )
                                              , PrevPosition( apemodem::kMaxFloat ) {
            DollyDelta = {0.0f, 0.0f, 0.0f};
            OrbitDelta = {0.0f, 0.0f};
        }

        void Update( float _dt, apemode::Input const& _input, apemodem::vec2 _widthHeight ) override {
            apemodem::vec3 mxyz = {_input.Analogs[ kAnalogInput_MouseX ],
                                   _input.Analogs[ kAnalogInput_MouseY ],
                                   _input.Analogs[ kAnalogInput_MouseScroll ]};

            if ( PrevPosition.x == apemodem::kMaxFloat ) {
                PrevPosition = mxyz.xy( );
            }

            DeltaPosition = ( mxyz.xy( ) - PrevPosition ) / _widthHeight;
            PrevPosition  = mxyz.xy( );

            // Delta orbit.
            OrbitDelta = _input.Buttons[ 0 ][ kDigitalInput_Mouse0 ] ? DeltaPosition : apemodem::vec2{0, 0};

            // Delta dolly.
            DollyDelta = {0, 0, 0};
            DollyDelta.y += ( _input.Buttons[ 0 ][ kDigitalInput_KeyE ] ) * _dt;
            DollyDelta.y -= ( _input.Buttons[ 0 ][ kDigitalInput_KeyQ ] ) * _dt;
            DollyDelta.x += ( _input.Buttons[ 0 ][ kDigitalInput_KeyD ] || _input.Buttons[ 0 ][ kDigitalInput_KeyRight ] ) * _dt;
            DollyDelta.x -= ( _input.Buttons[ 0 ][ kDigitalInput_KeyA ] || _input.Buttons[ 0 ][ kDigitalInput_KeyLeft ] ) * _dt;
            DollyDelta.z += ( _input.Buttons[ 0 ][ kDigitalInput_KeyW ] || _input.Buttons[ 0 ][ kDigitalInput_KeyUp ] ) * _dt;
            DollyDelta.z -= ( _input.Buttons[ 0 ][ kDigitalInput_KeyS ] || _input.Buttons[ 0 ][ kDigitalInput_KeyDown ] ) * _dt;
            DollyDelta.z += _input.Buttons[ 0 ][ kDigitalInput_Mouse1 ] ? ( DeltaPosition.x + DeltaPosition.y ) : 0;
        }
    };
} // namespace apemode