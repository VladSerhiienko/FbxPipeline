#include <AppBase.h>

namespace apemode {
    class App : public apemode::AppBase {
        friend class AppContent;

        AppContent * content;
        float        totalSecs;

    public:
        App( );
        virtual ~App( );

    public:
        virtual bool Initialize( int Args, char* ppArgs[] ) override;
        virtual apemode::IAppSurface* CreateAppSurface( ) override;

    public:
        bool         OnResized( );
        virtual void OnFrameMove( ) override;
        virtual void Update( float DeltaSecs, apemode::Input const& InputState ) override;
        virtual bool IsRunning( ) override;
    };
}