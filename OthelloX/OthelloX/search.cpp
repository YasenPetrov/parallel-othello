#include "stdafx.h"
#include "search.h"

#define MOVE_ORDER_SEARCH_DEPTH 1

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
	LOG_DEBUG("Negamax, MAX: " << maxTurn << " probe: " << isProbe << " depth: " << depth << " alpha: " << alpha << " beta: " << beta << " board: " << endl << printBoard(state, _parameters.black));

	if (!isProbe && _parameters.maxDepth - depth > _maxDepthReached) _maxDepthReached = _parameters.maxDepth - depth;

	if (depth == 0 || secondsElapsed() > _parameters.timeout)
	{
		if(!isProbe) LOG_DEBUG("DEPTH is " << depth << " or out of time");
		if(!isProbe) _boardsEvaluated++;
		_entireSpaceCovered = false;
		// Always evaluate for MAX!
		return evalBoard(state);
	}

	vector<gameMove> moves;

	if (isProbe)
	{
		moves = getMoves(state, maxTurn);
	}
	else
	{
		moves = treeSearch(state, MOVE_ORDER_SEARCH_DEPTH, true);
	}

	// If no moves - evaluate board
	if (moves.size() == 0)
	{
		LOG_DEBUG(i << " No more moves for MAX, board: " << endl << printBoard(state, _parameters.black));
		vector<gameMove> opponentMoves = getMoves(state, !maxTurn);

		if (opponentMoves.size() == 0)
		{
			LOG_DEBUG(i << " No more moves for MIN, board: " << endl << printBoard(state, _parameters.black));
			if(!isProbe) _boardsEvaluated++;
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
		if(!isProbe) _boardsEvaluated++;
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
				if(!isProbe) _boardsEvaluated++;
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

	short alpha = SHRT_MIN + 1;
	short beta = SHRT_MAX - 1;

	for (int i = 0; i < moves.size(); i++)
	{
		orderedMoves[i].move = moves[i];
		int val = -negaMax(applyMove(state, moves[i], true), maxDepth, -beta, -alpha, false, isProbe);
		orderedMoves[i].value = val;
		
		if (val > alpha) alpha = val;
	}

	sort(orderedMoves.begin(), orderedMoves.end(), [](const valueMove &left, const valueMove &right)
	{
		return left.value > right.value; // Sort in descending order
	});

	for (int i = 0; i < moves.size(); i++)
	{
		moves[i] = orderedMoves[i].move;
	}

	return moves;
}
