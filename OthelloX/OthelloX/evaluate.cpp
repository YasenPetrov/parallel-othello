#include "stdafx.h"
#include "evaluate.h"
#include "processes.h"

board _squareWeights;

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

			if (boardAt(state, i, j) == BRD_MAX_DISC)
			{
				visited[i][j] = true;
				result++;

				// Count the discs on the border row
				int borderRowCount = 0;
				while (j < m && j >= 0 && boardAt(state, i, j) == BRD_MAX_DISC && !visited[i][j])
				{
					visited[i][j] = true;
					borderRowCount++;
					result++;
					j += j_inc;
				}
				j = m - m * W - (1 - W); // Reset j to its initial value

				// Count the discs on the border column
				int borderColumnCount = 0;
				while (i < n && i >= 0 && boardAt(state, i, j) == BRD_MAX_DISC && !visited[i][j])
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
					while (thisRowCount < lastRowCount && boardAt(state, i, j) == BRD_MAX_DISC && !visited[i][j])
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



int evalBoardDynamic(const board &state, bool isFinal, int maxMoves, int minMoves)
{
	board flippedState = flipAll(state); // Used to calculate values for MIN

	int maxDiscs, minDiscs;
	discCount(state, maxDiscs, minDiscs); // Get the number of discs for each player
	// Score based on the disc count
	float parityScore = 100 * (maxDiscs - minDiscs) / ((float)(maxDiscs + minDiscs));

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

int evalBoardStatic(const board &state, bool isFinal)
{
	int score = 0;
	for(int i = 0; i < _M; i++)
	{
		for(int j = 0; j < _N; j++)
			score += boardAt(state, i, j) * boardAt(_squareWeights, i, j);
	}
	return score;
}

int evalBoardStatic(const board &state, int masterId, int slaveCount, bool isFinal)
{
	int score = 0;
	if(slaveCount < 2)
	{
		return evalBoardStatic(state, isFinal);
	}
	else // We have more than one slave
	{
		if(_currentProcId == MASTER_ID)
		{
			// Tell everyone more work is on the way
			int8_t hasWork = true;
			timePoint before = timeNow(), after;
			MPI_Bcast(&hasWork, 1, MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
			after = timeNow();
			_parallelEvalCommTime += nsBetween(before, after);

			// Tell them if it's a final evaluation
			before = timeNow();
			MPI_Bcast(&isFinal, 1, MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
			after = timeNow();
			_parallelEvalCommTime += nsBetween(before, after);

			// Scatter the data
			before = timeNow();	
			MPI_Scatterv(&state.front(), _sendCounts, _displacements, MPI_BYTE, NULL, 0, MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
			after = timeNow();
			_parallelEvalCommTime += nsBetween(before, after);

			// Do my own share of the work
			int masterSubScore = 0;
			for(int i = _displacements[MASTER_ID]; i < _displacements[MASTER_ID] + _sendCounts[MASTER_ID]; i++)
			{
				masterSubScore += state[i] * _squareWeights[i];
			}

			// // Gather results
			// before = timeNow();
			// MPI_Gather(&masterSubScore, 1, MPI_INT, _subScores, 1, MPI_INT, MASTER_ID, MPI_COMM_WORLD);
			// after = timeNow();
			// _parallelEvalCommTime += nsBetween(before, after);

			// // Aggregate
			// for(int procId = 0; procId < _slaveCount + 1; procId++)
			// {
			// 	score += _subScores[procId];
			// }

			before = timeNow();
			MPI_Reduce(&masterSubScore, &score, 1, MPI_INT, MPI_SUM, MASTER_ID, MPI_COMM_WORLD);
			after = timeNow();
			_parallelEvalCommTime += nsBetween(before, after);
		}
		else // Slave?!
		{
			LOG_ERR("SLAVE process in static evaluation for MASTER");
		}
	}
	return score;
}

void fillWeightsMatrix(board &matrix)
{
	for(int i = 0; i < _M; i++)
	{
		for(int j = 0; j < _N; j++)
		{
			// Corners
			if((i == 0 && j == 0) || (i == 0 && j == _N - 1) || (i == _M - 1 && j == 0) || (i == _M - 1 && j == _N - 1))
				boardAssign(matrix, i, j, _parameters.cornerWeight);
			// C-squares
			else if((i == 0 && j == 1) || (i == 0 && j == _N - 2) || (i == 1 && j == 0) || (i == 1 && j == _N - 1) || 
					(i == _M - 2 && j == 0) || (i == _M - 2 && j == _N - 1) || (i == _M - 1 && j == 1) || (i == _M - 1 && j == _N - 2))
				boardAssign(matrix, i, j, _parameters.cSquareWeight);
			// X-squares
			else if((i == 1 && j == 1) || (i == 1 && j == _N - 2) || (i == _M - 2 && j == 1) || (i == _M - 2 && j == _N - 2))
				boardAssign(matrix, i, j, _parameters.xSquareWieght);
			// Edges
			else if((i == 0) || (i == _M - 1) || (j == 0) || (j == _N - 1))
				boardAssign(matrix, i, j, _parameters.edgeSquareWeight);
			// Inner squares
			else if((i > 1 && i < _M - 2 && j > 1 && j < _N - 2))
				boardAssign(matrix, i, j , _parameters.innerSquareWeight);
			else
				boardAssign(matrix, i, j, 0);
		}
	}
}

int evalBoard(const board &state, bool isFinal, int maxMoves, int minMoves)
{
	int result;
	timePoint before = timeNow();
	timePoint after;
	if(_parameters.useStaticEvaluation)
	{
		if(_parameters.parallelSearch)
			result = evalBoardStatic(state, isFinal);
		else
			result = evalBoardStatic(state, MASTER_ID, _slaveCount, isFinal);
	}
	else
	{
		result = evalBoardDynamic(state, isFinal, maxMoves, minMoves);
	}
	after = timeNow();
	_totalEvaluationTime += nsBetween(before, after);
	return result;
}