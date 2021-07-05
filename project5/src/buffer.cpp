#include "../include/bufferManager.h"
#include "../include/indexManager.h"
#include <string.h>
#include <iostream>
#include <time.h>

using namespace std;

extern TableManager* tableManager;

BufferStack* bufStack;
BufferHash* bufHash;
Buffer* buf;
Buffer* LRU_Head;
Buffer* LRU_Tail;

pthread_mutex_t bufStack_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bufHash_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_latch = PTHREAD_MUTEX_INITIALIZER;

extern pthread_mutex_t log_manager_latch;

BufferStack::BufferStack(int max_num) {
	this->max_num = max_num;
	num_stack = 0;
	stack = new Buffer*[this->max_num];
	for (int i = 0; i < this->max_num; i++) {
		stack[i] = NULL;
	}
}

BufferStack::~BufferStack(){
	delete[] stack;
}

void BufferStack::push(Buffer* bufptr) {
	if (is_full()) {
		return;
	}
	stack[num_stack++] = bufptr;

	return;
}

Buffer* BufferStack::pop() {
	if (num_stack == 0) {
		return NULL;
	}
	num_stack--;
	Buffer* retbuf = stack[num_stack];

	return retbuf;
}

bool BufferStack::is_full(){
	if (num_stack == max_num){
		return true;
	}
	else{
		return false;
	}
}

BufferHash::~BufferHash() {
	delete[] hash;
	delete listHead;
}

int BufferHash::find_pos(pagenum_t pagenum) {
	return pagenum % size_hash;
}

Buffer* BufferHash::find_Hash(pagenum_t pagenum) {
	int pos = find_pos(pagenum);
	ListNode* c = hash + pos;
	Buffer* retbuf = NULL;

	while (c->next != NULL) {
		c = c->next;
		if (c->pagenum == pagenum) {
			retbuf = c->bufptr;
			break;
		}
	}

	return retbuf;
}

void BufferHash::insert_Hash(pagenum_t pagenum, Buffer* bufptr) {
	int pos = find_pos(pagenum);
	ListNode* c = hash + pos;

	while (c->next != NULL) {
		c = c->next;
	}

	ListNode* newNode = new ListNode;
	newNode->pagenum = pagenum;
	newNode->bufptr = bufptr;
	newNode->next = NULL;

	newNode->pagelistptr = new DoubleListNode;
	newNode->pagelistptr->pagenum = pagenum;
	insert_doublelist(newNode->pagelistptr);

	c->next = newNode;

	return;
}

void BufferHash::delete_Hash(pagenum_t pagenum) {
	int pos = find_pos(pagenum);
	ListNode* c = hash + pos;

	while (c->next != NULL) {
		ListNode* prev = c;
		c = c->next;
		if (c->pagenum == pagenum) {
			prev->next = c->next;
			delete_doublelist(c->pagelistptr);
			delete c;
			break;
		}
	}

	return;
}

void BufferHash::setHash(int size_hash){
	this->size_hash = size_hash;
	this->hash = new ListNode[size_hash];
	this->listHead = new DoubleListNode;
}

void BufferHash::insert_doublelist(DoubleListNode* nodeptr){
	DoubleListNode* next = this->listHead->next;
	this->listHead->next = nodeptr;
	
	nodeptr->next = next;
	nodeptr->prev = this->listHead;
	
	if (next != NULL){
		next->prev = nodeptr;
	}
}

void BufferHash::delete_doublelist(DoubleListNode* nodeptr){
	DoubleListNode* prev = nodeptr->prev;
	DoubleListNode* next = nodeptr->next;

	prev->next = next;
	if (next != NULL){
		next->prev = prev;
	}

	delete nodeptr;
}

DoubleListNode* BufferHash::get_listHead(){
	return this->listHead;
}

void buffer_read_page(int table_id, pagenum_t pagenum, page_t* dest, int mode){
	pthread_mutex_lock(&buffer_latch);
	Buffer* bufptr = bufHash[table_id - 1].find_Hash(pagenum);
	int fd = tableManager->get_fileTable(table_id)->getFd();
	if (bufptr == NULL){
		bufptr = bufStack->pop();

		if (bufptr == NULL){

			
			bufptr = LRU_Tail->prev;
			pthread_mutex_lock(&(bufptr->page_latch));
			
			Buffer* next = bufptr->next;
			Buffer* prev = bufptr->prev;

			next->prev = prev;
			prev->next = next;
		
			bufptr->next = LRU_Head->next;
			LRU_Head->next->prev = bufptr;

			LRU_Head->next = bufptr;
			bufptr->prev = LRU_Head;

			pthread_mutex_unlock(&buffer_latch);
			
			bufHash[bufptr->table_id - 1].delete_Hash(bufptr->pagenum);

			int old_fd = tableManager->get_fileTable(bufptr->table_id)->getFd();

			if(bufptr->is_dirty == true){

				file_write_page(old_fd, bufptr->pagenum, &(bufptr->frame));
			}

			file_read_page(fd, pagenum, &(bufptr->frame));
			memcpy(dest, &(bufptr->frame), PAGE_SIZE);

			if (mode == RD_ONLY){
				pthread_mutex_unlock(&(bufptr->page_latch));
			}
			
			bufptr->is_dirty = false;
			bufptr->pagenum = pagenum;
			bufptr->table_id = table_id;

			bufHash[table_id - 1].insert_Hash(pagenum, bufptr);
			
		}
		else{
	
			pthread_mutex_lock(&(bufptr->page_latch));

			bufptr->next = LRU_Head->next;
			LRU_Head->next->prev = bufptr;

			LRU_Head->next = bufptr;
			bufptr->prev = LRU_Head;
			
			pthread_mutex_unlock(&buffer_latch);
			
			file_read_page(fd, pagenum, &(bufptr->frame));
			memcpy(dest, &(bufptr->frame), PAGE_SIZE);

			if (mode == RD_ONLY){
				pthread_mutex_unlock(&(bufptr->page_latch));
			}
			
			bufptr->is_dirty = false;
			bufptr->pagenum = pagenum;
			bufptr->table_id = table_id;

			bufHash[table_id - 1].insert_Hash(pagenum, bufptr);

		}
	}
	else{
		Buffer* next = bufptr->next;
		Buffer* prev = bufptr->prev;

		next->prev = prev;
		prev->next = next;

		bufptr->next = NULL;
		bufptr->prev = NULL;
	
		bufptr->next = LRU_Head->next;
		LRU_Head->next->prev = bufptr;

		LRU_Head->next = bufptr;
		bufptr->prev = LRU_Head;
		
		pthread_mutex_lock(&(bufptr->page_latch));
		pthread_mutex_unlock(&buffer_latch);

		memcpy(dest, &(bufptr->frame), PAGE_SIZE);
		if (mode == RD_ONLY){
			pthread_mutex_unlock(&(bufptr->page_latch));
		}
	}
	return;
}

void buffer_RDWR(int table_id, pagenum_t pagenum, page_t* src){
	pthread_mutex_lock(&buffer_latch);
	Buffer* bufptr = bufHash[table_id - 1].find_Hash(pagenum);
	int fd = tableManager->get_fileTable(table_id)->getFd();
	if (bufptr == NULL){
		bufptr = bufStack->pop();

		if (bufptr == NULL){
			
			bufptr = LRU_Tail->prev;
			pthread_mutex_lock(&(bufptr->page_latch));
			
			Buffer* next = bufptr->next;
			Buffer* prev = bufptr->prev;

			next->prev = prev;
			prev->next = next;
		
			bufptr->next = LRU_Head->next;
			LRU_Head->next->prev = bufptr;

			LRU_Head->next = bufptr;
			bufptr->prev = LRU_Head;

			pthread_mutex_unlock(&buffer_latch);
			
			bufHash[bufptr->table_id - 1].delete_Hash(bufptr->pagenum);

			int old_fd = tableManager->get_fileTable(bufptr->table_id)->getFd();

			if(bufptr->is_dirty == true){
				file_write_page(old_fd, bufptr->pagenum, &(bufptr->frame));
			}
			memcpy(&(bufptr->frame), src, PAGE_SIZE);
			pthread_mutex_unlock(&(bufptr->page_latch));
			
			bufptr->is_dirty = true;
			bufptr->pagenum = pagenum;
			bufptr->table_id = table_id;

			bufHash[table_id - 1].insert_Hash(pagenum, bufptr);
		}
		else{
			pthread_mutex_lock(&(bufptr->page_latch));
			bufptr->next = LRU_Head->next;
			LRU_Head->next->prev = bufptr;

			LRU_Head->next = bufptr;
			bufptr->prev = LRU_Head;
			
			pthread_mutex_unlock(&buffer_latch);
			
			memcpy(&(bufptr->frame), src, PAGE_SIZE);
			pthread_mutex_unlock(&(bufptr->page_latch));
			
			bufptr->is_dirty = true;
			bufptr->pagenum = pagenum;
			bufptr->table_id = table_id;

			bufHash[table_id - 1].insert_Hash(pagenum, bufptr);
		}
	}
	else{
		Buffer* next = bufptr->next;
		Buffer* prev = bufptr->prev;

		next->prev = prev;
		prev->next = next;

		bufptr->next = NULL;
		bufptr->prev = NULL;
	
		bufptr->next = LRU_Head->next;
		LRU_Head->next->prev = bufptr;

		LRU_Head->next = bufptr;
		bufptr->prev = LRU_Head;

		pthread_mutex_lock(&(bufptr->page_latch));
		pthread_mutex_unlock(&buffer_latch);

		memcpy(&(bufptr->frame), src, PAGE_SIZE);
		pthread_mutex_unlock(&(bufptr->page_latch));
		bufptr->is_dirty = true;
	}
	return;
}

void buffer_write_page(int table_id, pagenum_t pagenum, page_t* src){
	Buffer* bufptr = bufHash[table_id - 1].find_Hash(pagenum);

	if(bufptr == NULL){
		return;
	}

	memcpy(&(bufptr->frame), src, PAGE_SIZE);
	bufptr->is_dirty = true;
	pthread_mutex_unlock(&(bufptr->page_latch));
}

void buffer_complete_read_without_write(int table_id, pagenum_t pagenum){
	Buffer* bufptr = bufHash[table_id - 1].find_Hash(pagenum);

	if(bufptr == NULL){
		return;
	}
	
	pthread_mutex_unlock(&(bufptr->page_latch));
}

void flushBuf(int table_id){
	pthread_mutex_lock(&buffer_latch);

	DoubleListNode* head = bufHash[table_id - 1].get_listHead();
	DoubleListNode* c = head->next;

	int fd = tableManager->get_fileTable(table_id)->getFd();

	Buffer* targetptr;

	while(head->next != NULL){
		pagenum_t targetpagenum = c->pagenum;
		c = c->next;
		if(c == NULL){
			c = head->next;
		}

		targetptr = bufHash[table_id - 1].find_Hash(targetpagenum);
		if (targetptr == NULL){
			continue;
		}
		
		pthread_mutex_lock(&(targetptr->page_latch));
		
		if (targetptr->is_dirty == true){
			file_write_page(fd, targetptr->pagenum, &(targetptr->frame));
			targetptr->is_dirty = false;
		}
		
		remove_from_LRUList(targetptr);
		bufHash[targetptr->table_id - 1].delete_Hash(targetptr->pagenum);
		bufStack->push(targetptr);
				
		targetptr->table_id = -1;
		targetptr->pagenum = 0;

		pthread_mutex_unlock(&(targetptr->page_latch));
		/*
		if (targetptr->pin_count == 0){
			if (targetptr->is_dirty == true){
				file_write_page(fd, targetptr->pagenum, &(targetptr->frame));
				targetptr->is_dirty = false;
			}
			bufHash[targetptr->table_id - 1].delete_Hash(targetptr->pagenum);
			bufStack->push(targetptr);
				
			remove_from_LRUList(targetptr);
			targetptr->table_id = -1;
			targetptr->pagenum = 0;		
		}
		*/
	}

	pthread_mutex_unlock(&buffer_latch);
}

Buffer* get_from_LRUList(){
	Buffer* Target = LRU_Tail->prev;
	
	/*
	while(Target->pin_count != 0){
		Target = Target->prev;
		if(Target == LRU_Head){
			Target = LRU_Tail->prev;
		}
	}*/
	pthread_mutex_lock(&(Target->page_latch));
	remove_from_LRUList(Target);
	insert_into_LRUList(Target);
	pthread_mutex_unlock(&buffer_latch);


	return Target;
}

void insert_into_LRUList(Buffer* bufptr){
	Buffer* next = LRU_Head->next;
	
	bufptr->next = next;
	next->prev = bufptr;

	LRU_Head->next = bufptr;
	bufptr->prev = LRU_Head;
}

void remove_from_LRUList(Buffer* bufptr){
	Buffer* next = bufptr->next;
	Buffer* prev = bufptr->prev;

	next->prev = prev;
	prev->next = next;

	bufptr->next = NULL;
	bufptr->prev = NULL;
}

pagenum_t buffer_alloc_page(int table_id){
	HeaderPage headerPage;
	FreePage target_freePage;
	pagenum_t allocated_pageNum;

	int fd = tableManager->get_fileTable(table_id)->getFd();

	buffer_read_page(table_id, 0, &headerPage);

	if(headerPage.free_pageNum == 0){
		buffer_complete_read_without_write(table_id, 0);
		return file_alloc_page(table_id, fd);
	}

	buffer_read_page(table_id, headerPage.free_pageNum, &target_freePage);
	allocated_pageNum = headerPage.free_pageNum;
	headerPage.free_pageNum = target_freePage.next_free_pageNum;
	target_freePage.next_free_pageNum = 0;
	
	buffer_write_page(table_id, 0, &headerPage);
	buffer_write_page(table_id, allocated_pageNum, &target_freePage);

	return allocated_pageNum;
}

void buffer_free_page(int table_id, pagenum_t pagenum){
	if (pagenum == 0) {
		return;
	}

	file_free_page(table_id, pagenum);
}

void print_buf(int num_buf){
	cout << "table_id :		";
	for(int i = 0; i < num_buf; i++){
		cout << (buf + i)->table_id << "	";
	}
	cout << endl;
	
	cout << "pagenum :		";
	for(int i = 0; i < num_buf; i++){
		cout << (buf + i)->pagenum << "	";
	}
	cout << endl;

	cout << "is_dirty :		";
	for(int i = 0; i < num_buf; i++){
		cout << (buf + i)->is_dirty << "	";
	}
	cout << endl;
}