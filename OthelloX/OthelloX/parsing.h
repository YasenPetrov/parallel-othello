#pragma once
#ifndef PARSING_H
#define PARSING_H
#endif // !PARSING_H


#include "stdafx.h"
#include "general.h"
#include "search.h"

#define PRS_SIZE "Size"
#define PRS_BRD_WHITE "White"
#define PRS_BRD_BLACK "Black"
#define PRS_TIMEOUT "Timeout"
#define PRS_COLOR "Color"

#define PRS_MAX_DEPTH "MaxDepth"
#define PRS_MAX_BOARDS "MaxBoards"
#define PRS_PARITY_WEIGHT "ParityWeight"
#define PRS_MOBILITY_WEIGHT "MobilityWeight"
#define PRS_STABILITY_WEIGHT "StabilityWeight"
#define PRS_COLOR_BLACK "Black"
#define PRS_COLOR_WHITE "White"
#define PRS_PRUNING "Prune"
#define PRS_MOVE_ORDERING "MoveOrdering"
#define PRS_LOAD_FACTOR "LoadFactor"

/*
Parse a position(ex: d4) for a board of size NxM
Save coords in xPos and yPos
*/
bool parsePosition(string pos, int &xPos, int &yPos, int N, int M);

// Parse the file, containing the initial board parameters and produce a board
// filename - the name of the params file
// black - Do we want the best move for black?
// state - a ref variable to save the parsed board in
// returns true if the parsing is successfull, false otherwise
// The file is formatted as follows:
/*
	Size: 8, 8			# x, y 
	White : { d4, e5 }
	Black : { d5, e4 } 
*/
bool parseBoardFile(const char* filename, board &state, evalParams &params);

// Parse the file, containing the evaluation parameters
// filename - path to the file, containing the params
// params - a ref variable to save the parsed struct to
// returns true if the parsing is successful, false otherwise
// The file is formatted as follows:
/*
	 MaxDepth: 10			# Maximum depth of following moves
	 MaxBoards : 10000000	# maximum number of boards to evaluate
	 ParityWeight : 0.3		# Parameter for the board evaluator
	 MobilityWeight : 0.4   # Parameter for the board evaluator
	 StabilityWeight : 0.3  # Parameter for the board evaluator
	 Color : Black			# return best moves for this color
	 Timeout : 10			# your solution is allowed to run for this time
*/
bool parseParamsFile(const char* filename, evalParams &params);