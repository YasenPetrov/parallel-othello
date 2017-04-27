// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#ifndef STDAFX_H
#define STDAFX_H
#endif // !STDAFX_H

#ifdef _WIN32
	#include "targetver.h"
#endif

// #define _DEBUG
// Debugging output
#ifdef _DEBUG
	#define LOG_DEBUG(x) std::cout << "DEBUG: " << x << std::endl
	#define LOG_WARNING(x) std::cerr << "WARNING: " << x << std::endl
#else
	#define LOG_DEBUG(x)
	#define LOG_WARNING(x)	
#endif //!_DEBUG
#define LOG_ERR(x) std::cerr << "ERROR: " << x << std::endl

using namespace std;



// TODO: reference additional headers your program requires here
#include <stdio.h>
#ifdef _WIN32
	#include <tchar.h>
#endif
#include <vector>
#include <climits>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <time.h>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>
#include <unordered_map>
#include <mpi.h>
#include <queue>
#include <assert.h>