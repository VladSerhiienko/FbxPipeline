#pragma once

#include <AppSurfaceBase.h>

namespace fbxv {
    class AppSurface : public fbxv::AppSurfaceBase {
        struct Context;
        friend Context;
        std::unique_ptr< Context > mContext;

    public:
        AppSurface( );
        virtual ~AppSurface( );

        virtual void OnFrameMove( ) override;
        virtual void OnFrameDone( ) override;
        virtual bool Initialize( ) override;
        virtual void Finalize( ) override;
    };
}