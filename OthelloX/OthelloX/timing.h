#pragma once
#include "stdafx.h"


#ifdef __linux__
	using namespace chrono;
#elif _WIN32
	using namespace chrono_literals;
#endif

typedef high_resolution_clock::time_point timePoint;

// Start the timer(from zero)
void startTimer();

// Get the time since the timer started in seconds
float secondsElapsed();

// Get the time since the timer started in nanoseconds
long long nsElapsed();

// Get a time point
timePoint timeNow();

// Get the difference between two time points
long long nsBetween(timePoint start, timePoint end);
