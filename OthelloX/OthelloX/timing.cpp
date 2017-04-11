#include "timing.h"
#include "stdafx.h"
#include <chrono>

using namespace chrono;
using namespace chrono_literals;

static high_resolution_clock::time_point _start;
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