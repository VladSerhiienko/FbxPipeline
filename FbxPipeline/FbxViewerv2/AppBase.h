#pragma once

namespace apemode {
    class Input;
    class IAppSurface;

    class AppBase {
        friend class AppBaseContent;
        AppBaseContent* pAppContent;

        void HandleWindowSizeChanged( );

    public:
        AppBase( );
        virtual ~AppBase( );

        virtual bool Initialize( int Args, char* ppArgs[] );
        virtual IAppSurface* CreateAppSurface( );
        IAppSurface*         GetSurface( );

        virtual void OnFrameMove( );
        virtual void Update( float DeltaSecs, Input const& InputState );
        virtual bool IsRunning( );
    };
}
