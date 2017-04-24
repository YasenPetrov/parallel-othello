#include "timing.h"
#include "stdafx.h"
#include <chrono>

static timePoint _start;
static bool _running = false;

void startTimer()
{
	_start = high_resolution_clock::now();
	_running = true;
}

float secondsElapsed()
{
	if (!_running)
	{
		LOG_ERR("Trying to get time when clock not running");
		return -1;
	}
	
	auto time = high_resolution_clock::now();
	return duration_cast<duration<float>>(time - _start).count();
}

long long nsElapsed()
{
	if (!_running)
	{
		LOG_ERR("Trying to get time when clock not running");
		return -1;
	}
	auto time = high_resolution_clock::now();
	long long elapsed = duration_cast<nanoseconds>(time - _start).count();
	return elapsed;
}

timePoint timeNow()
{
	return high_resolution_clock::now();
}

long long nsBetween(timePoint start, timePoint end)
{
	long long elapsed = duration_cast<nanoseconds>(end - start).count();
	return elapsed;
}