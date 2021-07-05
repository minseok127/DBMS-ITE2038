#include "../include/lockManager.h"
#include "../include/trxManager.h"
#include <pthread.h>
#include <time.h>

#define LOCK_TABLE_SIZE (1000)

using namespace std;

pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

extern pthread_mutex_t trx_manager_latch;

lock_table* lock_table_ptr;

extern trxManager* trx_manager;

typedef struct lock_t lock_t;

lock_table::lock_table(){
	size_table = LOCK_TABLE_SIZE;
	hash = new table_entry[size_table];
}

lock_table::~lock_table(){
	table_entry* del;

	for(int i = 0; i < size_table; i++){
		table_entry* c = hash + i;
		
		while(c != NULL){
			del = c;
			c = c->next;
			delete del;
		}
	}
}

int lock_table::find_pos(int table_id, int64_t key){
	return (table_id+key) % size_table;
}

table_entry* lock_table::find_table_entry(int table_id, int64_t key){
	int pos = (table_id + key) % size_table;

	table_entry* c = hash + pos;

	while(c->next != NULL){
		c = c->next;
		if(c->table_id == table_id && c->key == key){
			return c;
		}
	}

	table_entry* new_entry = new table_entry;
	new_entry->head = NULL;
	new_entry->tail = NULL;
	new_entry->working_count = 0;
	new_entry->next = NULL;
	new_entry->table_id = table_id;
	new_entry->key = key;

	c->next = new_entry;

	return new_entry;
}


int
init_lock_table()
{
	lock_table_ptr = new lock_table;
	if(lock_table_ptr == NULL){
		return -1;
	}

	return 0;
}

int
lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, lock_t** ret_lock)
{
	pthread_mutex_lock(&lock_table_latch);

	table_entry* target_entry = lock_table_ptr->find_table_entry(table_id, key);

	trxNode* targetTrx = trx_manager->get_trxNode(trx_id);

	lock_t* new_lock = new lock_t;
	new_lock->next = NULL;
	new_lock->prev = NULL;
	new_lock->next_lock = NULL;
	new_lock->prev_lock = NULL;
	new_lock->is_working = false;

	new_lock->sentinel_ptr = target_entry;
	new_lock->cond = PTHREAD_COND_INITIALIZER;
	
	new_lock->lock_mode = lock_mode;
	new_lock->trx_id = trx_id;

	if (trx_manager->is_working(targetTrx, table_id, key)){
		if (lock_mode == S || (target_entry->head->lock_mode == X && target_entry->head->trx_id == trx_id) || (target_entry->tail == target_entry->head && target_entry->head->trx_id == trx_id)){
			if (target_entry->tail == target_entry->head && target_entry->head->lock_mode < lock_mode){
				target_entry->head->lock_mode = lock_mode;
			}
			delete new_lock;
			pthread_mutex_unlock(&lock_table_latch);
			return ACQUIRED;
		}
		else if (!(target_entry->tail->is_working)){
			target_entry->tail->next = new_lock;
			new_lock->prev = target_entry->tail;
			target_entry->tail = new_lock;

			if (targetTrx->lock_tail == NULL){
				targetTrx->lock_head = new_lock;
				targetTrx->lock_tail = new_lock;
			}
			else{
				targetTrx->lock_tail->next_lock = new_lock;
				new_lock->prev_lock = targetTrx->lock_tail;
				targetTrx->lock_tail = new_lock;
			}
			targetTrx->lock_conflict = new_lock;
		}
		else{
			if (target_entry->tail->trx_id != trx_id){
				lock_t* c = target_entry->head;
				while(c->trx_id != trx_id){
					c = c->next;
				}
							
				if (target_entry->head == c){
					c->next->prev = NULL;
					target_entry->head = c->next;
				}
				else{
					c->next->prev = c->prev;
					c->prev->next = c->next;
				}
				target_entry->tail->next = c;
				c->prev = target_entry->tail;
				c->next = NULL;
				target_entry->tail = c;
			}

			target_entry->tail->lock_mode = X;
			target_entry->tail->is_working = false;
			target_entry->working_count--;

			trx_manager->delete_working(targetTrx, table_id, key);

			lock_t* del = new_lock;
			new_lock = target_entry->tail;

			delete del;

			targetTrx->lock_conflict = target_entry->tail;
		}
	}
	else{
		if (target_entry->head == NULL){
			target_entry->head = new_lock;
			target_entry->tail = new_lock;
			target_entry->working_count++;

			new_lock->is_working = true;

			if (targetTrx->lock_tail == NULL){
				targetTrx->lock_head = new_lock;
				targetTrx->lock_tail = new_lock;
			}
			else{
				targetTrx->lock_tail->next_lock = new_lock;
				new_lock->prev_lock = targetTrx->lock_tail;
				targetTrx->lock_tail = new_lock;
			}
			
			trx_manager->add_working(targetTrx, table_id, key);

			pthread_mutex_unlock(&lock_table_latch);
			return ACQUIRED;
		}

		target_entry->tail->next = new_lock;
		new_lock->prev = target_entry->tail;
		target_entry->tail = new_lock;

		if (targetTrx->lock_tail == NULL){
			targetTrx->lock_head = new_lock;
			targetTrx->lock_tail = new_lock;
		}
		else{
			targetTrx->lock_tail->next_lock = new_lock;
			new_lock->prev_lock = targetTrx->lock_tail;
			targetTrx->lock_tail = new_lock;
		}

		if ((target_entry->tail->prev->lock_mode == S && target_entry->tail->prev->is_working) && target_entry->tail->lock_mode == S){
			target_entry->tail->is_working = true;
			target_entry->working_count++;

			trx_manager->add_working(targetTrx, table_id, key);

			pthread_mutex_unlock(&lock_table_latch);
			return ACQUIRED;
		}

		targetTrx->lock_conflict = target_entry->tail;
	}
	
	/* Chcek deadlock */
	if (is_dead(new_lock)){
		targetTrx->lock_conflict = NULL;
		pthread_mutex_unlock(&lock_table_latch);
		return DEADLOCK;
	}

	/* Acquire the transaction latch */
	/* transaction manager latch is not transaction latch */
	pthread_mutex_lock(&(targetTrx->trxLatch));
	
	/* This will be used in db_find, db_update for wait */
	*ret_lock = new_lock;

	/* Release the lock manager latch */
	pthread_mutex_unlock(&lock_table_latch);


	return NEED_TO_WAIT;
}

int
lock_release(lock_t* lock_obj)
{
	pthread_mutex_lock(&lock_table_latch);

	table_entry* target_entry = lock_obj->sentinel_ptr;
	if (lock_obj->is_working){
		if (target_entry->working_count == 1){
			
			target_entry->head = lock_obj->next;
			if (lock_obj == target_entry->tail){
				target_entry->tail = NULL;
				target_entry->working_count--;
			}
			else{

				target_entry->head->prev = NULL;

				lock_t* c = target_entry->head;
				c->is_working = true;

				trxNode* targetTrx = trx_manager->get_trxNode(c->trx_id);

				targetTrx->lock_conflict = NULL;
				trx_manager->add_working(targetTrx, c->sentinel_ptr->table_id, c->sentinel_ptr->key);
				
				pthread_mutex_lock(&(targetTrx->trxLatch));
				pthread_cond_signal(&(c->cond));
				pthread_mutex_unlock(&(targetTrx->trxLatch));
					
				if (c->lock_mode == S){
					c = c->next;
					while(c != NULL && c->lock_mode != X){
						target_entry->working_count++;
						c->is_working = true;
						
						targetTrx = trx_manager->get_trxNode(c->trx_id);
						
						targetTrx->lock_conflict = NULL;
						trx_manager->add_working(targetTrx, c->sentinel_ptr->table_id, c->sentinel_ptr->key);

						pthread_mutex_lock(&targetTrx->trxLatch);
						pthread_cond_signal(&(c->cond));
						pthread_mutex_unlock(&targetTrx->trxLatch);
						
						c = c->next;
					}
				}
			}
		}
		else{

			if (lock_obj == target_entry->head){
				target_entry->head = lock_obj->next;
			}
			if (lock_obj == target_entry->tail){
				target_entry->tail = lock_obj->prev;
			}

			if (lock_obj->prev != NULL){
				lock_obj->prev->next = lock_obj->next;
			}
			if (lock_obj->next != NULL){
				lock_obj->next->prev = lock_obj->prev;
			}

			target_entry->working_count--;

			if (target_entry->head != NULL && target_entry->head->next != NULL && target_entry->head->trx_id == target_entry->head->next->trx_id){
				target_entry->head->prev_lock->next_lock = target_entry->head->next_lock;
				if (target_entry->head->next_lock != NULL){
					target_entry->head->next_lock->prev_lock = target_entry->head->prev_lock;
				}
				lock_t* del = target_entry->head;

				target_entry->head = target_entry->head->next;
				target_entry->head->is_working = true;
				target_entry->head->prev = NULL;

				trxNode* targetTrx = trx_manager->get_trxNode(target_entry->head->trx_id);
								
				targetTrx->lock_conflict = NULL;

				delete del;
				
				pthread_mutex_lock(&targetTrx->trxLatch);
				pthread_cond_signal(&(target_entry->head->cond));
				pthread_mutex_unlock(&targetTrx->trxLatch);
			}
		}
	}
	else{
		lock_obj->prev->next = lock_obj->next;
		if (lock_obj->next != NULL){
			lock_obj->next->prev = lock_obj->prev;
		}
		else{
			target_entry->tail = lock_obj->prev;
		}
	}
	
	pthread_mutex_unlock(&lock_table_latch);

	return 0;
}

void lock_wait(lock_t* lock_obj){
	trxNode* targetTrx = trx_manager->get_trxNode(lock_obj->trx_id);
	/* Sleep on the condition variable of lock_obj, atomically releaseing the transaciton latch */
	pthread_cond_wait(&(lock_obj->cond), &(targetTrx->trxLatch));
	/* Wake up and release transaction latch */
	pthread_mutex_unlock(&(targetTrx->trxLatch));
}
