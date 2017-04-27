#include "stats.h"
#include "general.h"

void parallelSearchStatsToFile(const vector<vector<slaveStats>> &jobStats, long long totTimeNs, long long seqPartNs)
{
    float boardsPerSec;
    float averageBoardsPerSlave;
    int boardRange, minBoards = INT_MAX, maxBoards = 0;
    double jobTimeRange, minJobTime = INT_MAX, maxJobTime = 0, totJobTime = 0;
    double slaveTimeRange, minSlaveTime = INT_MAX, maxSlaveTime = 0, totSlaveTime = 0;    
    double boardSD, totTimeSD, jobTimeSD;

    float seqTimeOverTotal = seqPartNs / (double) totTimeNs;

    int totBoardsEvaluated = 0;
    int nodesPruned = 0;
    int totEstPrunedMaxD = 0;
    long estNodesAtMaxD = pow(AVG_BRANCH_FACTOR, _parameters.maxDepth);
    float estPruneRatioMaxD;
    

    vector<int> boardsPerSlaveArr(_slaveCount);
    vector<long long> jobTimesPerSlaveArr(_slaveCount);
    vector<long long> totTimesPerSlaveArr(_slaveCount);
    


    // Go over all jobs, aggregate
    for (int slaveId = 0; slaveId < jobStats.size(); slaveId++)
    {
        int boardsForCurrSlave = 0;
        double jobTimeForCurrSlave = 0;
        double totTimeForCurrSlave = 0;
        
        for (slaveStats job : jobStats[slaveId])
        {
            totBoardsEvaluated += job.boardsEvaluated;
            nodesPruned += job.nodesPruned;
            totEstPrunedMaxD += job.estMaxDepthPruned;
            
            boardsForCurrSlave += job.boardsEvaluated;            
            jobTimeForCurrSlave += job.jobTime / BLN_DOUBLE;
            totTimeForCurrSlave += (job.jobTime + job.sendTime + job.receiveTime) / BLN_DOUBLE;
        }

        boardsPerSlaveArr[_slaveCount] = boardsForCurrSlave;
        maxBoards = max(boardsForCurrSlave, maxBoards);
        minBoards = min(boardsForCurrSlave, minBoards);
        
        jobTimesPerSlaveArr[slaveId] = jobTimeForCurrSlave;
        totJobTime += jobTimeForCurrSlave;
        maxJobTime = max(maxJobTime, jobTimeForCurrSlave);
        minJobTime = min(minJobTime, jobTimeForCurrSlave);

        totTimesPerSlaveArr[slaveId] = totTimeForCurrSlave;
        totSlaveTime += totTimeForCurrSlave;
        maxSlaveTime = max(maxSlaveTime, totTimeForCurrSlave);
        minSlaveTime = min(minSlaveTime, totTimeForCurrSlave);
    }

    // Calculate
    boardsPerSec = totBoardsEvaluated / (totTimeNs / BLN_DOUBLE);
    estPruneRatioMaxD = totEstPrunedMaxD / (float) estNodesAtMaxD;

    // Ranges
    boardRange = maxBoards - minBoards;
    jobTimeRange = maxJobTime - minJobTime;
    slaveTimeRange = maxSlaveTime - minSlaveTime;
    // Stddev
    averageBoardsPerSlave = totBoardsEvaluated / (float) _slaveCount;
    int boardSqDiffSum = 0;
    double jobTimeSqDiffSum = 0, slaveTimeSqDiffSum = 0;
    double avgJobTime = totJobTime / (double) _slaveCount;
    double avgTotTime = totSlaveTime / (double) _slaveCount; 
    for(int slaveId = 0; slaveId < _slaveCount; slaveId++)
    {
        boardSqDiffSum += pow(boardsPerSlaveArr[slaveId] - averageBoardsPerSlave, 2);
        jobTimeSqDiffSum += pow(jobTimesPerSlaveArr[slaveId] - avgJobTime, 2);
        slaveTimeSqDiffSum += pow(totTimesPerSlaveArr[slaveId] - avgTotTime, 2);
    }

    boardSD = sqrt(boardSqDiffSum);
    jobTimeSD = sqrt(jobTimeSqDiffSum);
    totTimeSD = sqrt(slaveTimeSqDiffSum);


    bool fileExists = false;
    if (ifstream(PAR_SEARCH_AGGR_STATS_FILENAME))
        fileExists = true;

    ofstream outfile(PAR_SEARCH_AGGR_STATS_FILENAME, ios::app | ios::ate); // Append mode, seek to the end of the file
    
    if (!outfile)
    {
        LOG_ERR("Cannot open stats file for writing");
        return;
    }

    if (!fileExists) // If we're creating the file now, write the header'
        outfile << "procCount, boardSize, loadFactor, depth, totBoardsEvaluated, totalTimeMaster, totalTimeSlaves, seqPartMaster, jobTimeSlaves, bpsec"
                << ", totNodesPruned, totEstPrunedMaxD, estPruneRatio, boardsPerSlaveRange, jobTimeRange, totSlaveTimeRange, boardsPerSlaveSD"
                << ", jobTimeSD, totSlaveTimeSD" << endl;
    
    outfile << _slaveCount + 1 << ", " << _N * _M << ", "<< _parameters.loadFactor << ", " << _parameters.maxDepth << ", " << totBoardsEvaluated << ", "
            << totTimeNs / BLN_DOUBLE << ", " << totSlaveTime << ", " << seqPartNs / BLN_DOUBLE << ", " << totJobTime << ", "
            <<  boardsPerSec << ", " << nodesPruned << ", " << totEstPrunedMaxD << ", " << estPruneRatioMaxD << ", "
            << boardRange << ", " << jobTimeRange << ", " << slaveTimeRange << ", " << boardSD << ", "
            << jobTimeSD << ", " << totTimeSD << endl;

}

void writeStatsToFile(const vector<vector<slaveStats>> &jobStats)
{
    bool fileExists = false;
    if (ifstream(STATS_FILENAME))
        fileExists = true;

    ofstream outfile(STATS_FILENAME, ios::app | ios::ate); // Append mode, seek to the end of the file

    if (!outfile)
    {
        LOG_ERR("Cannot open stats file for writing");
        return;
    }

    if (!fileExists) // If we're creating the file now, write the header'
        outfile << "boardSize, slaveCount, slaveId, sendTime, receiveTime, jobTime, boardsEvaluated, nodesPrunes, estMaxDepthPruned, maxDepthReached, entireSpace" << endl;

    for (int slaveId = 0; slaveId < jobStats.size(); slaveId++)
    {
        for (slaveStats job : jobStats[slaveId])
        {
            outfile << _M * _N << ", " << jobStats.size() << ", " << slaveId << ", " << job.sendTime << ", " << job.receiveTime
                    << ", " << job.jobTime << ", " << job.boardsEvaluated << ", " << job.nodesPruned << ", "
                    << job.estMaxDepthPruned << ", " << job.maxDepthReached << ", " << job.entireSpace << endl;
        }
    }
}

void outputStats(const vector<vector<slaveStats>> &jobStats, long long timeNs)
{
    int boardsAssesed = 0;
    int nodesPruned = 0;
    int estPruneMaxD = 0;
    int maxDepthReached = 0;
    bool entireSpace = true;

    for (int slaveId = 0; slaveId < jobStats.size(); slaveId++)
    {
        for (slaveStats job : jobStats[slaveId])
        {
            boardsAssesed += job.boardsEvaluated;
            nodesPruned += job.nodesPruned;
            estPruneMaxD += job.estMaxDepthPruned;
            if(job.maxDepthReached > maxDepthReached)
                maxDepthReached = job.maxDepthReached;
            if (!job.entireSpace)
                entireSpace = false;
        }
    }

    cout << "Number of boards assesed: " << boardsAssesed << endl;
    cout << "Number of nodes pruned: " << nodesPruned << endl;
    cout << "Estimated number of pruned nodes at maxDepth: " << estPruneMaxD << endl;
    cout << "Depth of boards: " << maxDepthReached << endl;
    cout << "Entire space: " << entireSpace << endl;
    cout << "Elapsed time in seconds: " << timeNs / 1000000000.0 << endl;
    cout << "Boards per second: " << boardsAssesed / (timeNs / 1000000000.0) << endl;
}

void staticEvalStatsToFile(float totalTimeInSec)
{
    bool fileExists = false;
    if (ifstream(STATIC_EVAL_STATS_FILENAME))
    fileExists = true;

    ofstream outfile(STATIC_EVAL_STATS_FILENAME, ios::app | ios::ate); // Append mode, seek to the end of the file

    if (!outfile)
    {
        LOG_ERR("Cannot open stats file for writing");
        return;
    }

    if (!fileExists) // If we're creating the file now, write the header'
        outfile << "staticEval, procCount, boardSize, boardsEvaluated, bpsec, totalTime, evaluationTime, commTime, prunedNodes, estPrunedAtMaxD, estPruneRatio" << endl;

        // TODO: Write the actual stats to the file
    outfile << _parameters.useStaticEvaluation << ", " << _slaveCount + 1 << ", " << _N * _M << ", " << _boardsEvaluated << ", " << _boardsEvaluated / totalTimeInSec
            << ", " << totalTimeInSec << ", " << _totalEvaluationTime / BLN_DOUBLE << ", " << _parallelEvalCommTime / BLN_DOUBLE << ", " << _nodesPruned << ", "
            << _estMaxDepthPruned << ", " << _estMaxDepthPruned / (double) pow(AVG_BRANCH_FACTOR, _parameters.maxDepth) << endl; 
}