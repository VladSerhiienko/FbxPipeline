#include <fbxvpch.h>
#include <Stopwatch.h>

fbxv::Stopwatch::Stopwatch()
    : mFrequency(0)
    , mStartPoint(0)
    , mFinishPoint(0)
{
}

void fbxv::Stopwatch::Start()
{
    Stop();
    mFrequency  = SDL_GetPerformanceFrequency();
    mStartPoint = SDL_GetPerformanceCounter();
}

void fbxv::Stopwatch::Stop()
{
    mFrequency   = 0;
    mStartPoint  = 0;
    mFinishPoint = 0;
}

void fbxv::Stopwatch::Update()
{
    mPrevPoint = mFinishPoint;
    mFinishPoint = SDL_GetPerformanceCounter();
}

double fbxv::Stopwatch::GetTotalElapsedSeconds() const
{
    double DeltaTicks = static_cast<double>(mFinishPoint - mStartPoint);
    return (DeltaTicks / static_cast<double>(mFrequency));
}

double fbxv::Stopwatch::GetElapsedSeconds() const
{
    double DeltaTicks = static_cast<double>(mFinishPoint - mPrevPoint);
    return (DeltaTicks / static_cast<double>(mFrequency));
}
