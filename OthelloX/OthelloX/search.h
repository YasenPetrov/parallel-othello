#pragma once
#ifndef SEARCH_H
#define SEARCH_H
#endif // !SEARCH_H

#include "stdafx.h"
#include "general.h"
#include "board.h"
#include "evaluate.h"
#include "timing.h"

// Default wieghts for the utillity heuristic
#define DEFAULT_PARITY_WEIGHT 0.3f
#define DEFAULT_STABILITY_WEIGHT 0.4f
#define DEFAULT_MOBILITY_WEIGHT 0.3f

struct valueMove
{
	int value;
	gameMove move;
};

// Search function, returns next bext move for MAX
gameMove search(board &state);

valueMove minValue(const board &state, int alpha, int beta, short depth);
valueMove maxValue(const board &state, int alpha, int beta, short depth);
