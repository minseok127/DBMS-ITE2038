#include "../include/logManager.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

pthread_mutex_t log_manager_latch = PTHREAD_MUTEX_INITIALIZER;

LogManager* log_manager;

LogManager::LogManager(char* path){
    logFileFd = open(path, O_RDWR | O_CREAT, 00700);
    trxTableHash = new trxTable[SIZE_TRX_TABLE_HASH];
    metaLog = new MetaLog;
    
    /* Initailize log buffer to 0x00 */
    memset(buf, 0x00, SIZE_LOG_BUF);
    
    /* Check log file's start lsn */
    /* The first 12bytes of the log file(metaLog) represent the largest LSN and the largest trx id of the last successful recovering */
    /* If recovering has not happend yet, metaLSN and metaTrxId is 0 */
    /* After 12bytes, log data start */
    lseek(logFileFd, 0, SEEK_SET);
    int readNum = read(logFileFd, metaLog, sizeof(MetaLog));
    
    /* Set tail of log buffer. */
    /* The tail of file is determined by how far the tail of buffer is applied to the file */
    if (readNum == 0){
        /* Log file is empty */
        tailBuf = 0;
        tailFile = tailBuf;

        /* Initialize */
        metaLog->metaLSN = 0;
        metaLog->metaTrxId = 0;
    }
    else{
        /* New logs will be published bigger than metaLSN */    
        tailBuf = metaLog->metaLSN;
        tailFile = tailBuf;
    }

}

LogManager::~LogManager(){
    trxTable* del;

	for(int i = 0; i < SIZE_TRX_TABLE_HASH; i++){
		trxTable* c = trxTableHash + i;
		c = c->next;

		while(c != NULL){
			del = c;
			c = c->next;
			delete del;
		}
	}
    delete[] trxTableHash;
}

MetaLog* LogManager::get_metaLog(){
    return metaLog;
}

int LogManager::get_logFileFd(){
    return logFileFd;
}

uint64_t LogManager::publish_beginLog(int trx_id){
    basicLog* new_log = new basicLog;
    

    /* Set new log information */
    new_log->trx_id = trx_id;
    new_log->type = BEGIN_T;
    new_log->prevSeqNo = 0;
    new_log->size = SIZE_BASIC_LOG;

    pthread_mutex_lock(&log_manager_latch);
    /* Make new empty trx table */
    set_trxTable(trx_id);
    new_log->seqNo = tailBuf + SIZE_BASIC_LOG;
    /* Update trx table */
    update_trxTable(new_log->trx_id, new_log->seqNo, new_log->type);
   
    /* Write into log buffer */
    if (is_stuck(SIZE_BASIC_LOG)){
        bufFlush();
    }
    int bufOffset = tailBuf - tailFile;
    memcpy(buf + bufOffset, new_log, SIZE_BASIC_LOG);
    move_tailBuf(SIZE_BASIC_LOG);
    pthread_mutex_unlock(&log_manager_latch);
    
    /* Return new_log's sequence number */
    uint64_t ret = new_log->seqNo;
    /* delete new_log, because new_log's information is copied into log buffer */
    delete new_log;

    return ret;

}

uint64_t LogManager::publish_commitLog(int trx_id){
    basicLog* new_log = new basicLog;

    /* Set new log information */
    new_log->trx_id = trx_id;
    new_log->type = COMMIT_T;
    new_log->size = SIZE_BASIC_LOG;

    pthread_mutex_lock(&log_manager_latch);
    new_log->prevSeqNo = get_trxTable(trx_id)->lastSeqNo;
    new_log->seqNo = tailBuf + SIZE_BASIC_LOG;
    /* Update trx table */
    update_trxTable(new_log->trx_id, new_log->seqNo, new_log->type);
    
    /* Write into log buffer */
    if (is_stuck(SIZE_BASIC_LOG)){
        bufFlush();
    }
    int bufOffset = tailBuf - tailFile;
    memcpy(buf + bufOffset, new_log, SIZE_BASIC_LOG);
    move_tailBuf(SIZE_BASIC_LOG);
    bufFlush();
    pthread_mutex_unlock(&log_manager_latch);

    /* Return new_log's sequence number */
    uint64_t ret = new_log->seqNo;
    /* delete new_log, because new_log's information is copied into log buffer */
    delete new_log;

    return ret;
}

uint64_t LogManager::publish_rollbackLog(int trx_id){
    basicLog* new_log = new basicLog;

    /* Set new log information */
    new_log->trx_id = trx_id;
    new_log->type = ROLLBACK_T;
    new_log->size = SIZE_BASIC_LOG;

    pthread_mutex_lock(&log_manager_latch);
    new_log->prevSeqNo = get_trxTable(trx_id)->lastSeqNo;
    new_log->seqNo = tailBuf + SIZE_BASIC_LOG;
    /* Update trx table */
    update_trxTable(new_log->trx_id, new_log->seqNo, new_log->type);
    
    /* Write into log buffer */
    if (is_stuck(SIZE_BASIC_LOG)){
        bufFlush();
    }
    int bufOffset = tailBuf - tailFile;
    memcpy(buf + bufOffset, new_log, SIZE_BASIC_LOG);
    move_tailBuf(SIZE_BASIC_LOG);
    pthread_mutex_unlock(&log_manager_latch);

    /* Return new_log's sequence number */
    uint64_t ret = new_log->seqNo;
    /* delete new_log, because new_log's information is copied into log buffer */
    delete new_log;

    return ret;
}

uint64_t LogManager::publish_updateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo){
    updateLog* new_log = new updateLog;

    /* Set new log information */
    new_log->trx_id = trx_id;
    new_log->type = UPDATE_T;

    new_log->table_id = table_id;
    new_log->pagenum = pagenum;
    new_log->offset = offset;
    new_log->dataLength = SIZE_RECORD;

    memcpy(new_log->undo, undo, 120);
    memcpy(new_log->redo, redo, 120);

    new_log->size = SIZE_UPDATE_LOG;
    pthread_mutex_lock(&log_manager_latch);
    new_log->prevSeqNo = get_trxTable(trx_id)->lastSeqNo;
    new_log->seqNo = tailBuf + SIZE_UPDATE_LOG;
    /* Update trx table */
    update_trxTable(new_log->trx_id, new_log->seqNo, new_log->type);
    
    /* Write into log buffer */
    if (is_stuck(SIZE_UPDATE_LOG)){
        bufFlush();
    }
    int bufOffset = tailBuf - tailFile;
    memcpy(buf + bufOffset, new_log, SIZE_UPDATE_LOG);
    move_tailBuf(SIZE_UPDATE_LOG);
    pthread_mutex_unlock(&log_manager_latch);

    /* Return new_log's sequence number */
    uint64_t ret = new_log->seqNo;
    /* delete new_log, because new_log's information is copied into log buffer */
    delete new_log;

    return ret;
}

uint64_t LogManager::publish_compensateLog(int trx_id, int table_id, uint64_t pagenum, int offset, char* undo, char* redo){
    compensateLog* new_log = new compensateLog;

    /* Set new log information */
    new_log->trx_id = trx_id;
    new_log->type = COMPENSATE_T;

    new_log->table_id = table_id;
    new_log->pagenum = pagenum;
    new_log->offset = offset;
    new_log->dataLength = SIZE_RECORD;

    memcpy(new_log->undo, undo, 120);
    memcpy(new_log->redo, redo, 120);

    new_log->size = SIZE_COMPENSATE_LOG;

    pthread_mutex_lock(&log_manager_latch);
    trxTable* targetTrxTable = get_trxTable(trx_id);
    new_log->prevSeqNo = targetTrxTable->lastSeqNo;
    new_log->seqNo = tailBuf + SIZE_COMPENSATE_LOG;
    /* Update trx table */
    update_trxTable(new_log->trx_id, new_log->seqNo, new_log->type);

    /* Set next undo LSN of new log */
    new_log->nextUndoSeqNo = targetTrxTable->nextUndoSeqNo;

    /* Write into log buffer */
    if (is_stuck(SIZE_COMPENSATE_LOG)){
        bufFlush();
    }
    int bufOffset = tailBuf - tailFile;
    memcpy(buf + bufOffset, new_log, SIZE_COMPENSATE_LOG);
    move_tailBuf(SIZE_COMPENSATE_LOG);
    pthread_mutex_unlock(&log_manager_latch);

    /* Return new_log's sequence number */
    uint64_t ret = new_log->seqNo;
    /* delete new_log, because new_log's information is copied into log buffer */
    delete new_log;

    return ret;
}

trxTable* LogManager::get_trxTable(int trx_id){
    int pos = trx_id % SIZE_TRX_TABLE_HASH;
    trxTable* target = trxTableHash + pos;

    while(target->next != NULL){
        target = target->next;
        if (target->trx_id == trx_id){
            return target;
        }
    }
    return NULL;
}

void LogManager::set_trxTable(int trx_id){
    trxTable* new_table = new trxTable;
    new_table->trx_id = trx_id;

    int pos = trx_id % SIZE_TRX_TABLE_HASH;
    trxTable* target = trxTableHash + pos;

    while(target->next != NULL){
        target = target->next;
    }
    target->next = new_table;
}

void LogManager::delete_trxTable(int trx_id){
    int pos = trx_id % SIZE_TRX_TABLE_HASH;
    trxTable* target = trxTableHash + pos;
    trxTable* prev = target;

    while(target->next != NULL){
        target = target->next;
        if (target->trx_id == trx_id){
            prev->next = target->next;
            delete target;
            break;
        }
        prev = target;
    }
}

void LogManager::update_trxTable(int trx_id, uint64_t newSeqNo, int type){
    trxTable* target = get_trxTable(trx_id);

    /* Set last sequence number */
    target->lastSeqNo = newSeqNo;

    /* If type is update, push new sequence number into stack */
    /* This will be used in publishing clr */
    if (type == UPDATE_T){
        target->updateSeqNoStack.push(newSeqNo);
    }

    /* Set next undo sequence number */
    if (type == COMPENSATE_T){
        target->updateSeqNoStack.pop();
        if (target->updateSeqNoStack.empty()){
            target->nextUndoSeqNo = 0;
        }
        else{
            target->nextUndoSeqNo = target->updateSeqNoStack.top();
        }
    }
    else if (type == UPDATE_T){
        target->nextUndoSeqNo = target->lastSeqNo;
    }
    else{
        target->nextUndoSeqNo = 0;
    }
}

bool LogManager::is_stuck(int size){
    int bufOffset = tailBuf - tailFile;

    if (bufOffset + size > SIZE_LOG_BUF){
        return true;
    }
    else{
        return false;
    }
}

void LogManager::bufFlush(){
    int bufOffset = tailBuf - tailFile;
    lseek(logFileFd, 0, SEEK_END);
	write(logFileFd, buf, bufOffset);
    tailFile = tailBuf;
}

void LogManager::move_tailBuf(int size){
    tailBuf += size;
}

void LogManager::sync_tails(){
    tailFile = tailBuf;
}

uint64_t LogManager::get_tailBuf(){
    return tailBuf;
}