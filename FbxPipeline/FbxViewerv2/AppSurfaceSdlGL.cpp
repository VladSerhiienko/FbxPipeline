#include <fbxvpch.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <AppSurfaceSdlGL.h>

class apemode::AppSurfaceSettings 
{
public:
    const char * pName;
    bool         bIsFullscreen;
    int          Width;
    int          Height;

public:
    AppSurfaceSettings ()
        : pName ("NesquikSdkSurface")

#if _WIN32
        , bIsFullscreen (false)
        , Width (1280)
        , Height (800)
#endif

#if __ANDROID__
        , bIsFullscreen (true)
        , Width (0)
        , Height (0)
#endif

    {
    }
};

struct apemode::AppSurfaceSdlGL::PrivateContent 
{
    SDL_Window *  pSdlWindow;
    SDL_GLContext pSdlGlContext;

    PrivateContent ()
        : pSdlWindow (nullptr)
        , pSdlGlContext (nullptr)
    {
    }

    ~PrivateContent ()
    {
        pSdlWindow    = nullptr;
        pSdlGlContext = nullptr;
    }
};

apemode::AppSurfaceSdlGL::AppSurfaceSdlGL ()
    : pContent (nullptr)
{
}

apemode::AppSurfaceSdlGL::~AppSurfaceSdlGL ()
{
    Finalize ();
}

bool apemode::AppSurfaceSdlGL::Initialize()
{
    SDL_Log ("apemode/AppSurfaceSdlGL/Initialize");

    pContent = new PrivateContent ();
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
#if defined(__ANDROID__)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

        AppSurfaceSettings defaultSettings;
        pContent->pSdlWindow = SDL_CreateWindow( defaultSettings.pName,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 SDL_WINDOWPOS_CENTERED,
                                                 defaultSettings.Width,
                                                 defaultSettings.Height,
                                                 SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL );

        SDL_Log("apemode/AppSurfaceSdlGL/Initialize: Created window.");

#ifdef _WIN32
        glewExperimental = GL_TRUE;
        if (GLEW_OK != glewInit()) {
            SDL_Log("apemode/AppSurfaceSdlGL/Initialize: Failed to initialize GLEW for Windows.");
            return false;
        }
#endif
        SDL_Log("apemode/AppSurfaceSdlGL/Initialize: Initialized GLEW.");

        if (pContent->pSdlGlContext)
        {
            SDL_Log("apemode/AppSurfaceSdlGL/OnRecreateGraphicsContext: Deleted OpenGL context.");
            SDL_GL_DeleteContext(pContent->pSdlGlContext);
            pContent->pSdlGlContext = nullptr;
        }

        SDL_Log("apemode/AppSurfaceSdlGL/OnRecreateGraphicsContext: Created OpenGL context.");
        pContent->pSdlGlContext = SDL_GL_CreateContext(pContent->pSdlWindow);

        assert(pContent->pSdlGlContext != nullptr && "Failed to initialize GL (SDL).");
        return pContent->pSdlGlContext != nullptr;
    }

    return false;
}

void apemode::AppSurfaceSdlGL::Finalize ()
{
    if (pContent->pSdlGlContext)
    {
        SDL_Log ("apemode/AppSurfaceSdlGL/Finalize: Deleting OpenGL context.");
        SDL_GL_DeleteContext (pContent->pSdlGlContext);
        pContent->pSdlGlContext = nullptr;
    }

    if (pContent->pSdlWindow)
    {
        SDL_Log ("apemode/AppSurfaceSdlGL/Finalize: Deleting window.");
        SDL_DestroyWindow (pContent->pSdlWindow);
        pContent->pSdlWindow = nullptr;
    }

    if (pContent)
    {
        SDL_Log ("apemode/AppSurfaceSdlGL/Finalize: Deleting content.");
        delete pContent;
        pContent = nullptr;
    }
}

uint32_t apemode::AppSurfaceSdlGL::GetWidth() const
{
    assert (pContent->pSdlWindow && "Not initialized.");

    int OutWidth;
    SDL_GetWindowSize (pContent->pSdlWindow, &OutWidth, nullptr);
    return static_cast<uint32_t> (OutWidth);
}

uint32_t apemode::AppSurfaceSdlGL::GetHeight() const
{
    assert (pContent->pSdlWindow && "Not initialized.");

    int OutHeight;
    SDL_GetWindowSize (pContent->pSdlWindow, nullptr, &OutHeight);
    return static_cast<uint32_t> (OutHeight);
}

void apemode::AppSurfaceSdlGL::OnFrameMove()
{
}

void apemode::AppSurfaceSdlGL::OnFrameDone ()
{
    SDL_GL_SwapWindow (pContent->pSdlWindow);
}

void * apemode::AppSurfaceSdlGL::GetWindowHandle ()
{
    return reinterpret_cast<void *> (pContent->pSdlWindow);
}

void * apemode::AppSurfaceSdlGL::GetGraphicsHandle ()
{
    return reinterpret_cast<void *> (pContent->pSdlGlContext);
}