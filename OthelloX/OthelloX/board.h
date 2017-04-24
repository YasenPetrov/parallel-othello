#pragma once
#ifndef BOARD_H
#define BOARD_H
#endif // !BOARD_H

#include "general.h"
#include "parsing.h"

#include<fstream>

// Flip MIN discs, moving i squares along the y-axis and j along the x-axis
// If we reach the end or an empty square flip them back, if we see a MAX disc, leave them flipped
// Return true if we see a MAX disc
bool maybeFlipInDirection(board &state, int y, int x, signed char i, signed char j);

// Generate the next board given the move
board applyMove(const board &state, const gameMove &move, bool max);

// Return a board with all of the discs flipped
board flipAll(const board &brd);

// Check if MAX to (i, j) is a valid move
bool isValidMove(const board &state, int y, int x);

/*
 Generate a vector of possible moves
 state - the current board
 max - a flag, indicating if it's MAX's turn
*/
vector<gameMove> getMoves(const board &state, bool max);

// Store the number of MAX and min discs in maxD and minD, respectively
void discCount(const board &state, int &maxD, int &minD);

// Return the value of the board at <x, y>
piece boardAt(const board &state, int x, int y);

// Set state[y][x] to <value>
void boardAssign(board &state, int x, int y, piece value);

// Print the board on the terminal
string printBoard(const board &state, bool blackIsMax);

/*
Save the contents of a board to a file in the format:
Size: 8, 8			# x, y
White : { d4, e5 }
Black : { d5, e4 }
*/
bool saveBoardToFile(const board &state, const char* filename, bool blackIsMax);