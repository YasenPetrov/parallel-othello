#pragma once
#ifndef GENERAL_H
#define GENERAL_H
#endif // !GENERAL_H

#include "stdafx.h"

// Constants for board entries
#define BRD_MIN_DISC -1
#define BRD_MAX_DISC 1
#define BRD_FREE 0

// Holds the evaluation parameters as parsed from params file
struct evalParams
{
	int maxDepth;			// How many moves ahead can we look
	int maxBoards;			// How many boards can we evaluate

	// Weights for the different scores when estimating utility
	float parityWeight;
	float mobilityWeight;
	float stabilityWeight;

	bool useTranspositionTable; // Should we save the states in a hash map

	bool black;			// Do we want the moves for black?
	float timeout; // Our solution is allowed to run for this time
};

extern evalParams _parameters;

// How many boards have we evaluated in total
extern unsigned int _boardsEvaluated;
// The maximum depth we reached
extern int _maxDepthReached;
// Have we looked through the entire search space
extern bool _entireSpaceCovered;

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
typedef vector<signed char> row;
typedef vector<row> board;