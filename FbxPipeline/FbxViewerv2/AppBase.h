#pragma once

namespace fbxv {
    class Input;
    class AppContent;
    class AppSurfaceBase;

    class AppBase {
        AppContent* pAppContent;

        void HandleWindowSizeChanged( );

    public:
        AppBase( );
        virtual ~AppBase( );

        virtual bool Initialize( int Args, char* ppArgs[] );
        virtual AppSurfaceBase* CreateAppSurface( );
        AppSurfaceBase*         GetSurface( );

        virtual void OnFrameMove( );
        virtual void Update( float DeltaSecs, Input const& InputState );
        virtual bool IsRunning( );
    };
}
