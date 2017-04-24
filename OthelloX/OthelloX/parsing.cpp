#include "stdafx.h"
#include "parsing.h"


// Trim a string from starting and trailing characters
string trim(string s, char c)
{
	int first = s.find_first_not_of(c);
	int last = s.find_last_not_of(c);

	return s.substr(first, (last - first + 1));
}

bool parsePosition(string pos, int &xPos, int &yPos, int N, int M)
{
	if (pos.size() < 2)
	{
		LOG_ERR("Malformed argument for a position: " << pos);
		return false;
	}

	xPos = (int)(pos[0] - 'a');
	yPos = stoi(pos.substr(1)) - 1; // The input we get is 1-based, so substract 1

	if (xPos < 0 || xPos >= M || yPos < 0 || yPos >= N)
	{
		LOG_ERR("Invalid position for a board of size " << N << "x" << M << ": " << pos);
		return false;
	}

	return true;
}

// Parse the positions in the vector put a 1 or -1 on the board for each
// position, depending on whether max is true or false 
bool fillPositions(const vector<string> positions, board &state, bool max)
{
	// Fill with 1 or -1 depending on whether these are MAX or MIN discs
	signed char disc = max ? 1 : -1;

	// Parse all positions and mark them on the board
	for (auto it = positions.begin(); it != positions.end(); it++)
	{
		int xPos, yPos;

		if (!parsePosition(*it, xPos, yPos, _M, _N)) return false;

		state[yPos][xPos] = disc;
	}

	return true;
}

bool parseBoardFile(const char * filename, board &state, evalParams &params)
{
	// Parameters for the board
	vector<string> whitePositions;
	vector<string> blackPositions;
	bool gotBoardSize = false;
	bool gotWhitePositions = false;
	bool gotBlackPositions = false;
	bool gotColor = false;
	bool gotTimeout = false;

	// Open file for reading
	ifstream in(filename);
	if (!in)
	{
		LOG_ERR("Cannot open file for reading: " << filename);
		return false;
	}

	// Parse line by line
	while (in)
	{
		// Read the line
		string line;
		getline(in, line);

		// Skip empty lines
		if (line.compare("") == 0) continue;

		// Split on ':'
		vector<string> tokens;
		istringstream is(line);
		string tok;
		while (getline(is, tok, ':'))
		{
			tokens.push_back(tok);
		}

		if (tokens.size() != 2)
		{
			LOG_ERR("Malformed line: "<< line);
			return false;
		}
		
		string param = trim(trim(tokens[0], ' '), '\r');
		string arg = trim(trim(tokens[1], ' '), '\r');

		// Match the first part against the constants for the parameters, parse the rest
		if (param.compare(PRS_SIZE) == 0)
		{
			int commaIndex = arg.find(",");
			try
			{
				_M = stoi(arg.substr(0, commaIndex));
				_N = stoi(arg.substr(commaIndex + 1));


				// Initialise board with zeros
				state = board(_M, vector<signed char>(_N, 0));

				gotBoardSize = true;
			}
			catch (invalid_argument &_)
			{
				LOG_ERR("Malformed argument for board size: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_BRD_WHITE) == 0 || param.compare(PRS_BRD_BLACK) == 0)
		{
			int openBracketIndex = arg.find("{");
			int closeBracketIndex = arg.find("}");

			if (openBracketIndex == string::npos || closeBracketIndex == string::npos)
			{
				LOG_ERR("Malformed argument for figure positions: " << arg);

				return false;
			}

			// Trim brackets
			arg = arg.substr(openBracketIndex + 1, closeBracketIndex - openBracketIndex - 1);

			// Get all positions(ex: d7)
			vector<string> positions;
			istringstream iss(arg);
			string pos;
			while (getline(iss, pos, ','))
			{
				if(pos.compare("") == 0 || pos.compare(" ") == 0)
					continue;
				positions.push_back(trim(pos, ' '));
			}

			if (param.compare(PRS_BRD_WHITE) == 0)
			{
				whitePositions = vector<string>(positions);
				gotWhitePositions = true;
			}
			else
			{
				blackPositions = vector<string>(positions);
				gotBlackPositions = true;
			}
		}
		else if (param.compare(PRS_COLOR) == 0)
		{
			if (arg.compare(PRS_COLOR_BLACK) == 0)
			{
				params.black = true;
				gotColor = true;
			}
			else if (arg.compare(PRS_COLOR_WHITE) == 0)
			{
				params.black = false;
				gotColor = true;
			}
			else
			{
				LOG_ERR("Bad argument for color: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_TIMEOUT) == 0)
		{
			try
			{
				params.timeout= stof(arg);
				gotTimeout = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for timeout: " << arg);
				return false;
			}
		}

		else
		{
			LOG_WARNING("Unexpected token in board file: " << tokens[0]);
		}
	}

	// Make sure we've got all we need
	if (!gotBoardSize || !gotBlackPositions || !gotWhitePositions || !gotTimeout || !gotColor)
	{
		LOG_ERR("While parsing board file - not all of the required information is specified. Make sure you provide a board size and positions for white and black discs");
		return false;
	}

	// Fill discs for both players, choosing MAX or MIN discs depending on whether we want the best move for black or white
	if (!fillPositions(blackPositions, state, params.black) || !fillPositions(whitePositions, state, !params.black))
	{
		LOG_ERR("Error while filling positions");
		return false;
	}

	// If we got to here, parsing was successful
	return true;
}


bool parseParamsFile(const char * filename, evalParams &params)
{
	// Flags, indicating whether we found the respective params in the file
	bool gotMaxBoards = false, gotMaxDepth = false, gotParityWeight = false,
		gotStabilityWeight = false, gotMobilityWeight = false, gotPruning = false,
		gotMoveOrdering = false, gotLoadFactor = false;



	// Open file for reading
	ifstream in(filename);
	if (!in)
	{
		fprintf(stderr, "Cannot open %s for reading", filename);
		LOG_ERR("Cannot open file for reading: " << filename);
		return false;
	}

	// Parse line by line
	while (in)
	{
		// Read the line
		string line;
		getline(in, line);

		// Skip empty lines
		if (line.compare("") == 0) continue;

		// Split on ':'
		vector<string> tokens;
		istringstream is(line);
		string tok;
		while (getline(is, tok, ':'))
		{
			tokens.push_back(tok);
		}

		if (tokens.size() != 2)
		{
			LOG_ERR("Malformed line: " << line);
			return false;
		}

		string param = trim(trim(tokens[0], '\r'), ' ');
		string arg = trim(trim(tokens[1], '\r'), ' ');

		// Match the first part against the constants for the parameters, parse the rest
		if (param.compare(PRS_MAX_BOARDS) == 0)
		{
			try
			{
				params.maxBoards = stoi(arg);
				gotMaxBoards = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for max boards: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_MAX_DEPTH) == 0)
		{
			try
			{
				params.maxDepth = stoi(arg);
				gotMaxDepth = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for max depth: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_STABILITY_WEIGHT) == 0)
		{
			try
			{
				params.stabilityWeight= stof(arg);
				gotStabilityWeight = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for stability weight: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_MOBILITY_WEIGHT) == 0)
		{
			try
			{
				params.mobilityWeight = stof(arg);
				gotMobilityWeight = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for mobility weight: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_PARITY_WEIGHT) == 0)
		{
			try
			{
				params.parityWeight = stof(arg);
				gotParityWeight = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for parity weight: " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_PRUNING) == 0)
		{
			try
			{
				params.usePruning = (bool) stoi(arg);
				gotPruning = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for pruning(0 or 1): " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_MOVE_ORDERING) == 0)
		{
			try
			{
				params.useMoveOrdering = (bool) stoi(arg);
				gotMoveOrdering = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for move ordering(specify 0 or 1): " << arg);
				return false;
			}
		}
		else if (param.compare(PRS_LOAD_FACTOR) == 0)
		{
			try
			{
				params.loadFactor = stoi(arg);
				gotLoadFactor = true;
			}
			catch (const std::exception&)
			{
				LOG_ERR("Bad argument for load factor: " << arg);
				return false;
			}
		}
		else
		{
			LOG_WARNING("Unexpected token in parameters file: " << tokens[0]);
		}
	}

	// Make sure we have everything we need
	if (!gotMaxBoards || !gotMaxDepth)
	{
		LOG_ERR("Missing some evaluation parameters. Make sure you specify " << PRS_MAX_BOARDS << ", "
			<< PRS_MAX_DEPTH);
		return false;
	}
	if (!gotMobilityWeight || !gotParityWeight || !gotStabilityWeight || !gotLoadFactor || !gotPruning || !gotMoveOrdering)
	{
		LOG_WARNING("Missing some or all additional parameters. Using default values");
	}

	// If we got to here, parsing was successful
	return true;
}
