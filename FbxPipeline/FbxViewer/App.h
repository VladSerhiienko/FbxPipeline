#include <AppBase.h>

namespace fbxv {
    class App : public fbxv::AppBase {
        friend struct AppContent;

        std::unique_ptr< AppContent > content;
        float                         totalSecs;

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