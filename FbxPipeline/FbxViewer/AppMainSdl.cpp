#include <fbxvpch.h>

#include <AppBase.h>
#include <SDL.h>
#include <SDL_main.h>

extern "C" fbxv::AppBase* CreateNesquikApp( );

extern "C" int SDL_main( int Args, char* ppArgs[] ) {
    fbxv::AppBase* pAppImpl = CreateNesquikApp( );

    if ( pAppImpl && pAppImpl->Initialize( Args, ppArgs ) ) {
        while ( pAppImpl->IsRunning( ) )
            pAppImpl->OnFrameMove( );
    }

    if ( pAppImpl )
        delete pAppImpl;
    return 0;
}
