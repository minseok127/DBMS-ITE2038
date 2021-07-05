#include "../include/file.h"
#include "../include/buffer.h"
#include <fcntl.h>
#include <iostream>

pagenum_t file_alloc_page(int table_id, int fd) {
	HeaderPage headerPage;
	FreePage target_freePage;
	pagenum_t allocated_pageNum;

	buffer_read_page(table_id, 0, &headerPage);

	allocated_pageNum = headerPage.num_page;
	target_freePage.next_free_pageNum = 0;
	file_write_page(fd, headerPage.num_page++, &target_freePage);

	headerPage.free_pageNum = headerPage.num_page;
	for (int i = 0; i < NEW_FREEPAGE_NUM - 2; i++) {
		target_freePage.next_free_pageNum = headerPage.num_page + 1;
		file_write_page(fd, headerPage.num_page++, &target_freePage);
	}
	target_freePage.next_free_pageNum = 0;
	file_write_page(fd, headerPage.num_page++, &target_freePage);

	buffer_write_page(table_id, 0, &headerPage);

	return allocated_pageNum;
}

void file_free_page(int table_id, pagenum_t pagenum) {
	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);

	if (pagenum >= headerPage.num_page) {
		buffer_complete_read_without_write(table_id, 0);
		return;
	}

	FreePage target_page;
	buffer_read_page(table_id, pagenum, &target_page);

	target_page.next_free_pageNum = headerPage.free_pageNum;
	for (int i = 0; i < 1022; i++) {
		target_page.reserved[i] = 0;
	}
	headerPage.free_pageNum = pagenum;

	buffer_write_page(table_id, 0, &headerPage);
	buffer_write_page(table_id, pagenum, &target_page);
}

void file_read_page(int fd, pagenum_t pagenum, page_t* dest) {
	if(fd < 0){
		return;
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	read(fd, dest, PAGE_SIZE);
}

void file_write_page(int fd, pagenum_t pagenum, const page_t* src) {
	if(fd < 0){
		return;
	}
	lseek(fd, pagenum * PAGE_SIZE, SEEK_SET);
	if (write(fd, src, PAGE_SIZE) == PAGE_SIZE){
		fsync(fd);
	}
}

int open_file(char* path) {
	int fd = open(path, O_RDWR | O_CREAT, 00700);
	
	if (fd < 0){
		return -1;
	}

	return fd;
}

int close_file(int fd){
	return close(fd);
}