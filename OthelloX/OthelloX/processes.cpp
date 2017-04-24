#include "processes.h"
#include "search.h"
#include "timing.h"

/*
********* MASTER FUNCTIONS *********
*/

void masterMain(board initState, int slaveCount)
{
    timePoint before, after;
    timePoint masterStart = timeNow();
    timePoint masterEnd;
    long long totalMasterTime = 0;
    long long totalRecvTime = 0;
    long long totalSendTime = 0;
    long long nodeGenerationTime = 0;
    long long scorePropagationTime = 0;

    // Generate some nodes
    vector<stateNode> nodes;
    queue<int> jobQueue;
    before = timeNow();
    generateNodes(initState, slaveCount * _parameters.loadFactor, nodes, jobQueue);
    after = timeNow();
    nodeGenerationTime = nsBetween(before, after);

    int jobsToComplete = jobQueue.size(); // How many jobs do we have in the pool at the start
    cout << "Job count: " << jobsToComplete << endl;
    vector<vector<slaveStats>> jobStats(slaveCount, vector<slaveStats>(0)); // Used to store statistics about each job per slave

    // Get the depth of a node at <nodeIdx>
    auto getDepth = [nodes](int nodeIdx) {
        if (nodeIdx == 0)
            return 0;
        else
        {
            int res = 0;
            while (nodeIdx != 0)
            {
                nodeIdx = nodes[nodeIdx].parentIndex;
                res++;
            }
            return res;
        }
    };

    if (nodes.size() == 1) // We only have the root - we have no possible moves
    {
        cout << "{ na }";
        // Tell slaves there is no work to do
        for (int slaveId = 0; slaveId < slaveCount; slaveId++)
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            before = timeNow();
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
            after = timeNow();
            totalSendTime += nsBetween(before, after);
        }
        return;
    }

    // Send a job to each slave
    for (int slaveId = 0; slaveId < slaveCount; slaveId++)
    {
        if (jobQueue.size() > 0)
        {
            int nodeId = jobQueue.front();
            jobQueue.pop();

            totalSendTime += sendJob(nodes[nodeId], nodeId, slaveId, getDepth(nodeId));
        }
        else // If somehow the number of slaves is greater than the pool size, we tell the other slaves there is no work for them
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            before = timeNow();
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
            after = timeNow();
            totalSendTime += nsBetween(before, after);
        }
    }

    // While we haven't received results for all jobs
    while (jobsToComplete > 0)
    {
        // Wait for a slave to signal it's done, get the result from it
        int slaveId;
        before = timeNow();
        jobResult result = receiveResult(slaveId);
        after = timeNow();
        totalRecvTime += nsBetween(before, after);
        jobsToComplete--;

        // Receive stats
        slaveStats stats;
        MPI_Recv(&stats, sizeof(slaveStats), MPI_CHAR, slaveId, Tags::SEARCH_JOB_STATS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        jobStats[slaveId].push_back(stats);
        // cout << "Received a result for job " << result.jobId << " from slave " << slaveId << endl;

        // Update the node with the result
        if (result.score > nodes[result.jobId].bestScore)
            nodes[result.jobId].bestScore = result.score;

        if (jobQueue.size() > 0) // If we have more jobs send one to the slave that just completed a job
        {
            // Some a-b pruning can take place here
            // Send the slave a job
            int nodeId = jobQueue.front();
            jobQueue.pop();
            totalSendTime += sendJob(nodes[nodeId], nodeId, slaveId, getDepth(nodeId));
        }
        else // Otherwise, tell the slave it won't be getting more work and get stats from it
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            before = timeNow();
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
            after = timeNow();
            totalSendTime += nsBetween(before, after);
        }
    }

    before = timeNow();
    vector<valueMove> rootOrderedMoves;
    // Go through the node list, updating parent nodes' scores
    for (int nodeId = nodes.size() - 1; nodeId > 0; nodeId--)
    {
        stateNode currentNode = nodes[nodeId];
        if (currentNode.bestScore > nodes[currentNode.parentIndex].bestScore)
        {
            nodes[currentNode.parentIndex].bestScore = currentNode.bestScore;
        }
        if (currentNode.parentIndex == 0) // This is a lvl 1 node
        {
            valueMove mv;
            mv.move = currentNode.generatingMove;
            mv.value = currentNode.bestScore;
            rootOrderedMoves.push_back(mv);
        }
    }

    // What is our best move?
    sort(rootOrderedMoves.begin(), rootOrderedMoves.end(), [](const valueMove &left, const valueMove &right) {
        return left.value > right.value; // Sort in descending order
    });
    after = timeNow();
    scorePropagationTime = nsBetween(before, after);

    // Time the whole function
    masterEnd = timeNow();
    totalMasterTime = nsBetween(masterStart, masterEnd);

    writeStatsToFile(jobStats);

    cout << "---------" << endl;
    cout << "Master spent " << totalSendTime << " ns sending data" << endl;
    cout << "Master spent " << totalRecvTime << " ns receiving data" << endl;
    cout << "Master spent " << nodeGenerationTime << " ns generating nodes" << endl;
    cout << "Master spent " << scorePropagationTime << " ns propagating scores" << endl;
    cout << "Master spent " << totalMasterTime << " ns in total" << endl;
    cout << "Sum of subtimes: " << totalSendTime + totalRecvTime + nodeGenerationTime + scorePropagationTime << " ns" << endl;
    
    outputStats(jobStats, totalMasterTime);
    
    cout << "Root moves: " << endl;
    for (valueMove mv : rootOrderedMoves)
    {
        cout << (char)(mv.move.x + 'a') << mv.move.y + 1 << " with a score of " << mv.value << endl;
    }
}

void generateNodes(board initState, int minJobs, vector<stateNode> &nodes, queue<int> &frontier)
{
    nodes.empty();
    frontier.empty();

    stateNode root;
    root.state = initState;
    root.parentIndex = -1;
    root.bestScore = INT_MIN;
    root.generatingMove = {-1, -1};
    root.isMaxNode = true;

    nodes.push_back(root);
    frontier.push(nodes.size() - 1);

    bool noMovesForPrevNode = false;
    int firstWithNoMoves = -1;
    while (frontier.size() < minJobs)
    {
        int currentIdx = frontier.front();
        vector<gameMove> nextMoves = getMoves(nodes[currentIdx].state, nodes[currentIdx].isMaxNode);
        if (nextMoves.size() == 0)
        {
            if (!noMovesForPrevNode)
            {
                noMovesForPrevNode = true;
                firstWithNoMoves = currentIdx;
                // Put this node at the back of the queue
                frontier.pop();
                frontier.push(currentIdx);
            }
            else if (firstWithNoMoves == currentIdx)
            {
                break; // None of the frontier nodes has children
            }
                
        }
        else
        {
            // Remove the current index and place its children in the queue
            frontier.pop();
            for (gameMove mv : nextMoves)
            {
                stateNode newNode;
                newNode.state = applyMove(nodes[currentIdx].state, mv, nodes[currentIdx].isMaxNode);
                newNode.parentIndex = currentIdx;
                newNode.bestScore = INT_MIN;
                newNode.generatingMove = mv;
                newNode.isMaxNode = !nodes[currentIdx].isMaxNode;

                nodes.push_back(newNode);
                frontier.push(nodes.size() - 1);
                
            }
        }
    }
}

long long sendJob(stateNode node, int jobId, int slaveId, int nodeDepth)
{
    timePoint before, after;
    before = timeNow();
    long long totalTime = 0;
    // Tell the slave it has more work
    short workFlag = FLAG_MORE_JOBS_TRUE;
    MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);

    // Send job ID
    int jIdCopy = jobId;
    MPI_Send(&jIdCopy, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);

    // Send the board to the slave
    MPI_Send(&node.state.front(), _N * _M, MPI_CHAR, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);

    // Send the MAX turn flag
    int flag = (int)node.isMaxNode;
    MPI_Send(&flag, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);

    // Send the depth
    MPI_Send(&nodeDepth, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);
    
    // Time
    after = timeNow();
    totalTime += nsBetween(before, after);

    return totalTime;
}

jobResult receiveResult(int &slaveId)
{
    MPI_Status status;
    // Receive from any sender
    int resArray[2];
    MPI_Recv(resArray, 2, MPI_INT, MPI_ANY_SOURCE, Tags::SEARCH_JOB_RESULT, MPI_COMM_WORLD, &status);

    // Query the status for the sender ID
    slaveId = status.MPI_SOURCE;

    jobResult res;
    res.jobId = resArray[0];
    res.score = resArray[1];

    return res;
}

/*
********* SLAVE FUNCTIONS *********
*/

void slaveMain(int masterId, int slaveId)
{
    long long totalRecvTime = 0;
    long long totalSendTime = 0;
    long long jobTime = 0;
    timePoint before, after;

    while (true)
    {
        // Will I be receiving a job?
        short willGetJob;
        timePoint beforeRecv = timeNow();
        MPI_Recv(&willGetJob, 1, MPI_SHORT, masterId, Tags::MORE_JOBS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        timePoint afterRecv = timeNow();
        totalRecvTime += nsBetween(beforeRecv, afterRecv);

        // If not, send stats and I am done
        if (willGetJob == FLAG_MORE_JOBS_FALSE)
        {
            break;
        }

        // Else, receive job
        searchJob currentJob;
        totalRecvTime += receiveJob(currentJob, masterId);
        // cout << "Slave " << slaveId << " got job " << currentJob.id << " to evaluate for MAX: " << currentJob.isMaxTurn << " with board " << endl
        //      << printBoard(currentJob.state, _parameters.black) << endl;

        // Do the job
        before = timeNow();
        jobResult result = doJob(currentJob);
        after = timeNow();
        jobTime = nsBetween(before, after);

        // Send the result back to master
        before = timeNow();
        sendResult(result, masterId);
        after = timeNow();
        totalSendTime += nsBetween(before, after);

        // Send stats
        slaveStats stats;
        stats.sendTime = totalSendTime;
        stats.receiveTime = totalRecvTime;
        stats.jobTime = jobTime;
        stats.boardsEvaluated = _boardsEvaluated;
        stats.nodesPruned = _nodesPruned;
        stats.estMaxDepthPruned = _estMaxDepthPruned;
        stats.maxDepthReached = _maxDepthReached;
        stats.entireSpace = _entireSpaceCovered;
        MPI_Send(&stats, sizeof(stats), MPI_CHAR, masterId, Tags::SEARCH_JOB_STATS, MPI_COMM_WORLD);
    }
}

long long receiveJob(searchJob &job, int masterId)
{
    timePoint recvStart = timeNow();
    // Get the job ID
    MPI_Recv(&job.id, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Make a new board
    job.state = board(_M * _N);

    // Get the board
    MPI_Recv(&job.state.front(), _N * _M, MPI_CHAR, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Receive the MAX turn flag
    int flag;
    MPI_Recv(&flag, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    job.isMaxTurn = (bool)flag;

    // Receive the node depth
    MPI_Recv(&job.depth, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Time
    timePoint recvEnd = timeNow();
    return nsBetween(recvStart, recvEnd);
}

jobResult doJob(searchJob job)
{
    jobResult result;
    result.jobId = job.id;
    result.score = slaveSearch(job.state, _parameters.maxDepth, job.isMaxTurn, job.depth);
    return result;
}

void sendResult(jobResult result, int masterId)
{
    // Put contents in an array since we want them to be sent as one message
    int resArray[2];
    resArray[0] = result.jobId;
    resArray[1] = result.score;

    // Send the array
    MPI_Send(resArray, 2, MPI_INT, masterId, Tags::SEARCH_JOB_RESULT, MPI_COMM_WORLD);
}
