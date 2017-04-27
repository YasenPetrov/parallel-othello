// OthelloX.cpp : Defines the entry point for the console application.
//
#include <sched.h>

#include "stdafx.h"
#include "general.h"
#include "board.h"
#include "parsing.h"
#include "search.h"
#include "timing.h"
#include "processes.h"

int _currentProcId = -1;
int _slaveCount = -1;
int _squaresPerProc;
int _remainderSquares;
board _sharedBoard;
int *_sendCounts;
int *_displacements;
int *_subScores;


long long _totalEvaluationTime = 0;
long long _parallelEvalCommTime = 0;
long long _parallelEvalCompTime = 0;


int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	// One master, N - 1 slaves
	_slaveCount = worldSize - 1;

	// Who am I?(And who are you? Who, who... who, who..)
	MPI_Comm_rank(MPI_COMM_WORLD, &_currentProcId); // Tell me who are YOU?

	board state;	// Our game board
	float secondsForSearch;	// Store the fime for each search

	// Parse the arguments in the master process
	if(_currentProcId == MASTER_ID)
	{
		if (argc < 3)
		{
			cout << "Usage: ./othello <path-to-initial-board-file> <path-to-eval-params-file>" << endl;
			MPI_Abort(MPI_COMM_WORLD, -1);		
		}
		
		// Parse the parameters file
		if (!parseParamsFile(argv[2], _parameters))
		{
			LOG_ERR("Error while parsing parameters file " << argv[2]);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}

		// Parse the board file
		if (!parseBoardFile(argv[1], state, _parameters))
		{
			LOG_ERR("Error whle parsing board file " << argv[1]);
			MPI_Abort(MPI_COMM_WORLD, -1);
		}
	}

	// Send the parameters to everyone
	MPI_Bcast(&_parameters, sizeof(_parameters), MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_M, 1, MPI_INT, MASTER_ID, MPI_COMM_WORLD);
	MPI_Bcast(&_N, 1, MPI_INT, MASTER_ID, MPI_COMM_WORLD);

	// Maybe initialize weights for static evaluation
	if(_parameters.useStaticEvaluation)
	{
		_squareWeights = board(_N * _M);
		fillWeightsMatrix(_squareWeights);
		if(_slaveCount > 0)
		{
			_squaresPerProc = (_N * _M) / worldSize;
			_remainderSquares = (_N * _M) % worldSize;
			_sharedBoard = board(_N * _M, 0);
			_sendCounts = new int[worldSize];
			_displacements = new int[worldSize];
			_subScores = new int[worldSize];
			int nextSquareIndex = 0; // Displacement
			int procsWithExtraSquare = _remainderSquares;
			for(int procId = 0; procId < worldSize; procId++)
			{
				// If boardSize & slaveCount = r, the next line ensures that the first r processes get 1 more square than the rest
				_sendCounts[procId] = (procsWithExtraSquare-- > 0 ? _squaresPerProc + 1 : _squaresPerProc);
				_displacements[procId] = nextSquareIndex;
				nextSquareIndex += _sendCounts[procId];
			}
		}
	}

	startTimer();

	// If we only have one process or we've switched parallel search off, do what we would do in serial mode
	if(_slaveCount < 2 || !_parameters.parallelSearch)
	{
		if(_currentProcId != MASTER_ID)
		{
			if(_parameters.parallelSearch || !_parameters.useStaticEvaluation) // If we want to do parallel search but we have only one slave, kill it an let the master do the work
			{
				MPI_Finalize();
				return 0;
			}
		}
		else // This is the master process
		{
			cout << "Running search.." << endl;
			_parameters.parallelSearch = false;
			timePoint before = timeNow();
			timePoint after;
			vector<gameMove> nextMoves = treeSearch(state, _parameters.maxDepth, false, true); // Play for MAX
			after = timeNow();
			long long nsForSearch = nsBetween(before, after);
			secondsForSearch = secondsElapsed();
			
			if (nextMoves.size() == 0)
			{
				LOG_ERR("Search did not produce a valid move");
				return -1;
			}

			state = applyMove(state, nextMoves[0], true); // The computer player is MAX
			
			// saveBoardToFile(state, "../initialbrd.txt", _parameters.black);

			cout << endl << printBoard(state, _parameters.black);
			cout << "Number of boards assesed: " << _boardsEvaluated << endl;
			cout << "Number of nodes pruned: " <<_nodesPruned << endl;
			cout << "Estimated number of pruned nodes at maxDepth: " << _estMaxDepthPruned << endl;
			cout << "Depth of boards: " << _maxDepthReached << endl;
			cout << "Entire space: " << _entireSpaceCovered << endl;
			cout << "Elapsed time in seconds: " << secondsForSearch << endl;
			cout << "Elapsed time in seconds: " << (nsForSearch / BLN_DOUBLE) << endl;
			cout << "Search without evaluation: " << (nsForSearch - _totalEvaluationTime) / BLN_DOUBLE << endl; 
			cout << "Boards per second: " << _boardsEvaluated / secondsForSearch << endl;
			cout << "Total evaluation time: " << _totalEvaluationTime / BLN_DOUBLE << endl;
			cout << "Parallel evaluation comm time: " << _parallelEvalCommTime / BLN_DOUBLE << endl;
			cout << "comm/totTime for evaluation: " << (double)_parallelEvalCommTime / (double)_totalEvaluationTime << endl;
			cout << "Master spent ns on comp in parallel eval: " << (_totalEvaluationTime - _parallelEvalCommTime) / BLN_DOUBLE << endl;
			cout << "Diff btw start and end of computation in master: " << _parallelEvalCompTime / BLN_DOUBLE << endl;

			// Save stats to file
			staticEvalStatsToFile(secondsForSearch);

			// If we have slaves, tell them they'll get no more work
			int8_t moreWork = false;
			MPI_Bcast(&moreWork, 1, MPI_BYTE, MASTER_ID, MPI_COMM_WORLD);

			MPI_Finalize();
			return 0;
		}
	}

	
	int sc_status;
	cpu_set_t my_set;
	unsigned short mask;

	memset(&my_set, 0, sizeof(my_set));
	CPU_ZERO(&my_set);
	sc_status = sched_getaffinity(0, sizeof(my_set), &my_set);
	if (sc_status)
	{
		perror("sched_getaffinity");
		return sc_status;
	}
	
	mask = 0;
	
	if (CPU_ISSET(0, &my_set)) mask |= 0x0001;
	if (CPU_ISSET(1, &my_set)) mask |= 0x0002;
	if (CPU_ISSET(2, &my_set)) mask |= 0x0004;
	if (CPU_ISSET(3, &my_set)) mask |= 0x0008;
	if (CPU_ISSET(4, &my_set)) mask |= 0x0010;
	if (CPU_ISSET(5, &my_set)) mask |= 0x0020;
	if (CPU_ISSET(6, &my_set)) mask |= 0x0040;
	if (CPU_ISSET(7, &my_set)) mask |= 0x0080;
	if (CPU_ISSET(8, &my_set)) mask |= 0x0100;
	if (CPU_ISSET(9, &my_set)) mask |= 0x0200;
	if (CPU_ISSET(10, &my_set)) mask |= 0x0400;
	if (CPU_ISSET(11, &my_set)) mask |= 0x0800;
	if (CPU_ISSET(12, &my_set)) mask |= 0x1000;
	if (CPU_ISSET(13, &my_set)) mask |= 0x2000;
	if (CPU_ISSET(14, &my_set)) mask |= 0x4000;
	if (CPU_ISSET(15, &my_set)) mask |= 0x8000;
	 
	printf("Process %d is on processor mask 0x%x\n", _currentProcId, mask);

	if(_currentProcId == MASTER_ID)
	{
		cout << "Master: " << _currentProcId  << " will run parallel search" << endl;
		masterMain(state, _slaveCount);
	}
	else
	{
		if(!_parameters.parallelSearch)
		{
			cout << "Slave: " << _currentProcId << " will run parallel evaluation" << endl;					
			slaveBoardEval();
		}
		else
		{
			cout << "Slave: " << _currentProcId << " will run parallel search" << endl;		
			slaveMain(MASTER_ID, _currentProcId);
		}
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
	// 		if (parsePosition(nextPlMoveString, playerMove.x, playerMove.y, _M, _N)))
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

