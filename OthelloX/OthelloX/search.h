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

/*
Returns the next best moves in descending order(best to worst)
state - the current board state
maxDepth - maximmum depth for the search
isProbe - is this a shallow probe, used to estimate the next best moves
*/
vector<gameMove> treeSearch(const board &state, short maxDepth, bool isProbe);


int slaveSearch(const board &state, short maxDepth, bool isMaxTurn);