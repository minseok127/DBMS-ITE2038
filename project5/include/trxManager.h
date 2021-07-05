#pragma once
#include "lockManager.h"
#include <pthread.h>

#define TRX_MANAGER_HASH_SIZE (500)
#define WORKING_SIZE (10)

/* Result of rollback */
#define ROLLBACK_SUCCESS (0)
#define ROLLBACK_FAIL (-1)

struct record_log{
    int table_id;
    int64_t key;
    char redo[120];
    char undo[120];
    record_log* next = NULL;
};

struct workingLock{
    int table_id;
    int64_t key;

    workingLock* next = NULL;
};

struct trxNode{
    int trx_id = 0;
    trxNode* next = NULL;
    trxNode* prev = NULL;

    lock_t* lock_head = NULL;
    lock_t* lock_tail = NULL;
    lock_t* lock_conflict = NULL;

    record_log* recordHead = NULL;

    workingLock* workinghash = NULL;

    /* For general design */
    pthread_mutex_t trxLatch = PTHREAD_MUTEX_INITIALIZER;
};

class trxManager{
private:
    int next_trx_id = 1;
    trxNode* hash;
public:
    trxManager();
    //~trxManager();

    int set_new_trxNode();
    trxNode* get_trxNode(int trx_id);
    int delete_trxNode(trxNode* targetTrx);

    int find_pos(int num, int size);

    void store_original_log(trxNode* targetTrx, int table_id, int64_t key, char* redo, char* undo);

    void add_working(trxNode* targetTrx, int table_id, int64_t key);
    void delete_working(trxNode* targetTrx, int table_id, int64_t key);
    bool is_working(trxNode* targetTrx, int table_id, int64_t key);

    void increase_trx_id();
};

int init_trxManager();
bool is_dead(lock_t* target);
int trx_rollback(trxNode* targetTrx);