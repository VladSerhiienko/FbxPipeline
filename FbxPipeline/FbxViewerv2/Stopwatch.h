#pragma once

#include <stdint.h>

namespace apemode
{
    class Stopwatch
    {
    public:
        Stopwatch();
        void Start();
        void Stop();
        void Update();
        double GetElapsedSeconds() const;
        double GetTotalElapsedSeconds() const;

    public:
        uint64_t mFrequency;
        uint64_t mStartPoint;
        uint64_t mPrevPoint;
        uint64_t mFinishPoint;
    };
}
