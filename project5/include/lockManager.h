#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <stdint.h>
#include <iostream>

/* Lock mode, Shared / Exclusive */
#define S (0)
#define X (1)

/* Lock acquire return three type of value */
#define ACQUIRED (0)
#define NEED_TO_WAIT (1)
#define DEADLOCK (2)

typedef int lock_mode_t;
typedef struct lock_t lock_t;
typedef struct table_entry table_entry;

struct lock_t {
	lock_t* next = NULL;
	lock_t* prev = NULL;
	pthread_cond_t cond;
	table_entry* sentinel_ptr = NULL;
	
	lock_mode_t lock_mode;
	lock_t* next_lock = NULL;
	lock_t* prev_lock = NULL;
	int trx_id;
	bool is_working = false;
};

struct table_entry{
	lock_t* head = NULL;
	lock_t* tail = NULL;
	int table_id = -1;
	int64_t key = -1;

	table_entry* next = NULL;
	int working_count = 0;

	pthread_mutex_t table_entry_latch = PTHREAD_MUTEX_INITIALIZER;
};

class lock_table{
private:
	int size_table;
	table_entry* hash;
public:
	lock_table();
	~lock_table();
	int find_pos(int table_id, int64_t key);
	table_entry* find_table_entry(int table_id, int64_t key);
};

/* APIs for lock table */
int init_lock_table();
int lock_acquire(int table_id, int64_t key, int trx_id, int mode, lock_t** ret_lock);
int lock_release(lock_t* lock_obj);
void lock_wait(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
