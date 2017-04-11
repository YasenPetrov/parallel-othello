#include "stdafx.h"
#include "board.h"

bool maybeFlipInDirection(board &state, int y, int x, signed char i, signed char j)
{
	int n = state.size(), m = state[0].size();

	if (y < 0 || y >= n || x < 0 || x >= m || state[y][x] == BRD_FREE) return false;
	if (state[y][x] == BRD_MAX_DISC) return true;

	// Flip current disc
	state[y][x] = BRD_MAX_DISC;
	// If it turns out we don't have to flip discs in this direction, flip it back
	bool haveToFlip = maybeFlipInDirection(state, y + i, x + j, i, j);
	if (!haveToFlip)
	{
		state[y][x] = BRD_MIN_DISC;
		return false;
	}

	// If we got to here, we're flippin'
	return true;
}

board applyMove(const board &state, const gameMove &move, bool max)
{
	board result = vector<vector<signed char>>(state);
	int N = state.size(), M = state[0].size();

	// The resulting board for a MIN move can be obtained by flipping all discs,
	// making a MAX move and then flipping them all again
	if (!max) result = flipAll(result);
	
	// Put disc down
	result[move.y][move.x] = BRD_MAX_DISC;

	// Check all 8 directions
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			// Do not check current square
			if (i == j && j == 0) continue;

			// If the next disc in the current direction is MIN, we want to check if we have to flip in this direction
			if (move.y + i >= 0 && move.y + i < M && move.x + j >= 0 && move.x + j < N && result[move.y + i][move.x + j] == BRD_MIN_DISC) maybeFlipInDirection(result, move.y + i, move.x + j, i, j);
		}
	}
	if (!max) return flipAll(result);
	return result;
}

board flipAll(const board &brd)
{
	// The resulting board - fill with 0s
	board result = board(brd.size(), row(brd[0].size(), 0));

	// If we see a disc, flip it
	for (int i = 0; i < brd.size(); i++)
	{
		for (int j = 0; j < brd[0].size(); j++)
		{
			result[i][j] = -brd[i][j];
		}
	}

	return result;
}

bool isValidMove(const board &state, int y, int x)
{
	int n = state.size(), m = state[0].size();

	// Move in every direction until we're seeing a MIN disc, if we see a MAX disc after we've seen at least one MIN disc, return true
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			// Do not check current square
			if (i == j && j == 0) continue;

			if (y + i >= 0 && y + i < n && x + j >= 0 && x + j < m && state[y + i][x + j] == BRD_MIN_DISC)
			{
				int p = y + 2 * i, q = x + 2 * j;
				while (p >= 0 && p < n && q >= 0 && q < m)
				{
					// If we see an empty spot, this is not a valid direction
					if (state[p][q] == BRD_FREE) break;
					if (state[p][q] == BRD_MAX_DISC) return true;
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
	vector<gameMove> moves;
	int n = brd.size(), m = brd[0].size();
	vector<vector<bool>> visited = vector<vector<bool>>(n, vector<bool>(m, false));

	// The legal moves for MIN are the same as the legal moves for MAX if all the discs were flipped
	if (!max) brd = flipAll(state);

	for (int i = 0; i < n; i++)
	{
		for (int j = 0; j < m; j++)
		{
			if (brd[i][j] == BRD_FREE)
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

	for (auto it_o = state.begin(); it_o != state.end(); it_o++)
	{
		for (auto it_i = it_o->begin(); it_i != it_o->end(); it_i++)
		{
			if (*(it_i) == BRD_MAX_DISC) maxD++;
			else if (*(it_i) == BRD_MIN_DISC) minD++;
		}
	}
}

string printBoard(const board & state, bool blackIsMax)
{
	// Board dimensions
	int N = state.size();
	int M = state[0].size();

	stringstream ss;

	// Print the top row(containing letters)
	ss << "  |";
	for (int i = 0; i < M; i++)
	{
		ss << " " << (char)('A' + i) << " |";
	}

	// Print the board, row by row
	ss << endl;
	for (int i = 0; i < N; i++)
	{
		string del = (i >= 9) ? "" : " ";
		ss << del << i + 1 << "|";

		for (int j = 0; j < M; j++)
		{
			char toPrint = '-';
			if (state[i][j] == 1)
			{
				if (blackIsMax) toPrint = 'B';
				else toPrint = 'W';
			}
			if (state[i][j] == -1)
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
	// Get board dimensions
	int N = state.size();
	int M = state[0].size();

	ofstream out(filename, std::ofstream::trunc); // Open and delete contents
	if (!out)
	{
		LOG_ERR("Error opening file for writing:");
		LOG_ERR(filename);
		return false;
	}

	// Write the size of the board to the file
	out << PRS_SIZE << " : " << M << ", " << N << endl;


	// Generate strings for the lists of positions of MAX and MIN discs
	stringstream maxSs, minSs;
	maxSs << "{ ";
	minSs << "{ ";
	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < M; j++)
		{
			if (state[i][j] == BRD_MAX_DISC) maxSs << (char)('a' + j) << i + 1 << ", ";
			else if (state[i][j] == BRD_MIN_DISC) minSs << (char)('a' + j) << i + 1 << ", ";
		}
	}

	string maxStr = maxSs.str().substr(0, maxSs.str().size() - 2); // Remove trailing comma and whitespace
	string minStr = minSs.str().substr(0, minSs.str().size() - 2); // Remove trailing comma and whitespace

	// Write MAX or MIN positions to black, depending on whether black is max
	if (blackIsMax)
	{
		out << PRS_BRD_BLACK << maxStr << " }" << endl;
		out << PRS_BRD_WHITE << minStr << " }" << endl;
	}
	else
	{
		out << PRS_BRD_BLACK << minStr << " }" << endl;
		out << PRS_BRD_WHITE << maxStr << " }" << endl;
	}

	return true;
}
