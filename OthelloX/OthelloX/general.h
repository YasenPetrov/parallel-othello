#pragma once
#ifndef GENERAL_H
#define GENERAL_H
#endif // !GENERAL_H

#include "stdafx.h"

// A billion
#define BLN_FLOAT 1000000000.0f
#define BLN_DOUBLE 1000000000.0

// Constants for board entries
#define BRD_MIN_DISC -1
#define BRD_MAX_DISC 1
#define BRD_FREE 0

#define AVG_BRANCH_FACTOR 7

#define DEFAULT_LOAD_FACTOR 10
// Default wieghts for the utillity heuristic
#define DEFAULT_PARITY_WEIGHT 30
#define DEFAULT_STABILITY_WEIGHT 40
#define DEFAULT_MOBILITY_WEIGHT 30

// Represents the game board
// 0 - free square
// 1 -	square, occupied by MAX
// -1  - square occupied by MIN
typedef int8_t piece;
// typedef vector<piece> row;
typedef vector<piece> board;

extern Mode _runMode;

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
	bool parallelSearch = true;
	// Static evaluation
	bool useStaticEvaluation = false;
	piece cornerWeight = 10;
	piece xSquareWieght = -8;
	piece cSquareWeight = - 4;
	piece edgeSquareWeight = 2;
	piece innerSquareWeight = 1;
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
// Weights for static evaluation
extern board _squareWeights;
// For parallel evaluation:
extern int _squaresPerProc;
extern int _remainderSquares;
extern board _sharedBoard;
extern int *_sendCounts;
extern int *_displacements;
extern int *_subScores;
// For parallel processes
extern int _slaveCount;
extern int _currentProcId;
// For timing
extern long long _totalEvaluationTime;
extern long long _parallelEvalCommTime;
extern long long _parallelEvalCompTime;

// Represents a game move
// Next figure to go to board[x][y]
struct gameMove
{
	int x;
	int y;
};
