#include "file.h"

#define MAX_KEY_LEAF 31
#define MAX_KEY_INTERNAL 248

int open_table(char* path);
int db_insert(int64_t key, char* value);
int db_find(int64_t key, char* ret_val);
int db_delete(int64_t key);

Record make_record(int64_t key, char* value);
Node make_Node(int64_t key, pagenum_t pagenum);
int cut(int len);

pagenum_t find_leafPage(pagenum_t root_pageNum, int64_t key);
pagenum_t make_new_tree(Record record);

int search_index_area_leaf(Record* arr, int64_t key, int num_key);
int search_index_location_leaf(Record* arr, int64_t key, int num_key);
int search_index_area_internal(Node* arr, int64_t key, int num_key);
int search_index_location_internal(Node* arr, int64_t key, int num_key);

pagenum_t insert_into_parent(pagenum_t leftPageNum, pagenum_t rightPageNUm, int64_t key);
pagenum_t insert_into_leafPage_after_splitting(pagenum_t splitted_leafPageNum, Record new_reord);
pagenum_t insert_into_internalPage_after_splitting(pagenum_t splitted_internalPageNum, int area, int64_t key, pagenum_t rightPageNum);

pagenum_t delete_entry(pagenum_t rootPageNum, pagenum_t targetPageNum, int64_t key);
pagenum_t remove_entry_from_page(pagenum_t targetPageNum, int64_t key);
pagenum_t adjust_root(pagenum_t rootPageNum);
pagenum_t delayed_merge(pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime);
pagenum_t redistribute(pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime, int k_prime_index);

struct treeNode
{
  pagenum_t pagenum;
  treeNode* next;
};


void print_tree();
void enqueue(pagenum_t pagenum);
treeNode dequeue(void);
int path_to_root(pagenum_t rootPageNum, pagenum_t childPageNum);
void scan();