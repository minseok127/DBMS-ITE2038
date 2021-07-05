#pragma once
#include "bufferManager.h"
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_KEY_LEAF 31
#define MAX_KEY_INTERNAL 248
#define MAX_PATH_LEN 20
#define MAX_TABLE_NUM 10

class file_table{
private:
	ino_t iNum = 0;
	int fd = -2;
public:
	int getFd();
	ino_t getInodeNum();
	void setFd(int fd);
	void setInodeNum(ino_t inum);
};

class TableManager{
private:
  file_table* fileTable;
public:
  TableManager();
  ~TableManager();
  file_table* get_fileTable(int table_id);
};


Record make_record(int64_t key, char* value);
Node make_Node(int64_t key, pagenum_t pagenum);
int cut(int len);

pagenum_t find_leafPage(int table_id, pagenum_t root_pageNum, int64_t key);
pagenum_t make_new_tree(int table_id, Record record);

int search_index_area_leaf(Record* arr, int64_t key, int num_key);
int search_index_location_leaf(Record* arr, int64_t key, int num_key);
int search_index_area_internal(Node* arr, int64_t key, int num_key);
int search_index_location_internal(Node* arr, int64_t key, int num_key);

pagenum_t insert_into_parent(int table_id, pagenum_t leftPageNum, pagenum_t rightPageNUm, int64_t key);
pagenum_t insert_into_leafPage_after_splitting(int table_id, pagenum_t splitted_leafPageNum, Record new_reord);
pagenum_t insert_into_internalPage_after_splitting(int table_id, pagenum_t splitted_internalPageNum, int area, int64_t key, pagenum_t rightPageNum);

pagenum_t delete_entry(int table_id, pagenum_t rootPageNum, pagenum_t targetPageNum, int64_t key);
pagenum_t remove_entry_from_page(int table_id, pagenum_t targetPageNum, int64_t key);
pagenum_t adjust_root(int table_id, pagenum_t rootPageNum);
pagenum_t delayed_merge(int table_id, pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime);
pagenum_t redistribute(int table_id, pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime, int k_prime_index);

struct treeNode
{
  pagenum_t pagenum;
  treeNode* next;
};


void print_tree(int table_id);
void enqueue(pagenum_t pagenum);
treeNode dequeue(void);
int path_to_root(int table_id, pagenum_t rootPageNum, pagenum_t childPageNum);
void scan(int table_id);