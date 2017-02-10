#include <fbxvpch.h>

//
// SDL
//

#include <SDL.h>

//
// Game
//

#include <AppBase.h>
#include <AppSurfaceBase.h>
#include <Input.h>
#include <Stopwatch.h>

class fbxv::AppContent
{
public:
    AppSurfaceBase * pSurface;
    Stopwatch        Stopwatch;
    Input            InputState;
    InputManager     InputManager;

    AppContent () : pSurface (nullptr)
    {
    }

    ~AppContent ()
    {
    }

    bool Initialize (fbxv::AppSurfaceBase * pInSurface)
    {
        SDL_LogVerbose (SDL_LOG_CATEGORY_APPLICATION, "AppContent/Initialize.");

        assert (pInSurface && "Surface is required.");
        pSurface = pInSurface;

        if (pSurface->Initialize ())
        {
            if (!InputManager.Initialize ())
                return false;

            Stopwatch.Start ();
            Stopwatch.Update ();
            return true;
        }

        return false;
    }

    void Finalize ()
    {
        SDL_LogVerbose (SDL_LOG_CATEGORY_APPLICATION, "AppContent/Finalize.");
        pSurface->Finalize ();
    }
};

//
// AppBase
//

void fbxv::AppBase::HandleWindowSizeChanged()
{
}

fbxv::AppBase::AppBase () : pAppContent (nullptr)
{
}

fbxv::AppBase::~AppBase ()
{
    if (pAppContent)
    {
        pAppContent->Finalize ();
        delete pAppContent;
    }
}

bool fbxv::AppBase::Initialize (int Args, char * ppArgs[])
{
    pAppContent = new AppContent ();
    return pAppContent && pAppContent->Initialize (CreateAppSurface ());
}

fbxv::AppSurfaceBase * fbxv::AppBase::CreateAppSurface ()
{
    return new AppSurfaceBase ();
}

fbxv::AppSurfaceBase * fbxv::AppBase::GetSurface()
{
    return pAppContent ? pAppContent->pSurface : nullptr;
}

void fbxv::AppBase::OnFrameMove ()
{
    using namespace fbxv;

    assert (pAppContent && "Not initialized.");

    auto & Surface      = *pAppContent->pSurface;
    auto & Stopwatch    =  pAppContent->Stopwatch;
    auto & InputState   =  pAppContent->InputState;
    auto & InputManager =  pAppContent->InputManager;

    Stopwatch.Update ();

    float const ElapsedSecs = static_cast<float> (Stopwatch.GetElapsedSeconds ());

    InputManager.Update (InputState, ElapsedSecs);

    Surface.OnFrameMove ();
    Update (ElapsedSecs, InputState);
    Surface.OnFrameDone ();
}

void fbxv::AppBase::Update (float /*DeltaSecs*/, Input const & /*InputState*/)
{
}

bool fbxv::AppBase::IsRunning ()
{
    using namespace fbxv;

    assert (pAppContent && "Not initialized.");
    return !pAppContent->InputState.bIsQuitRequested
        && !pAppContent->InputState.IsFirstPressed (kDigitalInput_BackButton)
        && !pAppContent->InputState.IsFirstPressed (kDigitalInput_KeyEscape);
}
