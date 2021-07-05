#pragma once
#include <stdint.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define NEW_FREEPAGE_NUM 1024
#define PAGE_HEADER_SIZE 128

typedef uint64_t pagenum_t;
struct page_t {};

typedef struct Record {
	int64_t key = 0x00;
	char value[120] = {0x00};
}Record;

typedef struct Node {
	int64_t key=0x00;
	pagenum_t pageNum=0x00;
}Node;

struct HeaderPage : public page_t {
	pagenum_t free_pageNum=0x00;
	pagenum_t root_pageNum=0x00;
	int64_t num_page=0x00;
	int reserved[1018] = { 0x00 };
};

struct FreePage : public page_t {
	pagenum_t next_free_pageNum=0x00;
	int reserved[1022] = { 0x00 };
};

struct InternalPage : public page_t {
	pagenum_t parent_pageNum=0x00;
	int isLeaf=0x00;
	int num_keys=0x00;
	int reserved_ = 0x00;
	int pageLSN = 0x00;
	int reserved[24] = { 0x00 };
	pagenum_t farLeft_pageNum=0x00;
	Node node[248] = {0x00};
};

struct LeafPage : public page_t {
	pagenum_t parent_pageNum=0x00;
	int isLeaf=0x00;
	int num_keys=0x00;
	int reserved_ = 0x00;
	int pageLSN = 0x00;
	int reserved[24] = { 0x00 };
	pagenum_t sibling_pageNum=0x00;
	Record record[31] = {0x00};
};


pagenum_t file_alloc_page(int table_id, int fd);
void file_free_page(int table_id, pagenum_t pagenum);
void file_read_page(int fd, pagenum_t pagenum, page_t* dest);
void file_write_page(int fd, pagenum_t pagenum, const page_t* src);

int open_file(char* path);
int close_file(int fd);