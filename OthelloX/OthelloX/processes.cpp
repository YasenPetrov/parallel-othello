#include "processes.h"
#include "search.h"

#define FLAG_MORE_JOBS_TRUE 1
#define FLAG_MORE_JOBS_FALSE 0

enum Tags
{
    MORE_JOBS,
    SEARCH_JOB,
    SEARCH_JOB_RESULT
};

/*
********* MASTER FUNCTIONS *********
*/

void masterMain(board initState, int slaveCount)
{
    // Generate some nodes
    vector<stateNode> nodes;
    queue<int> jobQueue;
    generateNodes(initState, slaveCount * MIN_LOAD_FACTOR, nodes, jobQueue);
    int jobsToComplete = jobQueue.size(); // How many jobs do we have in the pool at the start
    
    if(nodes.size() == 1) // We only have the root - we have no possible moves
    {
        cout << "{ na }";
        // Tell slaves there is no work to do
        for(int slaveId = 0; slaveId < slaveCount; slaveId++)
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
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

            sendJob(nodes[nodeId], nodeId, slaveId);
        }
        else // If somehow the number of slaves is greater than the pool size, we tell the other slaves there is no work for them
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
        }
    }

    // While we haven't received results for all jobs
    while (jobsToComplete > 0)
    {
        // Wait for a slave to signal it's done, get the result from it
        int slaveId;
        jobResult result = receiveResult(slaveId);
        jobsToComplete--;
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
            sendJob(nodes[nodeId], nodeId, slaveId);
        }
        else // Otherwise, tell the slave it won't be getting more work
        {
            short workFlag = FLAG_MORE_JOBS_FALSE;
            MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);
        }
    }

    // cout << "Out of jobs" << endl;

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

    rootOrderedMoves[0].value = 1;
    // What is our best move?
    sort(rootOrderedMoves.begin(), rootOrderedMoves.end(), [](const valueMove &left, const valueMove &right)
	{
		return left.value > right.value; // Sort in descending order
	});
    // cout << "Root moves: " << endl;
    for(valueMove mv : rootOrderedMoves)
    {
    // cout << (char)(mv.move.x + 'a') << mv.move.y + 1 << " with a score of " << mv.value << endl;
        
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
            }
            else if (firstWithNoMoves == currentIdx)
                break; // None of the frontier nodes has children
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

void sendJob(stateNode node, int jobId, int slaveId)
{
    // Tell the slave it has more work
    short workFlag = FLAG_MORE_JOBS_TRUE;
    MPI_Send(&workFlag, 1, MPI_SHORT, slaveId, Tags::MORE_JOBS, MPI_COMM_WORLD);

    // Send job ID
    int jIdCopy = jobId;
    MPI_Send(&jIdCopy, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);

    // Send board dimensions to the slave
    int M = node.state.size();
    int N = node.state[0].size();
    MPI_Send(&M, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);
    MPI_Send(&N, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);

    // Send each row of the board to the slave
    for (int row = 0; row < M; row++)
    {
        MPI_Send(&node.state[row].front(), N, MPI_CHAR, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);
    }

    // Send the MAX turn flag
    int flag = (int)node.isMaxNode;
    MPI_Send(&flag, 1, MPI_INT, slaveId, Tags::SEARCH_JOB, MPI_COMM_WORLD);
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
    while (true)
    {
        // Will I be receiving a job?
        short willGetJob;
        MPI_Recv(&willGetJob, 1, MPI_SHORT, masterId, Tags::MORE_JOBS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        // If not, I'm done
        if (willGetJob == FLAG_MORE_JOBS_FALSE)
            break;

        // Else, receive job
        searchJob currentJob = receiveJob(masterId);
        // cout << "Slave " << slaveId << " got job " << currentJob.id << " to evaluate for MAX: " << currentJob.isMaxTurn << " with board " << endl
        //      << printBoard(currentJob.state, _parameters.black) << endl;

        // Do the job
        jobResult result = doJob(currentJob);

        // Send the result back to master
        sendResult(result, masterId);
    }
}

searchJob receiveJob(int masterId)
{
    searchJob result;

    // Get the job ID
    MPI_Recv(&result.id, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Get the board dimensions
    int M, N;
    MPI_Recv(&M, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&N, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Make a new board
    result.state = board(M, row(N));

    signed char rowBuf[N];
    // Get each row and fill the board
    for (int rowId = 0; rowId < M; rowId++)
    {
        MPI_Recv(rowBuf, N, MPI_CHAR, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int colId = 0; colId < N; colId++)
        {
            result.state[rowId][colId] = rowBuf[colId];
        }
    }

    // Receive the MAX turn flag
    int flag;
    MPI_Recv(&flag, 1, MPI_INT, masterId, Tags::SEARCH_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    result.isMaxTurn = (bool)flag;

    return result;
}

jobResult doJob(searchJob job)
{
    jobResult result;
    result.jobId = job.id;

    result.score = slaveSearch(job.state, _parameters.maxDepth, job.isMaxTurn);

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