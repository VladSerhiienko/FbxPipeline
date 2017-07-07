#include <AppBase.h>

namespace apemode {
    class AppState;
    class AppContent;

    class App : public apemode::AppBase {
        friend AppState;
        friend AppContent;

        AppState *   appState;
        AppContent * appContent;
        float        totalSecs;

    public:
        App( );
        virtual ~App( );

    public:
        virtual bool Initialize( int Args, char* ppArgs[] ) override;
        virtual apemode::AppSurfaceBase* CreateAppSurface( ) override;

    public:
        bool         OnResized( );
        virtual void OnFrameMove( ) override;
        virtual void Update( float DeltaSecs, apemode::Input const& InputState ) override;
        virtual bool IsRunning( ) override;
    };
}