#pragma once

#include <AppSurfaceBase.h>

namespace fbxv {
    class AppSurface : public fbxv::AppSurfaceBase {
        uint32_t mWidth = 0, mHeight = 0;
    public:
        virtual void OnFrameMove( ) override;
        virtual void OnFrameDone( ) override;
        virtual bool Initialize( ) override;
        virtual void Finalize( ) override;
    };
}