// OthelloX.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "general.h"
#include "board.h"
#include "parsing.h"
#include "search.h"
#include "timing.h"
#include "processes.h"

// The slaves are processes 0 to (N-2) and process (N-1) is the master
#define MASTER_ID slaveCount

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	// One master, N - 1 slaves
	int slaveCount = worldSize - 1;

	// Who am I?(And who are you? Who, who... who, who..)
	int currentProcId;
	MPI_Comm_rank(MPI_COMM_WORLD, &currentProcId); // Tell me who are YOU?

	board state;	// Our game board
	float secondsForSearch;	// Store the fime for each search

	// Parse the arguments in the master process
	if(currentProcId == MASTER_ID)
	{
		if (argc < 3)
		{
			cout << "Usage: ./othello <path-to-initial-board-file> <path-to-eval-params-file>" << endl;
			MPI_Abort(MPI_COMM_WORLD, -1);		
		}
		
		// Parse the parameters file
		if (!parseParamsFile(argv[2], _parameters))
		{
			LOG_ERR("Error while parsing parameters file:");
			LOG_ERR(argv[2]);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		// Parse the board file
		if (!parseBoardFile(argv[1], _parameters.black, state))
		{
			LOG_ERR("Error whle parsing board file:");
			LOG_ERR(argv[1]);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
	}

	startTimer();

	// If we only have one process, do what we would do in serial mode
	if(slaveCount == 0)
	{
		vector<gameMove> nextMoves = treeSearch(state, _parameters.maxDepth, false);
		secondsForSearch = secondsElapsed();
		
		if (nextMoves.size() == 0)
		{
			LOG_ERR("Search did not produce a valid move");
			return -1;
		}

		state = applyMove(state, nextMoves[0], true); // The computer player is MAX

		cout << endl << printBoard(state, _parameters.black);
		cout << "Number of boards assesed: " << _boardsEvaluated << endl;
		cout << "Depth of boards: " << _maxDepthReached << endl;
		cout << "Entire space: " << _entireSpaceCovered << endl;
		cout << "Elapsed time in seconds: " << secondsForSearch << endl;
		
		MPI_Finalize();
		return 0;
	}

	// Send the parameters to everyone
	MPI_Bcast(&_parameters.maxDepth, 1, MPI_INT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.maxBoards, 1, MPI_INT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.parityWeight, 1, MPI_FLOAT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.mobilityWeight, 1, MPI_FLOAT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.stabilityWeight, 1, MPI_FLOAT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.timeout, 1, MPI_FLOAT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.useTranspositionTable, sizeof(bool), MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_parameters.black, sizeof(bool), MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
	

	if(currentProcId == MASTER_ID)
	{
		cout << "Master: " << currentProcId << endl;
		masterMain(state, slaveCount);
	}
	else
	{
		cout << "Slave: " << currentProcId << endl;		
		slaveMain(MASTER_ID, currentProcId);
	}

	
	MPI_Finalize();
	return 0;

	// while (1)
	// {
	// 	cout << "Calculating next move ..." << endl;
	// 	startTimer();
	// 	vector<gameMove> nextMoves = treeSearch(state, _parameters.maxDepth, false);
	// 	secondsForSearch = secondsElapsed();
		
	// 	if (nextMoves.size() == 0)
	// 	{
	// 		LOG_ERR("Search did not produce a valid move");
	// 		return -1;
	// 	}

	// 	state = applyMove(state, nextMoves[0], true); // The computer player is MAX

	// 	cout << endl << printBoard(state, _parameters.black);
	// 	cout << "Number of boards assesed: " << _boardsEvaluated << endl;
	// 	cout << "Depth of boards: " << _maxDepthReached << endl;
	// 	cout << "Entire space: " << _entireSpaceCovered << endl;
	// 	cout << "Elapsed time in seconds: " << secondsForSearch << endl;

	// 	// Get, parse and apply player move
	// 	string nextPlMoveString;
	// 	gameMove playerMove;
	// 	bool gotValidMove = false;
	// 	do
	// 	{
	// 		if (getMoves(state, false).size() == 0) break;

	// 		cout << "Your move(ex: d5): ";
	// 		cin >> nextPlMoveString;
	// 		cout << endl;
	// 		if (parsePosition(nextPlMoveString, playerMove.x, playerMove.y, state.size(), state[0].size()))
	// 		{
	// 			if (isValidMove(flipAll(state), playerMove.y, playerMove.x)) gotValidMove = true;
	// 			else cout << "Invalid move, try again" << endl;
	// 		}
	// 		else cout << "Cannot parse move, try again(lowercase letters, please)" << endl;
	// 	} while (!gotValidMove );

	// 	if(gotValidMove)
	// 		state = applyMove(state, playerMove, false); // The human player is MIN

	// 	cout << printBoard(state, _parameters.black);
	// }


	// return 0;
}

