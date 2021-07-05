#include "../include/trxManager.h"
#include "../include/indexManager.h"
#include "../include/logManager.h"
#include "../include/bpt.h"
#include <pthread.h>
#include <string.h>
#include <queue>
#include <stack>
#include <time.h>

using namespace std;


pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;

trxManager* trx_manager;

extern pthread_mutex_t lock_table_latch;
extern pthread_mutex_t log_manager_latch;
extern pthread_mutex_t tree_latch;
extern TableManager* tableManager;
extern LogManager* log_manager;

struct idnode{
    int trx_id = 0;
    idnode* next = NULL;
};

class hash_pass_trx_id{
private:
    idnode* hash;
    int size = 1000;
public:
    hash_pass_trx_id();
    ~hash_pass_trx_id();
    void insert_id(int trx_id);
    bool is_in(int trx_id);
    int find_pos(int trx_id);
};

hash_pass_trx_id::hash_pass_trx_id(){
    hash = new idnode[size];
}

hash_pass_trx_id::~hash_pass_trx_id(){
    delete[] hash;
}

int hash_pass_trx_id::find_pos(int trx_id){
    return trx_id % size;
}

void hash_pass_trx_id::insert_id(int trx_id){
    int pos = trx_id % size;
    idnode* c = hash + pos;
    
    idnode* newid = new idnode;
    newid->next = NULL;
    newid->trx_id = trx_id;

    while(c->next != NULL){
        c = c->next;
    }
    c->next = newid;
}

bool hash_pass_trx_id::is_in(int trx_id){
    idnode* c = hash + (trx_id % size);
    c = c->next;
    while(c != NULL){
        if (c->trx_id == trx_id){
            return true;
        }
        c = c->next;
    }
    return false;
}

trxManager::trxManager(){
    next_trx_id = 1;
    hash = new trxNode[TRX_MANAGER_HASH_SIZE];
}
/*
trxManager::~trxManager(){
    trxNode* del;

	for(int i = 0; i < TRX_MANAGER_HASH_SIZE; i++){
		trxNode* c = hash + i;
		
		while(c != NULL){
			del = c;
			c = c->next;
            
            record_log* del2;
            for(int j = 0; j < LOG_HASH_SIZE; j++){
                record_log* d = del->hash + j;
                while(d != NULL){
                    del2 = d;
                    d = d->next;

                    delete del2;
                }
            }

			delete del;
		}
	}
}*/

int trxManager::find_pos(int num, int size){
    return num % size;
}

int trxManager::set_new_trxNode(){

    trxNode* new_trx = new trxNode;
    if (new_trx == NULL){
        return 0;
    }
    new_trx->next = NULL;
    new_trx->lock_conflict = NULL;
    new_trx->lock_head = NULL;
    new_trx->recordHead = NULL;
    new_trx->workinghash = new workingLock[WORKING_SIZE];
    new_trx->prev = NULL;
    new_trx->trx_id = next_trx_id++;

    int pos = new_trx->trx_id % TRX_MANAGER_HASH_SIZE;
    
    trxNode* c = hash+pos;
    while(c->next != NULL){
        c = c->next;
    }

    c->next = new_trx;
    new_trx->prev = c;
    return new_trx->trx_id;
}

trxNode* trxManager::get_trxNode(int trx_id){
    trxNode* retNode = NULL;
    int pos = trx_id % TRX_MANAGER_HASH_SIZE;

    trxNode* c = hash+pos;

    pthread_mutex_lock(&trx_manager_latch);
    while(c->next != NULL){
        c = c->next;
        if (c->trx_id == trx_id){
            retNode = c;
            break;
        }
    }
    pthread_mutex_unlock(&trx_manager_latch);

    return retNode;
}

int trxManager::delete_trxNode(trxNode* targetTrx){
    pthread_mutex_lock(&trx_manager_latch);
    targetTrx->prev->next = targetTrx->next;
    if (targetTrx->next != NULL){
        targetTrx->next->prev = targetTrx->prev;
    }
    pthread_mutex_unlock(&trx_manager_latch);

    int ret = targetTrx->trx_id;

    record_log* c = targetTrx->recordHead;
    while (c != NULL)
    {
        record_log* del = c;
        c = c->next;
        delete del;
    }
    

    for(int i = 0; i < WORKING_SIZE; i++){
        workingLock* h1 = (targetTrx->workinghash + i)->next;
        workingLock* del1;
        while(h1 != NULL){
            del1 = h1;
            h1 = h1->next;
            delete del1;
        }
    }

    delete[] targetTrx->workinghash;
    delete targetTrx;
 
    return ret;
}

void trxManager::store_original_log(trxNode* targetTrx, int table_id, int64_t key, uint64_t pagenum, char* redo, char* undo){
    /* This is not log manager's log. Just for trx_abort */
    record_log* new_log = new record_log;
    new_log->key = key;
    new_log->table_id = table_id;
    memcpy(new_log->redo, redo, 120);
    memcpy(new_log->undo, undo, 120);
    new_log->pagenum = pagenum;
    
    /* Head is latest record */
    if (targetTrx->recordHead == NULL){
        targetTrx->recordHead = new_log;
    }
    else{
        new_log->next = targetTrx->recordHead;
        targetTrx->recordHead = new_log;
    }
}


void trxManager::add_working(trxNode* targetTrx, int table_id, int64_t key){
    workingLock* new_workingLock = new workingLock;
    new_workingLock->key = key;
    new_workingLock->table_id = table_id;

    workingLock* c = targetTrx->workinghash + ((key + table_id) % WORKING_SIZE);
    while (c->next != NULL){
        c = c->next;
    }
    c->next = new_workingLock;
}

void trxManager::delete_working(trxNode* targetTrx, int table_id, int64_t key){
    workingLock* c = targetTrx->workinghash + ((key + table_id) % WORKING_SIZE);
    workingLock* prev = c;
    c = c->next;
    while (c != NULL){
        if (c->table_id == table_id && c->key == key){
            prev->next = c->next;
            delete c;
            return;
        }
        prev = c;
        c = c->next;
    }
}

bool trxManager::is_working(trxNode* targetTrx, int table_id, int64_t key){
    workingLock* c = targetTrx->workinghash + ((key + table_id) % WORKING_SIZE);

    while (c->next != NULL){
        c = c->next;
        if (c->key == key && c->table_id == table_id){
            return true;
        }
    }
    return false;
}

void trxManager::set_next_trx_id(int trx_id){
    next_trx_id = trx_id;
}

int init_trxManager(){
    trx_manager = new trxManager;
    if (trx_manager == NULL){
        return -1;
    }
    
    return 0;
}

int trx_begin(){
    /* Create new trx node for manager lock */
    pthread_mutex_lock(&trx_manager_latch);
    int trx_id = trx_manager->set_new_trxNode();
    pthread_mutex_unlock(&trx_manager_latch);
    if (trx_id == 0){
        return 0;
    }

    /* Publish begin log about new trx */
    log_manager->publish_beginLog(trx_id);

    return trx_id;
}

int trx_commit(int trx_id){
    trxNode* target_trx = trx_manager->get_trxNode(trx_id);
    
    if (target_trx == NULL){
        return 0;
    }
    /* Publish commit log and flush log buffer */
    log_manager->publish_commitLog(trx_id);

    /* Release lock */
    lock_t* c = target_trx->lock_head;
    lock_t* del;
    
    while(c != NULL){
        del = c;
        c = c->next_lock;
        lock_release(del);

        delete del;
    }
    
    int ret = trx_manager->delete_trxNode(target_trx);

    return ret;
}

bool is_dead(lock_t* target){
    stack<int> s;
    hash_pass_trx_id pass;

    lock_t* c = target->prev;
    while(c != NULL){
        if (c->trx_id != target->trx_id){
            s.push(c->trx_id);
        }
        c = c->prev;
    }
    
    int front_trx_id;

    trxNode* targetTrx = trx_manager->get_trxNode(target->trx_id);

    while(!s.empty()){
        front_trx_id = s.top();
        s.pop();

        if (pass.is_in(front_trx_id)){
            continue;
        }
        pass.insert_id(front_trx_id);

        trxNode* frontTrx = trx_manager->get_trxNode(front_trx_id);
        lock_t* next_target = frontTrx->lock_conflict;

        if (next_target == NULL){
            continue;
        }
        
        if (trx_manager->is_working(targetTrx, next_target->sentinel_ptr->table_id, next_target->sentinel_ptr->key)){
            return true;
        }

        c = next_target->prev;
        while(c != NULL){
            if (!(pass.is_in(c->trx_id))){
                s.push(c->trx_id);
            }
            c = c->prev;
        }
    }
    return false;
}

int trx_rollback(trxNode* targetTrx){
    /* recordHead is updated records by targetTrx */
    /* Latest update is recordHead */
    record_log* targetRecord = targetTrx->recordHead;
	
    HeaderPage headerPage;
	LeafPage target_leafPage;
    InternalPage internalPage;
    int i;
    pagenum_t leafPageNum;
    pagenum_t prev;

    /* Undo all updated records, and publsih compensate log */
    while (targetRecord != NULL){
        /* Compensate log's redo is targetRecord's undo, and compensate log's undo is targetRecord's red */

        buffer_read_page(targetRecord->table_id, 0, &headerPage);
        buffer_complete_read_without_write(targetRecord->table_id, 0);
                
        /* Take mutex while finding leaf page */
        pthread_mutex_lock(&tree_latch);

        buffer_read_page(targetRecord->table_id, headerPage.root_pageNum, &internalPage);

        leafPageNum = headerPage.root_pageNum;
        prev = leafPageNum;
        while(!internalPage.isLeaf){
            i = search_index_area_internal(internalPage.node, targetRecord->key, internalPage.num_keys);
            if (i == -1){
                leafPageNum = internalPage.farLeft_pageNum;
            }
            else{
                leafPageNum = internalPage.node[i].pageNum;
            }
            buffer_read_page(targetRecord->table_id, leafPageNum, &internalPage);
            buffer_complete_read_without_write(targetRecord->table_id, prev);
            prev = leafPageNum;
        }
        pthread_mutex_unlock(&tree_latch);

        memcpy(&target_leafPage, &internalPage, PAGE_SIZE);

        i = search_index_location_leaf(target_leafPage.record, targetRecord->key, target_leafPage.num_keys);
        
        /* Before rollback, publish compensate log */
        /* Compensate log's undo is targetRecord->redo, redo is targetRecord->undo */
        /* publish_compensateLog(trx id, table id, pagenum, offset, undo, redo)
        /* Record = key(int64_t, 8byte) + value(char[120], 120byte), PAGE_HEADR_SIZE = 128byte*/
        int offset = PAGE_HEADER_SIZE + (i * sizeof(Record)) + sizeof(int64_t);
        
        uint64_t pageLSN = log_manager->publish_compensateLog(targetTrx->trx_id, targetRecord->table_id, targetRecord->pagenum, offset, targetRecord->redo, targetRecord->undo);

        /* Rollback value to targetRecord's undo and update pageLSN*/
        memcpy(target_leafPage.record[i].value, targetRecord->undo, 120);
        target_leafPage.pageLSN = pageLSN;
        
        buffer_write_page(targetRecord->table_id, targetRecord->pagenum, &target_leafPage);

        targetRecord = targetRecord->next;
    }

    return ROLLBACK_SUCCESS;
}

int trx_abort(int trx_id){
    trxNode* targetTrx = trx_manager->get_trxNode(trx_id);
    if (targetTrx == NULL){
        return 0;
    }

    lock_t* c = targetTrx->lock_head;
    lock_t* del;
    
    int rollbackRet = trx_rollback(targetTrx);
    /* After rollback is done, publish rollback log */
    log_manager->publish_rollbackLog(trx_id);

    /* Release lock */
    while(c != NULL){
        del = c;
        c = c->next_lock;
        lock_release(del);

        delete del;
    }

    return trx_manager->delete_trxNode(targetTrx);
}