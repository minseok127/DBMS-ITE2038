#include "file.h"
#include <fcntl.h>
#include <unistd.h>

int fd = -1;

pagenum_t file_alloc_page() {
	HeaderPage headerPage;
	FreePage target_freePage;
	int allocated_pageNum;

	file_read_page(0, &headerPage);

	if (headerPage.free_pageNum == 0) {
		allocated_pageNum = headerPage.num_page;
		target_freePage.next_free_pageNum = 0;
		file_write_page(headerPage.num_page++, &target_freePage);

		headerPage.free_pageNum = headerPage.num_page;
		for (int i = 0; i < NEW_FREEPAGE_NUM - 2; i++) {
			target_freePage.next_free_pageNum = headerPage.num_page + 1;
			file_write_page(headerPage.num_page++, &target_freePage);
		}
		target_freePage.next_free_pageNum = 0;
		file_write_page(headerPage.num_page++, &target_freePage);

		file_write_page(0, &headerPage);

		return allocated_pageNum;
	}

	file_read_page(headerPage.free_pageNum, &target_freePage);
	allocated_pageNum = headerPage.free_pageNum;
	headerPage.free_pageNum = target_freePage.next_free_pageNum;
	target_freePage.next_free_pageNum = 0;
	file_write_page(0, &headerPage);

	return allocated_pageNum;
}

void file_free_page(pagenum_t pagenum) {
	if (pagenum == 0) {
		return;
	}

	HeaderPage headerPage;
	file_read_page(0, &headerPage);

	if (pagenum >= headerPage.num_page) {
		return;
	}

	FreePage target_page;
	file_read_page(pagenum, &target_page);

	target_page.next_free_pageNum = headerPage.free_pageNum;
	for (int i = 0; i < 1022; i++) {
		target_page.reserved[i] = 0;
	}
	headerPage.free_pageNum = pagenum;

	file_write_page(0, &headerPage);
	file_write_page(pagenum, &target_page);
}

void file_read_page(pagenum_t pagenum, page_t* dest) {
	lseek(fd, pagenum * PAGE_SIZE, 0);
	read(fd, dest, PAGE_SIZE);
}

void file_write_page(pagenum_t pagenum, const page_t* src) {
	lseek(fd, pagenum * PAGE_SIZE, 0);
	if (write(fd, src, PAGE_SIZE) == PAGE_SIZE){
		fsync(fd);
	}
}

void open_file(char* path) {
	fd = open(path, O_RDWR | O_CREAT, 00700);
}