#include <AppBase.h>

namespace fbxv {
    class App : public fbxv::AppBase {
        float TotalSecs;

    public:
        App( );
        virtual ~App( );

    public:
        virtual bool Initialize( int Args, char* ppArgs[] ) override;
        virtual fbxv::AppSurfaceBase* CreateAppSurface( ) override;

    public:
        virtual void OnFrameMove( ) override;
        virtual void Update( float DeltaSecs, fbxv::Input const& InputState ) override;
        virtual bool IsRunning( ) override;
    };
}