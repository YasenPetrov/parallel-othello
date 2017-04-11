#include "stdafx.h"
#include "search.h"

#define MOVE_ORDER_SEARCH_DEPTH 2

// The parameters for searching
evalParams _parameters;

// How many boards have we evaluated in total
unsigned int _boardsEvaluated = 0;
// The maximum depth we reached
int _maxDepthReached = 0;
// Have we looked through the entire search space
bool _entireSpaceCovered = true;


int negaMax(const board &state, short depth, short alpha, short beta, bool maxTurn, bool isProbe)
{
	LOG_DEBUG("Negamax, MAX: " << maxTurn << " probe: " << isProbe << " depth: " << depth << " alpha: " << alpha << " beta: " << beta << " board: " << endl << printBoard(state, maxTurn));

	if (!isProbe && _parameters.maxDepth - depth > _maxDepthReached) _maxDepthReached = _parameters.maxDepth = depth;

	if (depth == 0 || secondsElapsed() > _parameters.timeout)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		// Always evaluate for MAX!
		return evalBoard(state);
	}

	vector<gameMove> moves;

	if (isProbe)
	{
		moves = getMoves(state, maxTurn);
		LOG_DEBUG("MoveS count: " << moves.size());
	}
	else
	{
		moves = treeSearch(state, MOVE_ORDER_SEARCH_DEPTH, true);
	}

	// If no moves - evaluate board
	if (moves.size() == 0)
	{
		vector<gameMove> opponentMoves = getMoves(state, !maxTurn);

		if (opponentMoves.size() == 0)
		{
			_boardsEvaluated++;
			return evalBoard(state);
		}
		else
		{
			return -negaMax(state, depth, -beta, -alpha, !maxTurn, isProbe);
		}
	}

	// If we will run out of boards while evaluating the children of this node -
	// return an esimated utility value for this node
	if (_boardsEvaluated + moves.size() > _parameters.maxBoards)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		return evalBoard(state);
	}

	int maxValue = INT_MIN + 1;

	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		int value = -negaMax(applyMove(state, *(it), maxTurn), depth - 1, -beta, -alpha, !maxTurn, isProbe);

		if (value > maxValue) maxValue = value;

		if (maxValue > alpha)
		{
			alpha = maxValue;
			if (maxValue >= beta)
			{
				_boardsEvaluated++;
				return evalBoard(state);
			}
		}
	}

	return maxValue;
}


vector<gameMove> treeSearch(const board &state, short maxDepth, bool isProbe)
{

	// Get next moves for MAX
	vector<gameMove> moves = getMoves(state, true);

	// This will also hold the utility values for each possible move
	vector<valueMove> orderedMoves = vector<valueMove>(moves.size());

	if (!isProbe)
	{
		// Reset the number of evaluated boards
		_boardsEvaluated = 0;
		_maxDepthReached = 0;
		_entireSpaceCovered = true;
	}

	for (int i = 0; i < moves.size(); i++)
	{
		orderedMoves[i].move = moves[i];
		orderedMoves[i].value = -negaMax(state, maxDepth, (SHRT_MAX - 1), (SHRT_MIN + 1), false, isProbe);
	}

	sort(orderedMoves.begin(), orderedMoves.end(), [](const valueMove &left, const valueMove &right)
	{
		return left.value < right.value;
	});

	for (int i = 0; i < moves.size(); i++)
	{
		moves[i] = orderedMoves[i].move;
	}

	return moves;
}
