#include "stdafx.h"
#include "general.h"
#include "board.h"

#define TAG_SEARCH_JOB 1

#define MIN_LOAD_FACTOR 9

struct stateNode
{
    board state;
    int parentIndex;
    int bestScore;
    gameMove generatingMove; // What move led to this state
    bool isMaxNode;
};

// Holds an instance of the initial information sent to a slave
struct searchJob
{
    int id;
    board state;
    bool isMaxTurn;
};

struct jobResult
{
    int jobId;
    int score;
};

/*
********* MASTER FUNCTIONS *********
*/

void masterMain(board initState, int slaveCount);

/*
BFS to generate nodes
- The last <minJobs> nodes go into the frontier - they will be turned into jobs and sent to slaves
*/ 
void generateNodes(board initState, int minJobs, vector<stateNode> &nodes, queue<int>&frontier);

// Send a job to process <slaveId>
void sendJob(stateNode job, int jobId, int slaveId);

/*
- Receive job results from a slave
- Store the ID of the slave that we received from in slaveId
*/
jobResult receiveResult(int &slaveId);


/*
********* SLAVE FUNCTIONS *********
*/

void slaveMain(int masterId, int slaveId);

// Receive a job from process <masterId>
searchJob receiveJob(int masterId);

// Perfrom a job
jobResult doJob(searchJob job);

// Send results back
void sendResult(jobResult result, int masterId);
