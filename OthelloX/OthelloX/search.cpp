#include "stdafx.h"
#include "search.h"

// The parameters for searching
evalParams _parameters;

// How many boards have we evaluated in total
unsigned int _boardsEvaluated = 0;
// The maximum depth we reached
int _maxDepthReached = 0;
// Have we looked through the entire search space
bool _entireSpaceCovered;


valueMove maxValue(const board &state, int alpha, int beta, short depth)
{
	if (depth > _maxDepthReached) _maxDepthReached = depth;

	// If we've reached the max depth or we reach the time cutoff, return an estimated utility value
	if (depth == _parameters.maxDepth || secondsElapsed() > _parameters.timeout)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		return { evalBoard(state, true), {-1, -1} };
	}

	// Get next moves
	vector<gameMove> moves = getMoves(state, true);
	/*LOG_DEBUG("maxValue - board:\n" << printBoard(state, true) << "\n And moves:");
#ifdef _DEBUG
	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		LOG_DEBUG((char)(it->y + 'A') << it->x + 1);
	}

#endif // _DEBUG*/



	// If we will run out of boards while evaluating the children of this node -
	// return an esimated utility value for this node
	if (_boardsEvaluated + moves.size() > _parameters.maxBoards)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		return { evalBoard(state, true), {-1, -1} };
	}

	// If no moves - check if opposing player has moves
	// If so, run minValue. Else, return a final evaluation
	if (moves.size() == 0)
	{
		if (getMoves(state, false).size() == 0)
		{
			LOG_DEBUG("Out of moves for both players, board:");
			LOG_DEBUG(printBoard(state, true));

			_boardsEvaluated++;
			return { evalBoard(state, true), {-1, -1} };
		}
		else
		{
			LOG_DEBUG("Out of moves for MAX(printed in black), board:");
			LOG_DEBUG(printBoard(state, true));
			return minValue(state, alpha, beta, depth);
		}
	}

	// Else, get the best possible value for a next move
	int value = INT_MIN;
	gameMove nextMove = { -1, -1 };

	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		// Recursive call
		valueMove next = minValue(applyMove(state, *(it), true), alpha, beta, depth + 1);
		
		if (next.value > value)
		{
			value = next.value;
			nextMove = *(it);
		}
		// Prune node if value bigger than beta
		if (value > beta)
			return { value, *(it) };

		// Update alpha
		alpha = max(value, alpha);
	}

	return { value, nextMove };
}

valueMove minValue(const board &state, int alpha, int beta, short depth)
{
	if (depth > _maxDepthReached) _maxDepthReached = depth;

	// If we've reached the max depth or we reach the time cutoff, return an estimated utility value
	if (depth == _parameters.maxDepth || secondsElapsed() > _parameters.timeout)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		return { evalBoard(state, false),{ -1, -1 } };
	}

	// Get next moves
	vector<gameMove> moves = getMoves(state, false);
	
/*	LOG_DEBUG("minValue - board:\n" << printBoard(state, true) << "And moves:");
#ifdef _DEBUG
	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		LOG_DEBUG((char)(it->y + 'A') << it->x + 1);
	}

#endif // _DEBUG */

	// If we will run out of boards while evaluating the children of this node -
	// return an esimated utility value for this node
	if (_boardsEvaluated + moves.size() > _parameters.maxBoards)
	{
		_boardsEvaluated++;
		_entireSpaceCovered = false;
		return { evalBoard(state, false),{ -1, -1 } };
	}

	// If no moves - check if opposing player has moves
	// If so, run minValue. Else, return a final evaluation
	if (moves.size() == 0)
	{
		if (getMoves(state, true).size() == 0)
		{
			_boardsEvaluated++;
			return { evalBoard(state, false),{ -1, -1 } };
		}
		else
		{
			return maxValue(state, alpha, beta, depth);
		}
	}

	// Else, get the best possible value for a next move
	int value = INT_MAX;
	gameMove nextMove = { -1, -1 };

	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		// Recursive call
		valueMove next = maxValue(applyMove(state, *(it), false), alpha, beta, depth + 1);

		if (next.value < value)
		{
			value = next.value;
			nextMove = *(it);
		}

		// Prune node if value smaller than alpha
		if (value < alpha)
			return { value, *(it) };

		// Update beta
		beta = min(value, beta);
	}

	return { value, nextMove };
}

// Returns the next best move given the current state(board)
gameMove search(board &state)
{
	// The MINIMAX algorithm will return this
	valueMove next;

	// Get next moves for MAX
	vector<gameMove> moves = getMoves(state, true);
	// If no moves - return an invalid move
	if (moves.size() == 0) // No moves for MAX
	{
		LOG_WARNING("No moves for MAX(printed black) on board: \n" << printBoard(state, true));

		vector<gameMove> minMoves = getMoves(state, true);
		
		// If MIN has move - play MIN
		if (minMoves.size() == 0)
		{
			LOG_WARNING("No moves for MIN(printed white) on board: \n" << printBoard(state, false));
			return { -1, -1 };
		}
		else
		{
			// Reset the number of evaluated boards
			_boardsEvaluated = 0;
			_maxDepthReached = 0;
			_entireSpaceCovered = true;

			// Run minimax for each move, choose the one with the best utility
			next = minValue(state, INT_MIN, INT_MAX, 0);
		}
	}
	else
	{
		// Reset the number of evaluated boards
		_boardsEvaluated = 0;
		_maxDepthReached = 0;
		_entireSpaceCovered = true;

		// Run minimax for each move, choose the one with the best utility
		next = maxValue(state, INT_MIN, INT_MAX, 0);
	}

	return next.move;
}
