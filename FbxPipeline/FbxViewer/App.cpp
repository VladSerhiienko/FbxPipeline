#include <fbxvpch.h>

#include <App.h>
#include <AppSurface.h>

using namespace fbxv;

App::App( ) {
}

App::~App( ) {
}

AppSurfaceBase* App::CreateAppSurface( ) {
    return new AppSurface( );
}

bool App::Initialize( int Args, char* ppArgs[] ) {
    if ( AppBase::Initialize( Args, ppArgs ) ) {
        TotalSecs = 0.0f;
        return true;
    }

    return false;
}

void App::OnFrameMove( ) {
    AppBase::OnFrameMove( );
}

void App::Update( float DeltaSecs, Input const& InputState ) {
    TotalSecs += DeltaSecs;
    // render
}

bool App::IsRunning( ) {
    return AppBase::IsRunning( );
}

extern "C" AppBase* CreateNesquikApp( ) {
    return new App( );
}