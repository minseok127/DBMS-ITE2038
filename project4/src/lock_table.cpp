#include <lock_table.h>
#include <pthread.h>

#define LOCK_TABLE_SIZE (15)

pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;

struct table_entry{
	lock_t* head = NULL;
	lock_t* tail = NULL;
	int table_id = -1;
	int64_t key = -1;

	table_entry* next = NULL;
};

struct lock_t {
	lock_t* next = NULL;
	lock_t* prev = NULL;
	pthread_cond_t cond;
	table_entry* sentinel_ptr = NULL;
};

typedef struct lock_t lock_t;

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
	int pos = find_pos(table_id, key);

	table_entry* c = hash + pos;

	while(c->next != NULL){
		c = c->next;
		if(c->table_id == table_id && c->key == key){
			return c;
		}
	}

	table_entry* new_entry = new table_entry;
	new_entry->table_id = table_id;
	new_entry->key = key;

	c->next = new_entry;

	return new_entry;
}

lock_table* lock_table_ptr;

int
init_lock_table()
{
	lock_table_ptr = new lock_table;
	if(lock_table_ptr == NULL){
		return -1;
	}

	return 0;
}

lock_t*
lock_acquire(int table_id, int64_t key)
{
	pthread_mutex_lock(&lock_table_latch);
	
	table_entry* target_entry = lock_table_ptr->find_table_entry(table_id, key);

	lock_t* new_lock = new lock_t;
	if(new_lock == NULL){
		pthread_mutex_unlock(&lock_table_latch);
		return NULL;
	}
	new_lock->sentinel_ptr = target_entry;
	new_lock->cond = PTHREAD_COND_INITIALIZER;

	if(target_entry->head == NULL){
		target_entry->head = new_lock;
		target_entry->tail = new_lock;
	}
	else{
		lock_t* prev = target_entry->tail;
		
		prev->next = new_lock;
		new_lock->prev = prev;

		target_entry->tail = new_lock;

		pthread_cond_wait(&(new_lock->cond), &lock_table_latch);
	}

	pthread_mutex_unlock(&lock_table_latch);
	
	return new_lock;
}

int
lock_release(lock_t* lock_obj)
{
	pthread_mutex_lock(&lock_table_latch);
	
	table_entry* target_entry = lock_obj->sentinel_ptr;

	if(lock_obj != target_entry->head){
		pthread_mutex_unlock(&lock_table_latch);
		return -1;
	}

	if (target_entry->head->next == NULL){
		target_entry->head = NULL;
		target_entry->tail = NULL;
	}
	else{
		target_entry->head = lock_obj->next;
		target_entry->head->prev = NULL;

		pthread_cond_signal(&(target_entry->head->cond));
	}

	delete lock_obj;
	
	pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
