#include "stdafx.h"
#include "board.h"

#include <assert.h>

// Board height
int _M;
// Board width
int _N;

bool maybeFlipInDirection(board &state, int y, int x, signed char i, signed char j)
{
	if (y < 0 || y >= _M || x < 0 || x >= _N || boardAt(state, y, x) == BRD_FREE) return false;
	if (boardAt(state, y, x) == BRD_MAX_DISC) return true;

	// Flip current disc
	boardAssign(state, y, x, BRD_MAX_DISC);
	// If it turns out we don't have to flip discs in this direction, flip it back
	bool haveToFlip = maybeFlipInDirection(state, y + i, x + j, i, j);
	if (!haveToFlip)
	{
		boardAssign(state, y, x, BRD_MIN_DISC);
		return false;
	}

	// If we got to here, we're flippin'
	return true;
}

board applyMove(const board &state, const gameMove &move, bool max)
{
	assert(isValidMove(max ? state : flipAll(state), move.y, move.x));

	board result = board(state);

	// The resulting board for a MIN move can be obtained by flipping all discs,
	// making a MAX move and then flipping them all again
	if (!max) result = flipAll(result);
	
	// Put disc down
	boardAssign(result, move.y, move.x, BRD_MAX_DISC);

	// Check all 8 directions
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			// Do not check current square
			if (i == j && j == 0) continue;

			// If the next disc in the current direction is MIN, we want to check if we have to flip in this direction
			if (move.y + i >= 0 && move.y + i < _M && move.x + j >= 0 && move.x + j < _N && boardAt(result, move.y + i, move.x + j) == BRD_MIN_DISC) maybeFlipInDirection(result, move.y + i, move.x + j, i, j);
		}
	}
	if (!max) return flipAll(result);
	return result;
}

board flipAll(const board &brd)
{
	// The resulting board - fill with 0s
	board result = board(_N * _M, 0);

	// If we see a disc, flip it
	for (int i = 0; i < _M; i++)
	{
		for (int j = 0; j < _N; j++)
		{
			boardAssign(result, i, j, -boardAt(brd, i, j));
		}
	}

	return result;
}

bool isValidMove(const board &state, int y, int x)
{
	// We cannot put a disc on top of another
	if (boardAt(state, y, x) != BRD_FREE) return false;

	// Move in every direction until we're seeing a MIN disc, if we see a MAX disc after we've seen at least one MIN disc, return true
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			// Do not check current square
			if (i == j && j == 0) continue;

			if (y + i >= 0 && y + i < _M && x + j >= 0 && x + j < _N && boardAt(state, y + i, x + j) == BRD_MIN_DISC)
			{
				int p = y + 2 * i, q = x + 2 * j;
				while (p >= 0 && p < _M && q >= 0 && q < _N)
				{
					// If we see an empty spot, this is not a valid direction
					if (boardAt(state, p, q) == BRD_FREE) break;
					if (boardAt(state, p, q) == BRD_MAX_DISC) return true;
					p += i;
					q += j;
				}
			}
		}
	}

	return false;
}

vector<gameMove> getMoves(const board &state, bool max)
{
	board brd = state;
	vector<gameMove> moves(0);
	vector<vector<bool>> visited = vector<vector<bool>>(_M, vector<bool>(_N, false));

	// The legal moves for MIN are the same as the legal moves for MAX if all the discs were flipped
	if (!max) brd = flipAll(state);

	for (int i = 0; i < _M; i++)
	{
		for (int j = 0; j < _N; j++)
		{
			if (boardAt(brd, i, j) == BRD_FREE)
			{
				if (isValidMove(brd, i, j)) moves.push_back({ j, i });
			}
		}
	}

	return moves;
}

void discCount(const board & state, int & maxD, int & minD)
{
	maxD = 0;
	minD = 0;

	for(int i = 0; i < _M; i++)
	{
		for(int j = 0; j < _N; j++)
		{
			if(boardAt(state, i, j) == BRD_MAX_DISC) maxD++;
			else if(boardAt(state, i, j) == BRD_MIN_DISC) minD++;
		}
	}
}

piece boardAt(const board &state, int y, int x)
{
	return state[_N * y + x];
	// return state[y][x];
}

void boardAssign(board &state, int y, int x, piece value)
{
	state[_N * y + x] = value;
	// state[y][x] = value;
}

void makeEmptyBoard(board &state)
{
	state = board(_N * _M, BRD_FREE);
}

string printBoard(const board & state, bool blackIsMax)
{
	stringstream ss;

	// Print the top row(containing letters)
	ss << "  |";
	for (int i = 0; i < _N; i++)
	{
		ss << " " << (char)('A' + i) << " |";
	}

	// Print the board, row by row
	ss << endl;
	for (int i = 0; i < _M; i++)
	{
		string del = (i >= 9) ? "" : " ";
		ss << del << i + 1 << "|";

		for (int j = 0; j < _N; j++)
		{
			char toPrint = '-';
			if (boardAt(state, i, j) == 1)
			{
				if (blackIsMax) toPrint = 'B';
				else toPrint = 'W';
			}
			if (boardAt(state, i, j) == -1)
			{
				if (blackIsMax) toPrint = 'W';
				else toPrint = 'B';
			}

			ss << " " << toPrint << " |";
		}
		ss << endl;
	}

	return ss.str();
}

bool saveBoardToFile(const board &state, const char* filename, bool blackIsMax)
{
	ofstream out(filename, std::ofstream::trunc); // Open and delete contents
	if (!out)
	{
		LOG_ERR("Error opening file for writing:");
		LOG_ERR(filename);
		return false;
	}

	// Write the size of the board to the file
	out << PRS_SIZE << " : " << _N << ", " << _M << endl;


	// Generate strings for the lists of positions of MAX and MIN discs
	stringstream maxSs, minSs;
	maxSs << "{ ";
	minSs << "{ ";
	for (int i = 0; i < _M; i++)
	{
		for (int j = 0; j < _N; j++)
		{
			if (boardAt(state, i, j) == BRD_MAX_DISC) maxSs << (char)('a' + j) << i + 1 << ", ";
			else if (boardAt(state, i, j) == BRD_MIN_DISC) minSs << (char)('a' + j) << i + 1 << ", ";
		}
	}

	string maxStr = maxSs.str().substr(0, maxSs.str().size() - 2); // Remove trailing comma and whitespace
	string minStr = minSs.str().substr(0, minSs.str().size() - 2); // Remove trailing comma and whitespace

	// Write MAX or MIN positions to black, depending on whether black is max
	if (blackIsMax)
	{
		out << PRS_BRD_BLACK << " : " << maxStr << " }" << endl;
		out << PRS_BRD_WHITE << " : " << minStr << " }" << endl;
	}
	else
	{
		out << PRS_BRD_BLACK << " : " << minStr << " }" << endl;
		out << PRS_BRD_WHITE << " : " << maxStr << " }" << endl;
	}

	// Write timeout and color
	out << PRS_TIMEOUT << " : " << _parameters.timeout << endl;
	out << PRS_COLOR << " : " << (_parameters.black ? PRS_COLOR_BLACK : PRS_COLOR_WHITE) << endl;

	return true;
}
