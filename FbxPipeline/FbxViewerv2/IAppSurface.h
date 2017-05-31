#pragma once

#include <stdint.h>

namespace apemode {

    enum AppSurfaceImpl {
        kAppSurfaceImpl_Noop,
        kAppSurfaceImpl_SdlGL,
        kAppSurfaceImpl_SdlVk,
    };

    class IAppSurface {
    protected:
        AppSurfaceImpl Impl;
        
    public:
        AppSurfaceImpl GetImpl() { return Impl; }
        virtual void OnFrameMove() {}
        virtual void OnFrameDone() {}
        virtual bool Initialize() { return false; }
        virtual void Finalize() {}
        virtual uint32_t GetWidth() const { return 0; }
        virtual uint32_t GetHeight() const { return 0; }
        virtual void* GetWindowHandle() { return nullptr; }
        virtual void* GetGraphicsHandle() { return nullptr; }
    };
}