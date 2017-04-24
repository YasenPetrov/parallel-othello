#include "stdafx.h"

#define STATS_FILENAME "jobStats.csv"

struct slaveStats
{
    long long sendTime;
    long long receiveTime;
    long long jobTime;
    int boardsEvaluated;
    int nodesPruned;
    int estMaxDepthPruned;
    int maxDepthReached;
    bool entireSpace;
};

struct serialStats
{
    long long jobTime;
    int boardsEvaluated;
    int nodesPruned;
    int estMaxDepthPruned;
    int maxDepthReached;
    bool entireSpace;
};

/*
* Write the stats received from the slaves to the stats file
* - info per job
*/
void writeStatsToFile(const vector<vector<slaveStats>> &jobStats);

/*
* Aggregate and output data as per spec
*/
void outputStats(const vector<vector<slaveStats>> &jobStats, long long timeNs);