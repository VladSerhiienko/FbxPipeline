#include <fbxvpch.h>

//
// SDL
//

#include <SDL.h>

//
// Game
//

#include <AppBase.h>
#include <IAppSurface.h>
#include <Input.h>
#include <Stopwatch.h>

class apemode::AppContent
{
public:
    IAppSurface * pSurface;
    Stopwatch        Stopwatch;
    Input            InputState;
    InputManager     InputManager;

    AppContent () : pSurface (nullptr)
    {
    }

    ~AppContent ()
    {
    }

    bool Initialize (apemode::IAppSurface * pInSurface)
    {
        SDL_LogVerbose (SDL_LOG_CATEGORY_APPLICATION, "AppContent/Initialize.");

        assert (pInSurface && "Surface is required.");
        pSurface = pInSurface;

        if (pSurface->Initialize (1280, 800, "FbxViewer v2"))
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

void apemode::AppBase::HandleWindowSizeChanged()
{
}

apemode::AppBase::AppBase () : pAppContent (nullptr)
{
}

apemode::AppBase::~AppBase ()
{
    if (pAppContent)
    {
        pAppContent->Finalize ();
        delete pAppContent;
    }
}

bool apemode::AppBase::Initialize (int Args, char * ppArgs[])
{
    pAppContent = new AppContent ();
    return pAppContent && pAppContent->Initialize (CreateAppSurface ());
}

apemode::IAppSurface * apemode::AppBase::CreateAppSurface ()
{
    return new IAppSurface ();
}

apemode::IAppSurface * apemode::AppBase::GetSurface()
{
    return pAppContent ? pAppContent->pSurface : nullptr;
}

void apemode::AppBase::OnFrameMove ()
{
    using namespace apemode;

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

void apemode::AppBase::Update (float /*DeltaSecs*/, Input const & /*InputState*/)
{
}

bool apemode::AppBase::IsRunning ()
{
    using namespace apemode;

    assert (pAppContent && "Not initialized.");
    return !pAppContent->InputState.bIsQuitRequested
        && !pAppContent->InputState.IsFirstPressed (kDigitalInput_BackButton)
        && !pAppContent->InputState.IsFirstPressed (kDigitalInput_KeyEscape);
}
