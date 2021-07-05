#pragma once
#include "logManager.h"
#include "indexManager.h"
#include <stdint.h>
#include <queue>
#include <set>

using namespace std;


/* Used in undo */
/* Used for deciding next undo trx */
struct MaxQNode{
    trxTable* trxTablePtr;
    uint64_t nextUndoSeqNo;
};

/* Used making max priority_queue */
bool operator < (MaxQNode x, MaxQNode y);

class RecoveryManager{
private:
    /* Used for recovery, contain log */
    char buf[SIZE_LOG_BUF];
    int bufOffset = 0;

    /* Log file fd */
    int logFileFd = -1;

    /* Log message file's file pointer */
    FILE* fp_log_message_file;
    
    /* bufPin is the offset of the file corresponding to start of the log buffer */
    /* startLSN is the LSN corresponding to start log's LSN in the log buffer  */
    int bufPin = 0;
    int startLSN = 0;

    /* For artificial crash */
    int checkedLogNum = 0;

    /* For choosing next undo trx */
    priority_queue<MaxQNode> maxPq;

    /* Winner, Loser */
    set<int> winnerSet;
    set<int> loserSet;

    /* Largest trx id */
    int maxTrxId = 0;

    /* Table id of target file */
    stack<int> idStack;
    bool idList[MAX_TABLE_NUM];

    /* Meta log */
    MetaLog* metaLog;
public:
    /* Initialize */
    /* Assume that log file is already open */
    /* So log manager must be initailized before recovery manager created */
    RecoveryManager(int logFileFd, char* log_message_fileName, MetaLog* metaLog);

    /* Raise new logs */
    void rasiseNewLog();

    /* Decode log */
    uint64_t get_seqNo();

    /* Analysis */
    void analysis();

    /* Redo */
    void redo(int log_num);
    void redoUpdateLog(updateLog* update_log);
    void redoCompensateLog(compensateLog* compensate_Log);

    /* Undo */
    void undo(int log_num);
    void make_priority_queue_about_loser();

    /* Increase trx id when meet the begin log */
    int get_maxTrxId();

    /* Get target file's id */
    stack<int> get_table_id_stack();
};