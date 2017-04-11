#include "stdafx.h"
#include "hashing.h"

// Store the random values   in a 3 x (N x M) 2d array - a different bitstring for every possible state(MIN, MAX, empty) of each square
vector<vector<unsigned long long>> _randomNumbers;

// Only store the hashes, not the whole board - collisions are extremely rare and do not affect performance greatly
unordered_map<unsigned long long, hashEntry> _map;

unsigned long long getHash(const board &state)
{
	unsigned long long hash = 0ULL;

	// Go over each square and XOR the has with the corresponding random number
	for (int i = 0; i < state.size(); i++)
	{
		for (int j = 0; j < state[0].size(); j++)
		{
			// Values on the board got from -1 to 1, so increment them by 1 to get
			// the random table row index
			hash ^= _randomNumbers[state[i][j] + 1][i * state.size() + j];
		}
	}
}

void initZorbistTable(int N, int M)
{
	// Define a uniform integer distribution to generate random bitstrings of length 64 using a Mersenne twister engine
	random_device rd;
	mt19937_64 mtEngine(rd());
	uniform_int_distribution<unsigned long long> dist(llround(pow(2, 63)), llround(pow(2, 64)));

	_randomNumbers = vector<vector<unsigned long long>>(3, vector<unsigned long long>(M * N));

	for (auto it = _randomNumbers.begin(); it != _randomNumbers.end(); it++)
	{
		for (auto it_i = it->begin(); it_i != it->end(); it_i++)
		{
			*(it_i) = dist(mtEngine);
		}
	}
	
}

void hashState(const board & state, hashEntry info)
{
	_map[getHash(state)] = info;
}

bool getInfoForState(const board & state, hashEntry & info)
{
	unsigned long long hash = getHash(state);
	auto res = _map.find(hash);

	if (res != _map.end()) // There is such a key - a hit
	{
		info = res->second;
		return true;
	}

	return false; // No luck
}
