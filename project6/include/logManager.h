#pragma once
#include <stdint.h>
#include <iostream>
#include <stack>

using namespace std;

/* Size of log buffer is 40 MB */
#define SIZE_LOG_BUF (1024 * 1024 * 40)

/* Size of hash in log manager */
#define SIZE_TRX_TABLE_HASH (1000)

/* Size of Record */
#define SIZE_RECORD (120)

/* Size of logs */
#define SIZE_BASIC_LOG (28)
#define SIZE_UPDATE_LOG (288)
#define SIZE_COMPENSATE_LOG (296)

/* Type of logs */
#define BEGIN_T (0)
#define UPDATE_T (1)
#define COMMIT_T (2)
#define ROLLBACK_T (3)
#define COMPENSATE_T (4)

/* Begin, Commit, Rollback */
#pragma pack(push, 4)
struct basicLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;
    int size;
};

/* Update */
struct updateLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;
    
    int table_id;
    uint64_t pagenum;
    int offset;
    int dataLength;
    char undo[120];
    char redo[120];

    int size;
};

/* Compensate */
struct compensateLog{
    uint64_t seqNo;
    uint64_t prevSeqNo;
    int trx_id;
    int type;

    int table_id;
    uint64_t pagenum;
    int offset;
    int dataLength;
    char undo[120];
    char redo[120];

    uint64_t nextUndoSeqNo;

    int size;
};
#pragma pack(pop)

/* Used in publishing new log */
struct trxTable{
    /* For Hash in LogManager */
    trxTable* next = NULL;
    int trx_id;

    /* For publish new log information about last sequence number */
    uint64_t lastSeqNo = 0;

    /* For making clr */
    stack<uint64_t> updateSeqNoStack;
    uint64_t nextUndoSeqNo = 0;
};

/* For durable LSN, trx id */
struct MetaLog{
    uint64_t metaLSN;
    int metaTrxId;
};

class LogManager{
private:
    /* Managing trxTable with hash */
    trxTable* trxTableHash = NULL;

    /* Log Buffer */
    char buf[SIZE_LOG_BUF];

    /* Maintain offset about buffer and file */
    uint64_t tailBuf = 0;
    uint64_t tailFile = 0;

    /* For manage log file */
    int logFileFd = -1;

    /* For durable LSN, trx id */
    MetaLog* metaLog;
public:
    /* Initialize - destroy */
    LogManager(char* logfile_path);
    ~LogManager();

    /* Get log file's fd */
    int get_logFileFd();

    /* Publish log */
    uint64_t publish_beginLog(int trx_id);
    uint64_t publish_commitLog(int trx_id);
    uint64_t publish_rollbackLog(int trx_id);
    uint64_t publish_updateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo);
    uint64_t publish_compensateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo);

    /* Manager offset */
    void move_tailBuf(int size);
    void sync_tails();
    uint64_t get_tailBuf();

    /* Get trx table */
    trxTable* get_trxTable(int trx_id);
    /* Set trx table */
    void set_trxTable(int trx_id);
    /* Delete trx table */
    void delete_trxTable(int trx_id);
    /* Update trx table */
    void update_trxTable(int trx_id, uint64_t newSeqNo, int type);

    /* Flush */
    bool is_stuck(int size);
    void bufFlush();

    /* Get meta log */
    MetaLog* get_metaLog();
};
