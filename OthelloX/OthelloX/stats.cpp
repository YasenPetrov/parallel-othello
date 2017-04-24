#include "stats.h"

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
        outfile << "slaveCount, slaveId, sendTime, receiveTime, jobTime, boardsEvaluated, nodesPrunes, estMaxDepthPruned, maxDepthReached, entireSpace" << endl;

    for (int slaveId = 0; slaveId < jobStats.size(); slaveId++)
    {
        for (slaveStats job : jobStats[slaveId])
        {
            outfile << jobStats.size() << ", " << slaveId << ", " << job.sendTime << ", " << job.receiveTime
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