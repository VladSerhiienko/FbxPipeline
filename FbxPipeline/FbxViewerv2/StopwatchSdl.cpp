#include <Stopwatch.h>
#include <fbxvpch.h>

apemode::Stopwatch::Stopwatch( ) : mFrequency( 0 ), mStartPoint( 0 ), mFinishPoint( 0 ) {
}

void apemode::Stopwatch::Start( ) {
    Stop( );
    mFrequency  = SDL_GetPerformanceFrequency( );
    mStartPoint = SDL_GetPerformanceCounter( );
}

void apemode::Stopwatch::Stop( ) {
    mFrequency   = 0;
    mStartPoint  = 0;
    mFinishPoint = 0;
}

void apemode::Stopwatch::Update( ) {
    mPrevPoint   = mFinishPoint;
    mFinishPoint = SDL_GetPerformanceCounter( );
}

double apemode::Stopwatch::GetTotalElapsedSeconds( ) const {
    double DeltaTicks = static_cast< double >( mFinishPoint - mStartPoint );
    return ( DeltaTicks / static_cast< double >( mFrequency ) );
}

double apemode::Stopwatch::GetElapsedSeconds( ) const {
    double DeltaTicks = static_cast< double >( mFinishPoint - mPrevPoint );
    return ( DeltaTicks / static_cast< double >( mFrequency ) );
}
