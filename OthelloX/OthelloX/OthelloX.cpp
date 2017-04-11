// OthelloX.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "general.h"
#include "board.h"
#include "parsing.h"
#include "search.h"
#include "timing.h"

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		cout << "Usage: ./othello <path-to-initial-board-file> <path-to-eval-params-file>" << endl;
		return -1;
	}
	
	board state;	// Our game board
	
	float secondsForSearch;	// Store the fime for each search

	// Parse the parameters file
	if (!parseParamsFile(argv[2], _parameters))
	{
		LOG_ERR("Error while parsing parameters file:");
		LOG_ERR(argv[2]);
		return -1;
	}


	// Parse the board file
	if (!parseBoardFile(argv[1], _parameters.black, state))
	{
		LOG_ERR("Error whle parsing board file:");
		LOG_ERR(argv[1]);
		return -1;
	}

	cout << printBoard(state, _parameters.black);
	
	while (1)
	{
		cout << "Calculating next move ..." << endl;
		startTimer();
		gameMove nextMove = search(state);
		secondsForSearch = secondsElapsed();
		
		if (nextMove.x == -1 && nextMove.y == -1)
		{
			LOG_ERR("Search did not produce a valid move");
			return -1;
		}

		state = applyMove(state, nextMove, _parameters.black);

		cout << endl << printBoard(state, _parameters.black);
		cout << "Number of boards assesed: " << _boardsEvaluated << endl;
		cout << "Depth of boards: " << _maxDepthReached << endl;
		cout << "Entire space: " << _entireSpaceCovered << endl;
		cout << "Elapsed time in seconds: " << secondsForSearch << endl;

		// Get, parse and apply player move
		string nextPlMoveString;
		gameMove playerMove;
		bool gotValidMove = false;
		do
		{
			if (getMoves(state, false).size() == 0) break;

			cout << "Your move(ex: d5): ";
			cin >> nextPlMoveString;
			cout << endl;
			if (parsePosition(nextPlMoveString, playerMove.x, playerMove.y, state.size(), state[0].size()))
			{
				if (isValidMove(flipAll(state), playerMove.y, playerMove.x)) gotValidMove = true;
				else cout << "Invalid move, try again" << endl;
			}
			else cout << "Cannot parse move, try again(lowercase letters, please" << endl;
		} while (!gotValidMove );

		if(gotValidMove)
			state = applyMove(state, playerMove, false);

		cout << printBoard(state, _parameters.black);
	}


	return 0;
}

