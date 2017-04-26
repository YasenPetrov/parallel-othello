#include "stdafx.h"
#include "search.h"

#define MOVE_ORDER_SEARCH_DEPTH 2
#define AVG_BRANCH_FACTOR 7

// The parameters for searching
evalParams _parameters;

// How many boards have we evaluated in total
int _boardsEvaluated = 0;
// The maximum depth we reached
int _maxDepthReached = 0;
// Have we looked through the entire search space
bool _entireSpaceCovered = true;
// How many nodes we pruned
int _nodesPruned = 0;
// Estimate of the nodes at maxDepth that were pruned
int _estMaxDepthPruned = 0;

int negaMax(const board &state, short depth, short alpha, short beta, bool maxTurn, bool isProbe)
{
	// LOG_DEBUG("Negamax, MAX: " << maxTurn << " probe: " << isProbe << " depth: " << depth << " alpha: " << alpha << " beta: " << beta << " board: " << endl << printBoard(state, _parameters.black));

	short multiplier = maxTurn ? 1 : -1; // When we evaluate a node for MIN, we want to return <-score>

	if (!isProbe && _parameters.maxDepth - depth > _maxDepthReached)
		_maxDepthReached = _parameters.maxDepth - depth;

	if (depth <= 0 || secondsElapsed() > _parameters.timeout)
	{
		// if(!isProbe) LOG_DEBUG("DEPTH is " << depth << " or out of time");
		if(!isProbe) _boardsEvaluated++;
		// If we have more moves but we have to stop, we haven't checked everything
		if(!isProbe && (getMoves(state, true).size() > 0 || getMoves(state,false).size() > 0))
			_entireSpaceCovered = false;
		// Always evaluate for MAX!
		return evalBoard(state) * multiplier;
	}

	vector<gameMove> moves(0);

	if (isProbe || !_parameters.useMoveOrdering)
	{
		moves = getMoves(state, maxTurn);
	}
	else
	{
		moves = treeSearch(state, MOVE_ORDER_SEARCH_DEPTH, true, maxTurn);
	}

	// If no moves - evaluate board
	if (moves.size() == 0)
	{
		// LOG_DEBUG(" No more moves for MAX, board: " << endl << printBoard(state, _parameters.black));
		vector<gameMove> opponentMoves = getMoves(state, !maxTurn);

		if (opponentMoves.size() == 0)
		{
			// LOG_DEBUG(" No more moves for MIN, board: " << endl << printBoard(state, _parameters.black));
			if(!isProbe) _boardsEvaluated++;
			return evalBoard(state) * multiplier;
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
		//_entireSpaceCovered = false;
		return evalBoard(state) * multiplier;
	}

	int maxValue = INT_MIN + 1;

	for (auto it = moves.begin(); it != moves.end(); it++)
	{
		int value = -negaMax(applyMove(state, *(it), maxTurn), depth - 1, -beta, -alpha, !maxTurn, isProbe);

		if (value > maxValue) maxValue = value;

		if (_parameters.usePruning && maxValue > alpha)
		{
			alpha = maxValue;
			if (maxValue >= beta)
			{
				_nodesPruned += moves.end() - it;
				// We've we're pruning the rest of the children
				_estMaxDepthPruned += pow(AVG_BRANCH_FACTOR, depth - 1) * (moves.end() - it) - depth;
				return maxValue;
			}
		}
	}

	return maxValue;
}


vector<gameMove> treeSearch(const board &state, short maxDepth, bool isProbe, bool isMaxTurn)
{

	// Get next moves
	vector<gameMove> moves = getMoves(state, isMaxTurn);

	// If we're trying to get a move ordering and we have less than 2 moves, we're done
	if(isProbe && moves.size() < 2) return moves;

	// This will also hold the utility values for each possible move
	vector<valueMove> orderedMoves = vector<valueMove>(moves.size());

	if (!isProbe)
	{
		// Reset the number of evaluated boards
		_boardsEvaluated = 0;
		_maxDepthReached = 0;
		_nodesPruned = 0;
		_estMaxDepthPruned = 0;
		_entireSpaceCovered = true;
	}

	short alpha = SHRT_MIN + 1;
	short beta = SHRT_MAX - 1;

	for (int i = 0; i < moves.size(); i++)
	{
		orderedMoves[i].move = moves[i];
		// ! Call with -beta, -alpha, since we update alpha
		int val = (isMaxTurn ? -1 : 1) * negaMax(applyMove(state, moves[i], isMaxTurn), maxDepth - 1, -beta, -alpha, !isMaxTurn, isProbe);
		orderedMoves[i].value = val;
		
		// if (val > alpha) alpha = val;
	}

	sort(orderedMoves.begin(), orderedMoves.end(), [](const valueMove &left, const valueMove &right)
	{
		return left.value > right.value; // Sort in descending order
	});

	if(!isProbe)
	{
		cout << "Moves: " << endl;
		for (valueMove mv : orderedMoves)
		{
			cout << (char)(mv.move.x + 'a') << mv.move.y + 1 << " with a score of " << mv.value << endl;
		}
	}

	for (int i = 0; i < moves.size(); i++)
	{
		moves[i] = orderedMoves[i].move;
	}

	return moves;
}

int slaveSearch(const board &state, short maxDepth, bool isMaxTurn, int currentDepth)
{
	board stateCopy = board(state);

	// Reset the statistics measures
	_boardsEvaluated = 0;
	_maxDepthReached = 0;
	_nodesPruned = 0;
	_estMaxDepthPruned = 0;
	_entireSpaceCovered = true;

	short alpha = SHRT_MIN + 1;
    short beta = SHRT_MAX - 1;

    if(isMaxTurn)
    {
        return negaMax(stateCopy, maxDepth - currentDepth, alpha, beta, isMaxTurn, false);
    }
    else
    {
        return -negaMax(stateCopy, maxDepth - currentDepth, -beta, -alpha, isMaxTurn, false);
    }
}