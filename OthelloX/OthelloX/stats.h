#pragma once
#include "stdafx.h"

#define STATS_FILENAME "jobStats.csv"
#define PAR_SEARCH_AGGR_STATS_FILENAME "parSearchAggr.csv"
#define STATIC_EVAL_STATS_FILENAME "staticEval.csv"


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

struct staticEvalStats
{
    long long totalTime;
    long long commTime;
    int boardsEvaluated;
    int nodesPruned;
    int estMaxDepthPruned;
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

/*
* Aggregate stats for parallel search
*/
void parallelSearchStatsToFile(const vector<vector<slaveStats>> &jobStats, long long totTimeNs, long long seqPartNs);

/*
* Stats for static eval (parallel, too! wohoo!)
*/
void staticEvalStatsToFile(float totalTimeInSec);