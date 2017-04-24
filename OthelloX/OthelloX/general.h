#pragma once
#ifndef GENERAL_H
#define GENERAL_H
#endif // !GENERAL_H

#include "stdafx.h"

// Constants for board entries
#define BRD_MIN_DISC -1
#define BRD_MAX_DISC 1
#define BRD_FREE 0

#define DEFAULT_LOAD_FACTOR 10
// Default wieghts for the utillity heuristic
#define DEFAULT_PARITY_WEIGHT 30
#define DEFAULT_STABILITY_WEIGHT 40
#define DEFAULT_MOBILITY_WEIGHT 30

// Holds the evaluation parameters as parsed from params file
struct evalParams
{
	int maxDepth;			// How many moves ahead can we look
	int maxBoards;			// How many boards can we evaluate
	bool black;			// Do we want the moves for black?
	float timeout; // Our solution is allowed to run for this time
	bool usePruning = true; // Should we do alpha-beta pruning
	bool useMoveOrdering = false; // Should we order moves for AB pruning
	int loadFactor = DEFAULT_LOAD_FACTOR; // How many processes per slave(avg) in the pool(minimum)?
	// Weights for the different scores when estimating utility
	int parityWeight;
	int mobilityWeight;
	int stabilityWeight;
};

extern evalParams _parameters;

// How many boards have we evaluated in total
extern int _boardsEvaluated;
// The maximum depth we reached
extern int _maxDepthReached;
// Have we looked through the entire search space
extern bool _entireSpaceCovered;
// How many nodes we pruned
extern int _nodesPruned;
// Estimate of the nodes at maxDepth that were pruned
extern int _estMaxDepthPruned;
// Board height
extern int _M;
// Board width
extern int _N;

// Represents a game move
// Next figure to go to board[x][y]
struct gameMove
{
	int x;
	int y;
};

// Represents the game board
// 0 - free square
// 1 -	square, occupied by MAX
// -1  - square occupied by MIN
typedef signed char piece;
// typedef vector<piece> row;
typedef vector<piece> board;