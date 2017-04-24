#include "stdafx.h"
#include "evaluate.h"

// Returns the number of "stable" MAX discs
// We call "stable" discs that cannot be flipped back
int stableDiscCount(const board &state)
{
	int n = _M, m = _N;
	vector<vector<bool>> visited = vector<vector<bool>>(n, vector<bool>(m, false));
	int result = 0;

	// Check corners. If there is a MAX disc in a corner, go along the sides and count the number
	// of contiguous MAX discs. Go to the next row. Count the number of contiguous MAX discs up to
	// the count on the border row. Go to the next row. Count up to the number in the last row.
	// Repeat until the count is 0 or we reach the number of contiguous discs on the border column.
	for (int N = 1; N >= 0; N--) // 1 - we're at a North corner
	{
		for (int W = 1; W >= 0; W--) // 1 - we're at a West corner
		{
			// Corner indices
			int i = n - n * N - (1 - N);
			int j = m - m * W - (1 - W);
			// Increments, +1 or -1, depending on the corner
			int i_inc = -1 + 2 * N;
			int j_inc = -1 + 2 * W;

			if (state[i][j] == BRD_MAX_DISC)
			{
				visited[i][j] = true;
				result++;

				// Count the discs on the border row
				int borderRowCount = 0;
				while (j < m && j >= 0 && state[i][j] == BRD_MAX_DISC && !visited[i][j])
				{
					visited[i][j] = true;
					borderRowCount++;
					result++;
					j += j_inc;
				}
				j = m - m * W - (1 - W); // Reset j to its initial value

				// Count the discs on the border column
				int borderColumnCount = 0;
				while (i < n && i >= 0&& state[i][j] == BRD_MAX_DISC && !visited[i][j])
				{
					visited[i][j] = true;
					borderColumnCount++;
					result++;
					i += i_inc;
				};
				i = n - n * N - (1 - N); // Reset i to its original value

				// Go to the next row. If we counted only the border element on the last row,
				// we won't find any new stable discs from now on. Abort
				int lastRowCount = borderRowCount;
				while (i + i_inc < borderColumnCount && lastRowCount > 1)
				{
					i += i_inc;
					// Set j to the column next to the border one
					j = m - m * W - (1 - W) + j_inc;

					// Count the disks on this row. Cannot be more than the last row count - 
					// any discs beyoud that won't be stable
					int thisRowCount = 1;
					while (thisRowCount < lastRowCount && state[i][j] == BRD_MAX_DISC && !visited[i][j])
					{
						visited[i][j] = true;
						thisRowCount++;
						result++;
						j += j_inc;
					}

					lastRowCount = thisRowCount;
				}
			}
		}
	}

	return result;
}

int evalBoard(const board & state)
{
	board flippedState = flipAll(state); // Used to calculate values for MIN

	int maxDiscs, minDiscs;
	discCount(state, maxDiscs, minDiscs); // Get the number of discs for each player
	// Score based on the disc count
	float parityScore = 100 * (maxDiscs - minDiscs) / ((float)(maxDiscs + minDiscs));

	int maxMoves = getMoves(state, true).size();
	int minMoves = getMoves(state, false).size();
	// Score based on the mobility(number of available moves) for each player
	float mobilityScore = 0;
	if (maxMoves + minMoves != 0)
	{
		mobilityScore = (maxMoves - minMoves) / ((float)(maxMoves + minMoves));
	}
	else // No more moves - this is a final state of the board
	{
		int result = maxDiscs - minDiscs;
		// Intermediate scores are in the -100-100 range.
		// We add 100 to the result to make sure final evaluations have more weight than
		// the approximations we make when making a cutoff.
		if (result > 0)
			return 30000 + result;
		else if (result < 0)
			return -30000 + result;
		else return 0;
	}

	int maxStableCount = stableDiscCount(state);
	int minStableCount = stableDiscCount(flippedState);
	// Score based on the number of stable discs
	float stabilityScore = 0;
	if (maxStableCount + minStableCount != 0)
		stabilityScore = 100 * (maxStableCount - minStableCount) / ((float)(maxStableCount + minStableCount));

	LOG_DEBUG("======== Evaluating board: \n" << printBoard(state, _parameters.black) << "Scores:"
					    << "Parity: " << parityScore << endl << "Max stable count: " << maxStableCount << ", MIN stable count: "
						<< minStableCount << endl << ", Stability: " << stabilityScore << "MAX moves: " << maxMoves 
						<< ", MIN moves: " << minMoves << endl << ", Mobility: " << mobilityScore << endl
						<< "Utility: " << _parameters.parityWeight * parityScore + _parameters.stabilityWeight + stabilityScore +
						_parameters.mobilityWeight * mobilityScore << endl);
	

	return _parameters.parityWeight * parityScore + _parameters.stabilityWeight + stabilityScore + _parameters.mobilityWeight * mobilityScore;
}

int evalBoardStatic(const board &state, int masterId, int slaveCount)
{
	int boardSize = _M * _N;
	int squaresPerSlave = slaveCount / boardSize;
}