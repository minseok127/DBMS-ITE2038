#pragma once
#include "fileManager.h"
#include <pthread.h>


struct Buffer{
    FreePage frame;
    int table_id = -1;
    pagenum_t pagenum = 0;
    bool is_dirty = false;
    //int pin_count = 0;
	pthread_mutex_t page_latch = PTHREAD_MUTEX_INITIALIZER;
    Buffer* next = NULL;
	Buffer* prev = NULL;
};

struct DoubleListNode{
	DoubleListNode* prev = NULL;
	DoubleListNode* next = NULL;
	pagenum_t pagenum = 0;
};

struct ListNode {
	pagenum_t pagenum = 0;
	Buffer* bufptr = NULL;
	ListNode* next = NULL;
	DoubleListNode* pagelistptr = NULL;
};


class BufferStack {
private:
	int num_stack;
	int max_num;
	Buffer** stack;
public:
	BufferStack(int max_num);
	void push(Buffer* bufptr);
	Buffer* pop();
	bool is_full();
	~BufferStack();
};

class BufferHash {
private:
	ListNode* hash;
	int size_hash;
	DoubleListNode* listHead;
public:
	int find_pos(pagenum_t pagenum);
	Buffer* find_Hash(pagenum_t pagenum);
	void insert_Hash(pagenum_t pagenum, Buffer* bufptr);
	void delete_Hash(pagenum_t);
	void setHash(int size_hash);
	void insert_doublelist(DoubleListNode* nodeptr);
	void delete_doublelist(DoubleListNode* nodeptr);
	DoubleListNode* get_listHead();
	~BufferHash();
};

void buffer_read_page(int table_id, pagenum_t pagenum, page_t* dest);
void buffer_write_page(int table_id, pagenum_t pagenum, page_t* src);
void buffer_complete_read_without_write(int table_id, pagenum_t pagenum);

void flushBuf(int table_id);

Buffer* get_from_LRUList();
void insert_into_LRUList(Buffer* bufptr);
void remove_from_LRUList(Buffer* bufptr);

pagenum_t buffer_alloc_page(int table_id);
void buffer_free_page(int table_id, pagenum_t pagenum);

void print_buf(int num_buf);