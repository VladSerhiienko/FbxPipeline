#pragma once

#include <stdint.h>

namespace fbxv {
    class AppSurfaceSettings;

    /**
     * Contains handle to window and graphics context.
     */
    class AppSurfaceBase {
        struct PrivateContent;
        friend struct PrivateContent;
        PrivateContent* pContent;

    public:
        AppSurfaceBase( );
        virtual ~AppSurfaceBase( );

        /** Creates window and initializes its graphics context. */
        virtual bool Initialize( );

        virtual bool OnRecreateGraphicsContext( );

        /** Releases graphics context and destroyes window. */
        virtual void Finalize( );

        /** Must be called when new frame starts. */
        virtual void OnFrameMove( );

        /** Must be called when the current frame is done. */
        virtual void OnFrameDone( );

        virtual uint32_t GetWidth( ) const;
        virtual uint32_t GetHeight( ) const;
        virtual void*    GetWindowHandle( );
        virtual void*    GetGraphicsHandle( );
    };
}