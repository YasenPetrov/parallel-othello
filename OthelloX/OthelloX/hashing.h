#pragma once
#include "stdafx.h"
#include "board.h"
#include "general.h"

struct hashEntry
{
	int maxDepth;			// The depth the result is coming from
	gameMove bestMove;		// The best move found
};

/*
	Initialise the random bitstring table
	N - board height
	M - board width
*/
void initZorbistTable(int N, int M);

/*
	Hash information for a given board state
*/
void hashState(const board &state, hashEntry info);

/*
	Get the entry for this state
	return false state not in hash
*/
bool getInfoForState(const board &state, hashEntry &info);