#include "../include/recoveryManager.h"
#include "../include/bufferManager.h"
#include "../include/trxManager.h"
#include "../include/bpt.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

extern LogManager* log_manager;
extern trxManager* trx_manager;

RecoveryManager* recovery_manager;

RecoveryManager::RecoveryManager(int logFileFd, char* log_message_fileName, MetaLog* metaLog){
    /* Set log file's fd and initialize log buffer for recovery */
    this->logFileFd = logFileFd; 
    memset(buf, 0x00, SIZE_LOG_BUF);

    /* Open log message file with file pointer */
    fp_log_message_file = fopen(log_message_fileName, "w");

    for(int i = 0; i < MAX_TABLE_NUM; i++){
        idList[i] = false;
    }

    this->metaLog = metaLog;
    this->startLSN = metaLog->metaLSN;
    this->bufPin = sizeof(MetaLog);
}

void RecoveryManager::rasiseNewLog(){
    /* Raise log file into buffer of recovery manager */
    /* Log file's first 12bytes is MetaLog */
    lseek(logFileFd, bufPin, SEEK_SET);
	int realReadNum = read(logFileFd, buf, SIZE_LOG_BUF);

    /* Make empty area to 0 */
    if (realReadNum != SIZE_LOG_BUF){
        memset(buf + realReadNum, 0x00, SIZE_LOG_BUF - realReadNum);
    }
}

uint64_t RecoveryManager::get_seqNo(){
    /* Read 8byte for knowing log sequence number */
    uint64_t seqNo;
    memcpy(&seqNo, buf + bufOffset, sizeof(uint64_t));

    return seqNo;
}

int RecoveryManager::get_maxTrxId(){
    return maxTrxId;
}

stack<int> RecoveryManager::get_table_id_stack(){
    return idStack;
}

/******************* Analysis Pass *********************/

void RecoveryManager::analysis(){
    fprintf(fp_log_message_file, "[ANALYSIS] Analysis pass start\n");

    rasiseNewLog();

    bufOffset = 0;
    int seqNo = get_seqNo();
    int startOffset = log_manager->get_tailBuf();

    while (seqNo != 0){
        /* Parsing begin, commit, rollback logs */
        basicLog* basic_log = new basicLog;

        switch (seqNo - startOffset)
        {
        case SIZE_BASIC_LOG:
            memcpy(basic_log, buf + bufOffset, SIZE_BASIC_LOG);
            if (basic_log->type == BEGIN_T){
                /* If log type is begin, set new trx table. It will be used publishing new log */
                log_manager->set_trxTable(basic_log->trx_id);

                /* Put trx id into loser Set */
                loserSet.insert(basic_log->trx_id);
            }   
            else{
                /* If log tpye is commit or rollback, this trx will not publish new log. So delete trx table */
                log_manager->delete_trxTable(basic_log->trx_id);

                /* Move trx id from loser Set to winner Set */
                loserSet.erase(basic_log->trx_id);

                winnerSet.insert(basic_log->trx_id);
            }
            break;
        default:
            break;
        }

        /* Delete log instance. */ 
        delete basic_log;

        /* Set other information */
        bufOffset += (seqNo - startOffset);
        startOffset = seqNo;
        seqNo = get_seqNo();
        if (seqNo - startLSN >= SIZE_LOG_BUF){
            bufPin += startOffset - startLSN;
            startLSN = startOffset;
            rasiseNewLog();
            bufOffset = 0;
        }
    }

    /* Max trx id */
    int maxWinner = 0;
    int maxLoser = 0;

    set<int>::iterator iter;
    
    fprintf(fp_log_message_file, "[ANALYSIS] Analysis success. Winner:");

    for (iter = winnerSet.begin(); iter != winnerSet.end(); iter++){
        maxWinner = *iter;
        fprintf(fp_log_message_file, " %d", *iter);
    }

    fprintf(fp_log_message_file, ", Loser:");
    for (iter = loserSet.begin(); iter != loserSet.end(); iter++){
        maxLoser = *iter;
        fprintf(fp_log_message_file, " %d", *iter);
    }
    fprintf(fp_log_message_file, "\n");

    maxTrxId = maxWinner > maxLoser ? maxWinner : maxLoser;
}


/******************* Redo Pass *************************/

void RecoveryManager::redo(int log_num){
    fprintf(fp_log_message_file, "[REDO] Redo pass start\n");

    /* If first phase does not in buffer, raise it */ 
    if (bufPin != sizeof(MetaLog)){
        bufPin = sizeof(MetaLog);
        rasiseNewLog();

        startLSN = metaLog->metaLSN;
    }

    bufOffset = 0;
    int seqNo = get_seqNo();
    int startOffset = log_manager->get_tailBuf();

    while (seqNo != 0){
        basicLog* basic_log = new basicLog;
        updateLog* update_log = new updateLog;
        compensateLog* compensate_log = new compensateLog;

        /* Distinguish log type */
        int type;

        switch (seqNo - startOffset)
        {
        case SIZE_UPDATE_LOG:
            /* Copy data in log buffer to the log instance and distinguish log type*/
            memcpy(update_log, buf + bufOffset, SIZE_UPDATE_LOG);
            type = UPDATE_T;
            /* Move log manager's buffer tail for publishing new log at undo pass */
            log_manager->move_tailBuf(SIZE_UPDATE_LOG);
            /* Synchronize tail of log buffer and tail of log file */
            /* After crash, log manager's log buffer is empty(not recovery manager's buffer). This case is tailBuf = tailFIle */
            log_manager->sync_tails();
            /* If target file is not open */
            if (!idList[update_log->table_id - 1]){
                /* Open for recovery */
                char path[20];
                sprintf(path, "DATA%d", update_log->table_id);
                open_table(path);
                idList[update_log->table_id - 1] = true;
                idStack.push(update_log->table_id);
            }
            break;
        case SIZE_COMPENSATE_LOG:
            /* Copy data in log buffer to the log instance and distinguish log type*/
            memcpy(compensate_log, buf + bufOffset, SIZE_COMPENSATE_LOG);
            type = COMPENSATE_T;
            /* Move log manager's buffer tail for publishing new log at undo pass */
            log_manager->move_tailBuf(SIZE_COMPENSATE_LOG);
            /* Synchronize tail of log buffer and tail of log file */
            /* After crash, log manager's log buffer is empty(not recovery manager's buffer). This case is tailBuf = tailFIle */
            log_manager->sync_tails();
            break;
        default:
            /* Copy data in log buffer to the log instance and distinguish log type*/
            memcpy(basic_log, buf + bufOffset, SIZE_BASIC_LOG);
            /* Move log manager's buffer tail for publishing new log at undo pass */
            log_manager->move_tailBuf(SIZE_BASIC_LOG);
            /* Synchronize tail of log buffer and tail of log file */
            /* After crash, log manager's log buffer is empty(not recovery manager's buffer). This case is tailBuf = tailFIle */
            log_manager->sync_tails();
            if (basic_log->type == BEGIN_T){
                type = BEGIN_T;
            }
            else if (basic_log->type == COMMIT_T){
                type = COMMIT_T;
            }
            else{
                type = ROLLBACK_T;
            }
            break;
        }

        /* Redo about target log */
        switch (type)
        {
        case UPDATE_T:
            redoUpdateLog(update_log);
            break;
        case COMPENSATE_T:
            redoCompensateLog(compensate_log);
            break;
        case BEGIN_T:
            fprintf(fp_log_message_file, "LSN %lu [BEGIN] Transaction id %d\n", basic_log->seqNo, basic_log->trx_id);
            break;
        case COMMIT_T:
            fprintf(fp_log_message_file, "LSN %lu [COMMIT] Transaction id %d\n", basic_log->seqNo, basic_log->trx_id);
            break;
        case ROLLBACK_T:
            fprintf(fp_log_message_file, "LSN %lu [ROLLBACK] Transaction id %d\n", basic_log->seqNo, basic_log->trx_id);
            break;
        }

        /* Delete log instance */
        delete basic_log;
        delete update_log;
        delete compensate_log;

        /* Set next log */
        /* If buffer is full, raise new logs */
        /* Set other information */
        bufOffset += (seqNo - startOffset);
        startOffset = seqNo;
        seqNo = get_seqNo();
        if (seqNo - startLSN >= SIZE_LOG_BUF){
            bufPin += startOffset - startLSN;
            startLSN = startOffset;
            rasiseNewLog();
            bufOffset = 0;
        }

        checkedLogNum++;
        if (log_num != 0 && log_num == checkedLogNum){
            fclose(fp_log_message_file);
            return;
        }
    }

    fprintf(fp_log_message_file, "[REDO] Redo pass end\n");
}

void RecoveryManager::redoUpdateLog(updateLog* update_log){
    trxTable* targetTable = log_manager->get_trxTable(update_log->trx_id);

    /* If update_log's trx table not exists, this trx is commited or rollbacked. So updating trx table not required */
    /* If update_logs' trx table exists, updateing is required. Because it must publish compensate log later */
    if (targetTable != NULL){
        log_manager->update_trxTable(update_log->trx_id, update_log->seqNo, update_log->type);
    }

    int pagenum = update_log->pagenum;
    int table_id = update_log->table_id;

    LeafPage leafPage;
    buffer_read_page(table_id, pagenum, &leafPage);

    /* Consider redo */
    /* If leafPage's pageLsn >= log'seqNo, it does not have to redo */
    if (leafPage.pageLSN < update_log->seqNo){
        /* Redo */
        int i = (update_log->offset - PAGE_HEADER_SIZE - sizeof(int64_t))/sizeof(Record);
        memcpy(&leafPage.record[i].value, update_log->redo, update_log->dataLength);
        /* Update last update log sequence number */
        leafPage.pageLSN = update_log->seqNo;

        buffer_write_page(table_id, pagenum, &leafPage);
        
        fprintf(fp_log_message_file, "LSN %lu [UPDATE] Transaction id %d redo apply\n", update_log->seqNo, update_log->trx_id);

        return;
    }
    else{
        /* Consider redo */
        buffer_complete_read_without_write(table_id, pagenum);

        fprintf(fp_log_message_file, "LSN %lu [CONSIDER-REDO] Transaction id %d\n", update_log->seqNo, update_log->trx_id);
    }

}

void RecoveryManager::redoCompensateLog(compensateLog* compensate_log){
    trxTable* targetTable = log_manager->get_trxTable(compensate_log->trx_id);

    /* If update_log's trx table not exists, this trx is commited or rollbacked. So updating trx table not required */
    /* If update_logs' trx table exists, updateing is required. Because it must publish compensate log later */
    if (targetTable != NULL){
        log_manager->update_trxTable(compensate_log->trx_id, compensate_log->seqNo, compensate_log->type);
    }

    int pagenum = compensate_log->pagenum;
    int table_id = compensate_log->table_id;

    LeafPage leafPage;
    buffer_read_page(table_id, pagenum, &leafPage);

    /* Consider redo */
    /* If leafPage's pageLsn >= log'seqNo, it does not have to redo */
    if (leafPage.pageLSN < compensate_log->seqNo){
        /* Redo */
        int i = (compensate_log->offset - PAGE_HEADER_SIZE - sizeof(int64_t))/sizeof(Record);
        memcpy(&leafPage.record[i].value, compensate_log->redo, compensate_log->dataLength);
        /* Update last update log sequence number */
        leafPage.pageLSN = compensate_log->seqNo;

        buffer_write_page(table_id, pagenum, &leafPage);

        fprintf(fp_log_message_file, "LSN %lu [CLR] next undo lsn %lu\n", compensate_log->seqNo, compensate_log->nextUndoSeqNo);
        
        return;
    }
    else{
        /* Consider redo */
        buffer_complete_read_without_write(table_id, pagenum);

        fprintf(fp_log_message_file, "LSN %lu [CONSIDER-REDO] Transaction id %d\n", compensate_log->seqNo, compensate_log->trx_id);
    }

}

/********************** Undo Pass **********************************/

/* For compare MaxQNode. it will be used max priority queue about nextUndoSeqNo */
bool operator < (MaxQNode x, MaxQNode y){
    if (x.nextUndoSeqNo < y.nextUndoSeqNo){
        return true;
    }
    else{
        return false;
    }
}

void RecoveryManager::undo(int log_num){
    fprintf(fp_log_message_file, "[UNDO] Undo pass start\n");

    checkedLogNum = 0;
    
    /* make priority queue */
    make_priority_queue_about_loser();

    /* Undo */
    while (!(maxPq.empty())){
        /* Get trx info that has highest next undo number */
        MaxQNode qNode = maxPq.top();
        maxPq.pop();

        int size;

        /* Set buffer for next undo number */
        /* LSN is end offset of log and LSN is located in start of log. */
        /* It makes analysis and redo to know the size of log immediately. */
        /* But undo read from end of log. So it must read first about size of log */
        if (qNode.nextUndoSeqNo - sizeof(int) < startLSN){
            /* If can't read size of log(Out of buffer) */
            /* Raise new buffer */
            /* New buffer's end is qNode.nextUndoSeqNo */
            if (qNode.nextUndoSeqNo - SIZE_LOG_BUF < metaLog->metaLSN){
                /* If new buffer's start is less than metaLSN, restrict start point to metaLSN */
                startLSN = metaLog->metaLSN;
            }
            else{
                startLSN = qNode.nextUndoSeqNo - SIZE_LOG_BUF;
            }
            bufPin = startLSN - metaLog->metaLSN + sizeof(MetaLog);
            rasiseNewLog();

            /* Read size of next undo log */
            memcpy(&size, buf + (qNode.nextUndoSeqNo - startLSN - sizeof(int)), sizeof(int));
        }
        else{
            /* If can read size of log */
            /* Read size of next undo log */
            memcpy(&size, buf + (qNode.nextUndoSeqNo - startLSN - sizeof(int)), sizeof(int));

            /* If log is truncated by start of buffer, rasie new buffer */
            /* New buffer's end is qNode.nextUndoSeqNo */
            if (qNode.nextUndoSeqNo - size < startLSN){
                if (qNode.nextUndoSeqNo - SIZE_LOG_BUF < metaLog->metaLSN){
                    /* If new buffer's start is less than metaLSN, restrict start point to metaLSN */
                    startLSN = metaLog->metaLSN;
                }
                else{
                    startLSN = qNode.nextUndoSeqNo - SIZE_LOG_BUF;
                }
                bufPin = startLSN - metaLog->metaLSN + sizeof(MetaLog);
                rasiseNewLog();
            }
        }

        /* Set start offset of undoing update log */
        /* Undoing log is update log */
        int startOffset = (qNode.nextUndoSeqNo - startLSN - size);

        trxTable* targetTable = qNode.trxTablePtr;
        updateLog* update_log = new updateLog;

        /* Copy to update_log */
        memcpy(update_log, buf + startOffset, size);

        /* Publish compensate log about undo */
        uint64_t publishedSeqNo = log_manager->publish_compensateLog(update_log->trx_id, update_log->table_id, update_log->pagenum, update_log->offset, update_log->redo, update_log->undo);
        // update_log->redo will be compensate_log's undo. update_log->undo will be compensate_log's redo.
        // trx table will be updated in publish_compensateLog()
        // Recovery is working in single thread, mutex does not required

        LeafPage leafPage;
        buffer_read_page(update_log->table_id, update_log->pagenum, &leafPage);

        /* Consider undo */
        if (leafPage.pageLSN >= update_log->seqNo){
            /* Undo record of target page */
            int i = (update_log->offset - PAGE_HEADER_SIZE - sizeof(int64_t))/sizeof(Record);
            memcpy(leafPage.record[i].value, update_log->undo, update_log->dataLength);
            
            /* Update last update log sequence number */
            /* targetTable's last sequence number is compensate log */ 
            leafPage.pageLSN = publishedSeqNo;

            buffer_write_page(update_log->table_id, update_log->pagenum, &leafPage);

            fprintf(fp_log_message_file, "LSN %lu [UPDATE] Transaction id %d undo apply\n", update_log->seqNo, update_log->trx_id);
        }
        else{
            /* Consider undo */
            buffer_complete_read_without_write(update_log->table_id, update_log->pagenum);

            fprintf(fp_log_message_file, "LSN %lu [CONSIDER-UNDO] Transaction id %d\n", update_log->seqNo, update_log->trx_id);
        }

        /* If trx does not have update log, publish rollback log */
        /* Else update qNode with new next undo number, push into priority queue again */
        if (targetTable->nextUndoSeqNo == 0){
            log_manager->publish_rollbackLog(update_log->trx_id);
        }
        else{
            qNode.nextUndoSeqNo = targetTable->nextUndoSeqNo;
            maxPq.push(qNode);  
        }

        /* Delete log instance */
        delete update_log;

        checkedLogNum++;
        if (log_num != 0 && checkedLogNum == log_num){
            fclose(fp_log_message_file);
            return;
        }
    }

    fprintf(fp_log_message_file, "[UNDO] Undo pass end\n");

    fclose(fp_log_message_file);

    /* Flush will occured in last of init_db() */
}

void RecoveryManager::make_priority_queue_about_loser(){
    trxTable* targetTable;
    
    /* Push all losers into priority queue */
    set<int>::iterator iter;
    for(iter = loserSet.begin(); iter != loserSet.end(); iter++){
        targetTable = log_manager->get_trxTable(*iter);

        MaxQNode new_qNode;
        new_qNode.nextUndoSeqNo = targetTable->nextUndoSeqNo;
        new_qNode.trxTablePtr = targetTable;

        /* If next undo sequence number is 0, undo is not not required */
        if (new_qNode.nextUndoSeqNo != 0){
            maxPq.push(new_qNode);
        }
    }
}