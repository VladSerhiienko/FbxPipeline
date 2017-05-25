#pragma once

#include <AppSurfaceBase.h>

namespace apemode {
    class AppSurface : public apemode::AppSurfaceBase {
    public:
        AppSurface( );
        virtual ~AppSurface( );

        virtual void OnFrameMove( ) override;
        virtual void OnFrameDone( ) override;
        virtual bool Initialize( ) override;
        virtual void Finalize( ) override;
    };
}