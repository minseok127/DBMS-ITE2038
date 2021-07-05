#include "../include/bpt.h"
#include <string.h>
#include <iostream>

using namespace std;

TableManager* tableManager;

extern Buffer* buf;
extern BufferHash* bufHash;
extern BufferStack* bufStack;
extern Buffer* LRU_Head;
extern Buffer* LRU_Tail;

int file_table::getFd(){
	return fd;
}

ino_t file_table::getInodeNum(){
	return iNum;
}

void file_table::setFd(int fd){
	this->fd = fd;
}

void file_table::setInodeNum(ino_t inum){
	this->iNum = inum;
}

TableManager::TableManager(){
	this->fileTable = new file_table[MAX_TABLE_NUM];
}

TableManager::~TableManager(){
	delete[] this->fileTable;
}

file_table* TableManager::get_fileTable(int table_id){
	return &(this->fileTable[table_id - 1]);
}

int init_db(int num_buf){
	tableManager = new TableManager;
	if(tableManager == NULL){
		return -1;
	}
	
	LRU_Head = new Buffer;
	LRU_Tail = new Buffer;

	LRU_Head->next = LRU_Tail;
	LRU_Tail->prev = LRU_Head;
    
	buf = new Buffer[num_buf];
    if(buf == NULL){
        return -1;
    }

    bufHash = new BufferHash[MAX_TABLE_NUM];
    if(bufHash == NULL){
        return -1;
    }
	for(int i = 0; i < MAX_TABLE_NUM; i++){
		bufHash[i].setHash(num_buf);
	}

    bufStack = new BufferStack(num_buf);
    if(bufStack == NULL){
        return -1;
    }
	for(int i = 0; i < num_buf; i++){
		bufStack->push(buf + i);
	}

    return 0;
}

int shutdown_db(){
	for(int table_id = 1; table_id <= MAX_TABLE_NUM && tableManager->get_fileTable(table_id)->getFd() != -2; table_id++){
		flushBuf(table_id);

		if (tableManager->get_fileTable(table_id)->getFd() == -1){
			tableManager->get_fileTable(table_id)->setFd(-2);
			continue;
		}
		else if (close_file(tableManager->get_fileTable(table_id)->getFd()) < 0){
			return -1;
		}

		tableManager->get_fileTable(table_id)->setFd(-2);
	}
	
	delete[] bufHash;
	delete[] buf;
	delete bufStack;
	delete LRU_Head;
	delete LRU_Tail;
	delete tableManager;

	return 0;
}

int close_table(int table_id){
	flushBuf(table_id);
	
	int ret = close_file(tableManager->get_fileTable(table_id)->getFd());
	
	if (ret == 0){
		tableManager->get_fileTable(table_id)->setFd(-1);
		return ret;
	}
	else{
		return -1;
	}
}

int open_table(char* path) {
	if(buf == NULL || bufHash == NULL || bufStack == NULL || strlen(path) > MAX_PATH_LEN){
		return -1;
	}
	int fd = open_file(path);
	
	if (fd == -1){
		return -1;
	}

	struct stat st;
	stat(path, &st);
	int table_id;

	for(table_id = 1; table_id <= MAX_TABLE_NUM && tableManager->get_fileTable(table_id)->getFd() != -2; table_id++){
		if (tableManager->get_fileTable(table_id)->getInodeNum() == st.st_ino){
			break;
		}
	}
	if (table_id > MAX_TABLE_NUM){
		close_file(fd);
		return -1;
	}
	else if (tableManager->get_fileTable(table_id)->getFd() == -2){
		tableManager->get_fileTable(table_id)->setInodeNum(st.st_ino);
		tableManager->get_fileTable(table_id)->setFd(fd);
	}
	else{
		tableManager->get_fileTable(table_id)->setFd(fd);
	}
	
	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	if(headerPage.num_page == 0){
		headerPage.num_page = 1;
		headerPage.free_pageNum = 0;
		headerPage.root_pageNum = 0;
		buffer_write_page(table_id, 0, &headerPage);
	}
	else{
		buffer_complete_read_without_write(table_id, 0);
	}
	return table_id;
}

int db_find(int table_id, int64_t key, char* ret_val) {
	if (tableManager->get_fileTable(table_id)->getFd() < 0){
		return -1;
	}
	HeaderPage headerPage;
	LeafPage target_leafPage;

	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);
	if (headerPage.root_pageNum == 0) {
		return -1;
	}
	pagenum_t leafPageNum = find_leafPage(table_id, headerPage.root_pageNum, key);
	buffer_read_page(table_id, leafPageNum, &target_leafPage);
	buffer_complete_read_without_write(table_id, leafPageNum);

	int i = search_index_location_leaf(target_leafPage.record, key, target_leafPage.num_keys);
	if (i == -1) {
		return -1;
	}
	strcpy(ret_val, target_leafPage.record[i].value);

	return 0;
}

pagenum_t find_leafPage(int table_id, pagenum_t root_pageNum, int64_t key) {
	pagenum_t LeafPage_num;
	InternalPage targetPage;
	int area;
	buffer_read_page(table_id, root_pageNum, &targetPage);
	buffer_complete_read_without_write(table_id, root_pageNum);
	
	if (targetPage.isLeaf == 1) {
		return root_pageNum;
	}
	
	while(targetPage.isLeaf != 1) {
		area = search_index_area_internal(targetPage.node, key, targetPage.num_keys);
		if (area == -1){
			LeafPage_num = targetPage.farLeft_pageNum;
		}
		LeafPage_num = targetPage.node[area].pageNum;
		buffer_read_page(table_id, LeafPage_num, &targetPage);
		buffer_complete_read_without_write(table_id, LeafPage_num);
	}
	return LeafPage_num;
}

int search_index_area_internal(Node* arr, int64_t key, int num_key){
    int start = 0;
	int end = num_key - 1;
	int i = (start+end)/2;
	
	if (num_key == 1){
		if(arr[0].key <= key){
			return 0;
		}
		else{
			return -1;
		}
	}

	if (num_key == 2){
		if(arr[1].key <= key){
			return 1;
		}
		else if(arr[0].key <= key){
			return 0;
		}
		else{
			return -1;
		}
	}

	while(start <= end){
		if (i+1 == num_key || i-1 == -1){
			break;
		}
		else if (arr[i].key <= key && key < arr[i+1].key){
			return i;
		}
		else if(arr[i-1].key <= key && key < arr[i].key){
			return i - 1;
		}
		else if(arr[i].key < key){
			start = i + 1;
		}
		else{
			end = i - 1;
		}
		i = (start+end)/2;
	}
	if (arr[i].key <= key){
		return i;
	}
	return -1;
}

int search_index_area_leaf(Record* arr, int64_t key, int num_key){
    int start = 0;
	int end = num_key - 1;
	int i = (start+end)/2;

	if (num_key == 1){
		if(arr[0].key <= key){
			return 0;
		}
		else{
			return -1;
		}
	}

	if (num_key == 2){
		if(arr[1].key <= key){
			return 1;
		}
		else if(arr[0].key <= key){
			return 0;
		}
		else{
			return -1;
		}
	}

	while(start <= end){
		if (i+1 == num_key || i-1 == -1){
			break;
		}
		else if (arr[i].key <= key && key < arr[i+1].key){
			return i;
		}
		else if(arr[i-1].key <= key && key < arr[i].key){
			return i - 1;
		}
		else if(arr[i].key < key){
			start = i + 1;
		}
		else{
			end = i - 1;
		}
		i = (start+end)/2;
	}
	if (arr[i].key <= key){
		return i;
	}
	return -1;
}

int search_index_location_internal(Node* arr, int64_t key, int num_key){
	int start = 0;
	int end = num_key - 1;
	int i = (start+end)/2;

	while(start <= end){
		if (arr[i].key == key){
			return i;
		}
		else if(arr[i].key < key){
			start = i + 1;
		}
		else{
			end = i - 1;
		}
		i = (start+end)/2;
	}
	return -1;
}

int search_index_location_leaf(Record* arr, int64_t key, int num_key){
	int start = 0;
	int end = num_key - 1;
	int i = (start+end)/2;

	while(start <= end){
		if (arr[i].key == key){
			return i;
		}
		else if(arr[i].key < key){
			start = i + 1;
		}
		else{
			end = i - 1;
		}
		i = (start+end)/2;
	}
	return -1;
}

int db_insert(int table_id, int64_t key, char* value){
	if(tableManager->get_fileTable(table_id)->getFd() < 0){
		return -1;
	}

	char ret_val[120];
	if (db_find(table_id, key,ret_val) == 0){
		return -1;
	}
	
	Record new_record = make_record(key, value);

	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);
	if (headerPage.root_pageNum == 0){
		make_new_tree(table_id, new_record);
		return 0;
	}

	pagenum_t leafPageNum = find_leafPage(table_id, headerPage.root_pageNum, key);

	LeafPage leafPage;
	buffer_read_page(table_id, leafPageNum, &leafPage);

	if (leafPage.num_keys < MAX_KEY_LEAF){
		int area = search_index_area_leaf(leafPage.record, new_record.key, leafPage.num_keys) + 1;
		int i;
		for(i = leafPage.num_keys; i > area; i--){
			leafPage.record[i] = leafPage.record[i-1];
		}
		leafPage.record[i] = new_record;
		leafPage.num_keys++;

		buffer_write_page(table_id, leafPageNum, &leafPage);
	}
	else{
		buffer_complete_read_without_write(table_id, leafPageNum);
		insert_into_leafPage_after_splitting(table_id, leafPageNum, new_record);
	}
	
	return 0;
}

Record make_record(int64_t key, char* value){
	Record record;
	record.key = key;
	strcpy(record.value, value);
	return record;
}

pagenum_t make_new_tree(int table_id, Record new_record){
	pagenum_t rootPagenum = buffer_alloc_page(table_id);
	
	HeaderPage headerPage;
	LeafPage rootPage;

	buffer_read_page(table_id, 0, &headerPage);
	buffer_read_page(table_id, rootPagenum, &rootPage);

	rootPage.isLeaf = 1;
	rootPage.num_keys = 1;
	rootPage.parent_pageNum = 0;
	rootPage.record[0] = new_record;
	rootPage.sibling_pageNum = 0;

	headerPage.root_pageNum = rootPagenum;

	buffer_write_page(table_id, 0, &headerPage);
	buffer_write_page(table_id, rootPagenum, &rootPage);

	return rootPagenum;
}

pagenum_t insert_into_leafPage_after_splitting(int table_id, pagenum_t splitted_leafPageNum, Record new_record){
	pagenum_t new_paegNum = buffer_alloc_page(table_id);
	
	LeafPage splitted_leafPage, new_leafPage;
	buffer_read_page(table_id, splitted_leafPageNum, &splitted_leafPage);
	buffer_read_page(table_id, new_paegNum, &new_leafPage);
	
	new_leafPage.isLeaf = 1;
	new_leafPage.num_keys = 0;
	new_leafPage.parent_pageNum = splitted_leafPage.parent_pageNum;
	new_leafPage.sibling_pageNum = splitted_leafPage.sibling_pageNum;

	new_leafPage.sibling_pageNum = splitted_leafPage.sibling_pageNum;
	splitted_leafPage.sibling_pageNum = new_paegNum;

	int area = search_index_area_leaf(splitted_leafPage.record, new_record.key, splitted_leafPage.num_keys) + 1;

	int num_tmparr = splitted_leafPage.num_keys + 1;
	Record* tmparr = new Record[num_tmparr];
	for(int i = 0, j = 0; i < splitted_leafPage.num_keys; i++, j++){
		if(j == area){
			j++;
		}
		tmparr[j] = splitted_leafPage.record[i];
	}
	tmparr[area] = new_record;

	int split = cut(splitted_leafPage.num_keys);

	splitted_leafPage.num_keys = 0;

	for(int i = 0; i < split; i++){
		splitted_leafPage.record[i] = tmparr[i];
		splitted_leafPage.num_keys++;
	}
	for(int i = split, j = 0; i < num_tmparr; i++, j++){
		new_leafPage.record[j] = tmparr[i];
		new_leafPage.num_keys++;
	}
	delete[] tmparr;

	for(int i = splitted_leafPage.num_keys; i < 31; i++){
		splitted_leafPage.record[i] = {0};
	}
	for(int i = new_leafPage.num_keys; i < 31; i++){
		new_leafPage.record[i] = {0};
	}

	buffer_write_page(table_id, splitted_leafPageNum, &splitted_leafPage);
	buffer_write_page(table_id, new_paegNum, &new_leafPage);
	
	insert_into_parent(table_id, splitted_leafPageNum, new_paegNum, new_leafPage.record[0].key);

	return splitted_leafPageNum;
}

int cut(int len){
	if (len%2 == 0){
		return len/2;
	}
	else{
		return len/2 + 1;
	}
}

pagenum_t insert_into_parent(int table_id, pagenum_t leftPageNum, pagenum_t rightPageNum, int64_t key){
	InternalPage leftPage;
	buffer_read_page(table_id, leftPageNum, &leftPage);

	pagenum_t parentPageNum = leftPage.parent_pageNum;

	if(parentPageNum == 0){
		pagenum_t new_rootPagNum = buffer_alloc_page(table_id);

		HeaderPage headerPage;
		buffer_read_page(table_id, 0, &headerPage);

		InternalPage rightPage, rootPage;
		buffer_read_page(table_id, rightPageNum, &rightPage);
		buffer_read_page(table_id, new_rootPagNum, &rootPage);

		rootPage.farLeft_pageNum = leftPageNum;
		rootPage.node[0].pageNum = rightPageNum;
		rootPage.node[0].key = key;
		rootPage.isLeaf = 0;
		rootPage.num_keys = 1;
		rootPage.parent_pageNum = 0;
		leftPage.parent_pageNum = new_rootPagNum;
		rightPage.parent_pageNum = new_rootPagNum;
		headerPage.root_pageNum = new_rootPagNum;

		buffer_write_page(table_id, 0, &headerPage);
		buffer_write_page(table_id, new_rootPagNum, &rootPage);
		buffer_write_page(table_id, leftPageNum, &leftPage);
		buffer_write_page(table_id, rightPageNum, &rightPage);

		return new_rootPagNum;
	}

	InternalPage parentPage;
	buffer_read_page(table_id, parentPageNum, &parentPage);

	int area = search_index_area_internal(parentPage.node, key, parentPage.num_keys) + 1;
	if(parentPage.num_keys < MAX_KEY_INTERNAL){
		for(int i = parentPage.num_keys; i > area; i--){
			parentPage.node[i] = parentPage.node[i-1];
		}
		parentPage.node[area].key = key;
		parentPage.node[area].pageNum = rightPageNum;
		parentPage.num_keys++;
		buffer_write_page(table_id, parentPageNum, &parentPage);
		buffer_complete_read_without_write(table_id, leftPageNum);
		
		return parentPageNum;
	}
	else{
		buffer_complete_read_without_write(table_id, parentPageNum);
		buffer_complete_read_without_write(table_id, leftPageNum);
		insert_into_internalPage_after_splitting(table_id, parentPageNum, area, key, rightPageNum);
		return parentPageNum;
	}
}


pagenum_t insert_into_internalPage_after_splitting(int table_id, pagenum_t splitted_internalPageNum, int area, int64_t key, pagenum_t rightPageNum){
	pagenum_t new_internalPageNum = buffer_alloc_page(table_id);

	InternalPage new_internalPage, splitted_internalPage;
	buffer_read_page(table_id, new_internalPageNum, &new_internalPage);
	buffer_read_page(table_id, splitted_internalPageNum, &splitted_internalPage);

	new_internalPage.isLeaf = 0;
	new_internalPage.num_keys = 0;
	new_internalPage.parent_pageNum = splitted_internalPage.parent_pageNum;

	int num_tmparr = splitted_internalPage.num_keys + 1;
	Node* tmparr = new Node[num_tmparr];
	for(int i = 0, j = 0; i < splitted_internalPage.num_keys; i++, j++){
		if(j == area){
			j++;
		}
		tmparr[j] = splitted_internalPage.node[i];
	}
	tmparr[area].key = key;
	tmparr[area].pageNum = rightPageNum;

	int split = cut(splitted_internalPage.num_keys+1);
	splitted_internalPage.num_keys = 0;

	int i, j, k_prime;
	for(i = 0; i < split - 1; i++){
		splitted_internalPage.node[i] = tmparr[i];
		splitted_internalPage.num_keys++;
	}

	k_prime = tmparr[i].key;
	new_internalPage.farLeft_pageNum = tmparr[i].pageNum;

	for(++i, j = 0; i < num_tmparr; i++, j++){
		new_internalPage.node[j] = tmparr[i];
		new_internalPage.num_keys++;
	}

	delete[] tmparr;

	for(i = splitted_internalPage.num_keys; i < 31; i++){
		splitted_internalPage.node[i] = {0};
	}
	for(i = new_internalPage.num_keys; i < 31; i++){
		new_internalPage.node[i] = {0};
	}


	InternalPage childPage;
	pagenum_t childPageNum;

	childPageNum = new_internalPage.farLeft_pageNum;
	buffer_read_page(table_id, childPageNum, &childPage);
	childPage.parent_pageNum = new_internalPageNum;
	buffer_write_page(table_id, childPageNum, &childPage);
	for(i = 0; i < new_internalPage.num_keys; i++){
		childPageNum = new_internalPage.node[i].pageNum;
		buffer_read_page(table_id, childPageNum, &childPage);
		childPage.parent_pageNum = new_internalPageNum;
		buffer_write_page(table_id, childPageNum, &childPage);
	}

	buffer_write_page(table_id, splitted_internalPageNum, &splitted_internalPage);
	buffer_write_page(table_id, new_internalPageNum, &new_internalPage);

	insert_into_parent(table_id, splitted_internalPageNum, new_internalPageNum, k_prime);

	return splitted_internalPageNum;
}

int db_delete(int table_id, int64_t key){
	if (tableManager->get_fileTable(table_id)->getFd() < 0){
		return -1;
	}

	pagenum_t key_leafPageNum;
	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);

	if(headerPage.root_pageNum == 0){
		return -1;
	}

	char ret_val[120];
	if(db_find(table_id, key, ret_val) == 0){
		key_leafPageNum = find_leafPage(table_id, headerPage.root_pageNum, key);
		delete_entry(table_id, headerPage.root_pageNum, key_leafPageNum, key);
		return 0;
	}
	else{
		return -1;
	}
}

pagenum_t delete_entry(int table_id, pagenum_t rootPageNum, pagenum_t targetPageNum, int64_t key){
	targetPageNum = remove_entry_from_page(table_id, targetPageNum, key);

	if (targetPageNum == rootPageNum){
		return adjust_root(table_id, targetPageNum);
	}

	InternalPage targetPage, parentPage;
	buffer_read_page(table_id, targetPageNum, &targetPage);
	buffer_complete_read_without_write(table_id, targetPageNum);

	if (targetPage.num_keys > 0){
		return rootPageNum;
	}

	int targetPage_index, neighborPage_index, k_prime_index;
	int64_t k_prime;
	pagenum_t neighborPageNum;
	pagenum_t parentPageNum = targetPage.parent_pageNum;

	buffer_read_page(table_id, parentPageNum, &parentPage);
	buffer_complete_read_without_write(table_id, parentPageNum);

	targetPage_index = search_index_area_internal(parentPage.node, key, parentPage.num_keys);
	neighborPage_index = targetPage_index == -1 ? 0 : targetPage_index - 1;
	k_prime_index = targetPage_index == -1 ? 0 : targetPage_index;
	k_prime = parentPage.node[k_prime_index].key;
	neighborPageNum = neighborPage_index == -1 ? parentPage.farLeft_pageNum : parentPage.node[neighborPage_index].pageNum;

	InternalPage neighborPage;
	buffer_read_page(table_id, neighborPageNum, &neighborPage);
	buffer_complete_read_without_write(table_id, neighborPageNum);

	if(neighborPage.isLeaf == 0 && neighborPage.num_keys == MAX_KEY_INTERNAL){
		return redistribute(table_id, targetPageNum, neighborPageNum, targetPage_index, k_prime, k_prime_index);
	}
	return delayed_merge(table_id, targetPageNum, neighborPageNum, targetPage_index, k_prime);
}

pagenum_t remove_entry_from_page(int table_id, pagenum_t targetPageNum, int64_t key){
	int i = 0;

	InternalPage targetPage;
	buffer_read_page(table_id, targetPageNum, &targetPage);

	if (targetPage.isLeaf == 1){
		LeafPage targetPage_leaf;
		memcpy(&targetPage_leaf, &targetPage, PAGE_SIZE);
		
		i = search_index_location_leaf(targetPage_leaf.record, key, targetPage_leaf.num_keys);
	
		for(++i; i < targetPage_leaf.num_keys; i++){
			targetPage_leaf.record[i - 1] = targetPage_leaf.record[i];
		}
		targetPage_leaf.num_keys--;
		for(i = targetPage_leaf.num_keys; i < MAX_KEY_LEAF; i++){
			targetPage_leaf.record[i] = {0};
		}
		buffer_write_page(table_id, targetPageNum, &targetPage_leaf);
		return targetPageNum;
	}

	i = search_index_location_internal(targetPage.node, key, targetPage.num_keys);
	for(++i; i < targetPage.num_keys; i++){
		targetPage.node[i - 1] = targetPage.node[i];
	}
	targetPage.num_keys--;
	for(i = targetPage.num_keys; i < MAX_KEY_INTERNAL; i++){
		targetPage.node[i] = {0};
	}
	buffer_write_page(table_id, targetPageNum, &targetPage);
	return targetPageNum;
}

pagenum_t adjust_root(int table_id,pagenum_t rootPagNum){
	InternalPage rootPage, new_rootPage;
	HeaderPage headerPage;
	buffer_read_page(table_id, rootPagNum, &rootPage);
	buffer_complete_read_without_write(table_id, rootPagNum);

	pagenum_t new_rootPageNum;

	if (rootPage.num_keys > 0){
		return rootPagNum;
	}

	if (rootPage.isLeaf == 0){
		new_rootPageNum = rootPage.farLeft_pageNum;
		buffer_free_page(table_id, rootPagNum);

		buffer_read_page(table_id, 0, &headerPage);
		buffer_read_page(table_id, new_rootPageNum, &new_rootPage);

		new_rootPage.parent_pageNum = 0;
		headerPage.root_pageNum = new_rootPageNum;

		buffer_write_page(table_id, 0, &headerPage);
		buffer_write_page(table_id, new_rootPageNum, &new_rootPage);

		return new_rootPageNum;
	}
	else{
		buffer_free_page(table_id, rootPagNum);
		buffer_read_page(table_id, 0, &headerPage);
		headerPage.root_pageNum = 0;
		buffer_write_page(table_id, 0, &headerPage);

		return 0;
	}

}

pagenum_t delayed_merge(int table_id, pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime){
	InternalPage targetPage, parentPage;
	buffer_read_page(table_id, targetPageNum, &targetPage);
	buffer_complete_read_without_write(table_id, targetPageNum);

	pagenum_t parentPageNum = targetPage.parent_pageNum;
	buffer_read_page(table_id, parentPageNum, &parentPage);

	if (targetPage.isLeaf == 1){
		LeafPage targetPage_leaf, neighborPage_leaf;
		memcpy(&targetPage_leaf, &targetPage, PAGE_SIZE);
		buffer_read_page(table_id, neighborPageNum, &neighborPage_leaf);

		if (targetPage_index != -1){
			neighborPage_leaf.sibling_pageNum = targetPage_leaf.sibling_pageNum;
			buffer_write_page(table_id, neighborPageNum, &neighborPage_leaf);
			buffer_complete_read_without_write(table_id, parentPageNum);
		}
		else{
			InternalPage rootPage;
			HeaderPage headerPage;
			buffer_read_page(table_id, 0, &headerPage);
			buffer_read_page(table_id, headerPage.root_pageNum, &rootPage);
			buffer_complete_read_without_write(table_id, 0);
			buffer_complete_read_without_write(table_id, headerPage.root_pageNum);

			int area_targetSubTree = search_index_area_internal(rootPage.node, k_prime, rootPage.num_keys);
			if (area_targetSubTree > -1){
				pagenum_t pagenum_leftSubTree = area_targetSubTree == 0 ? rootPage.farLeft_pageNum : rootPage.node[area_targetSubTree - 1].pageNum;
				InternalPage leftSubTreePage;
				buffer_read_page(table_id, pagenum_leftSubTree, &leftSubTreePage);
				while(leftSubTreePage.isLeaf != 1){
					buffer_complete_read_without_write(table_id, pagenum_leftSubTree);
					pagenum_leftSubTree = leftSubTreePage.node[leftSubTreePage.num_keys - 1].pageNum;
					buffer_read_page(table_id, pagenum_leftSubTree, &leftSubTreePage);
				}
				LeafPage leftSubTreePage_leaf;
				memcpy(&leftSubTreePage_leaf, &leftSubTreePage, PAGE_SIZE);
				leftSubTreePage_leaf.sibling_pageNum = targetPage_leaf.sibling_pageNum;
				buffer_write_page(table_id, pagenum_leftSubTree, &leftSubTreePage_leaf);
			}

			parentPage.farLeft_pageNum = neighborPageNum;
			buffer_write_page(table_id, parentPageNum, &parentPage);
			buffer_complete_read_without_write(table_id, neighborPageNum);
		}
		buffer_free_page(table_id, targetPageNum);
	}
	else{
		InternalPage neighborPage;
		buffer_read_page(table_id, neighborPageNum, &neighborPage);

		if (targetPage_index != -1){
			neighborPage.node[neighborPage.num_keys].key = k_prime;
			neighborPage.node[neighborPage.num_keys].pageNum = targetPage.farLeft_pageNum;
			neighborPage.num_keys++;

			buffer_free_page(table_id, targetPageNum);

			InternalPage childPage;
			pagenum_t childPageNum = neighborPage.node[neighborPage.num_keys - 1].pageNum;
			buffer_read_page(table_id, childPageNum, &childPage);
			childPage.parent_pageNum = neighborPageNum;
			
			buffer_write_page(table_id, childPageNum, &childPage);
			buffer_write_page(table_id, neighborPageNum, &neighborPage);
			buffer_complete_read_without_write(table_id, parentPageNum);
		}
		else{
			for(int i = neighborPage.num_keys; i > 0; i--){
				neighborPage.node[i] = neighborPage.node[i-1];
			}
			neighborPage.node[0].key = k_prime;
			neighborPage.node[0].pageNum = neighborPage.farLeft_pageNum;
			neighborPage.farLeft_pageNum = targetPage.farLeft_pageNum;
			parentPage.farLeft_pageNum = neighborPageNum;
			neighborPage.num_keys++;

			InternalPage childPage;
			pagenum_t childPageNum = neighborPage.farLeft_pageNum;
			buffer_read_page(table_id, childPageNum, &childPage);
			childPage.parent_pageNum = neighborPageNum;

			buffer_write_page(table_id, childPageNum, &childPage);
			buffer_write_page(table_id, neighborPageNum, &neighborPage);
			buffer_write_page(table_id, parentPageNum, &parentPage);
		}
	}

	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);

	delete_entry(table_id, headerPage.root_pageNum, parentPageNum, k_prime);
	return targetPageNum;
}

pagenum_t redistribute(int table_id, pagenum_t targetPageNum, pagenum_t neighborPagNum, int targetPage_index, int64_t k_prime, int k_prime_index){
	InternalPage targetPage, neighborPage, parentPage;
	buffer_read_page(table_id, targetPageNum, &targetPage);
	buffer_read_page(table_id, neighborPagNum, &neighborPage);
	buffer_read_page(table_id, targetPage.parent_pageNum, &parentPage);

	if (targetPage_index == -1){
		targetPage.node[0].key = k_prime;
		targetPage.node[0].pageNum = neighborPage.farLeft_pageNum;
		neighborPage.farLeft_pageNum = neighborPage.node[0].pageNum;
		parentPage.node[k_prime_index].key = neighborPage.node[0].key;

		for(int i = 1; i < neighborPage.num_keys; i++){
			neighborPage.node[i - 1] = neighborPage.node[i];
		}

		targetPage.num_keys++;
		neighborPage.num_keys--;
		for(int i = neighborPage.num_keys; i < MAX_KEY_INTERNAL; i++){
			neighborPage.node[i] = {0};
		}

		InternalPage childPage;
		pagenum_t childPageNum = targetPage.node[0].pageNum;
		buffer_read_page(table_id, childPageNum, &childPage);
		childPage.parent_pageNum = targetPageNum;

		buffer_write_page(table_id, childPageNum, &childPage);
		buffer_write_page(table_id, neighborPagNum, &neighborPage);
		buffer_write_page(table_id, targetPageNum, &targetPage);
		buffer_write_page(table_id, targetPage.parent_pageNum, &parentPage);
	}
	else{
		targetPage.node[0].key = k_prime;
		targetPage.node[0].pageNum = targetPage.farLeft_pageNum;
		targetPage.farLeft_pageNum = neighborPage.node[neighborPage.num_keys - 1].pageNum;
		parentPage.node[k_prime_index].key = neighborPage.node[neighborPage.num_keys - 1].key;

		targetPage.num_keys++;
		neighborPage.num_keys--;
		for(int i = neighborPage.num_keys; i < MAX_KEY_INTERNAL; i++){
			neighborPage.node[i] = {0};
		}

		InternalPage childPage;
		pagenum_t childPageNum = targetPage.farLeft_pageNum;
		buffer_read_page(table_id, childPageNum, &childPage);
		childPage.parent_pageNum = targetPageNum;

		buffer_write_page(table_id, childPageNum, &childPage);
		buffer_write_page(table_id, neighborPagNum, &neighborPage);
		buffer_write_page(table_id, targetPageNum, &targetPage);
		buffer_write_page(table_id, targetPage.parent_pageNum, &parentPage);
	}
	return targetPageNum;
}

treeNode* queue;

void print_tree(int table_id){
	if(tableManager->get_fileTable(table_id)->getFd() < 0){
		cout << "Table is closed" << endl;
		return;
	}

	treeNode* n = new treeNode;

	int i = 0;
	int rank = 0;
	int new_rank = 0;

	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);

	if(headerPage.root_pageNum == 0){
		cout << "empty tree" << endl;
		return;
	}
	
	queue = NULL;
	enqueue(headerPage.root_pageNum);

	while(queue != NULL){
		*n = dequeue();
		pagenum_t targetPageNum = n->pagenum;
		
		InternalPage targetPage;
		buffer_read_page(table_id, targetPageNum, &targetPage);
		buffer_complete_read_without_write(table_id, targetPageNum);
		LeafPage targetPage_leaf;

		if(targetPage.parent_pageNum != 0){
			InternalPage parentPage;
			buffer_read_page(table_id, targetPage.parent_pageNum, &parentPage);
			buffer_complete_read_without_write(table_id, targetPage.parent_pageNum);
			if (parentPage.farLeft_pageNum == targetPageNum){
				new_rank = path_to_root(table_id, headerPage.root_pageNum, targetPageNum);
				if(new_rank != rank){
					rank = new_rank;
					cout << endl;
				}
			}
		}
		if (targetPage.isLeaf == 1){
			memcpy(&targetPage_leaf, &targetPage, PAGE_SIZE);
			for(i = 0; i <targetPage_leaf.num_keys; i++){
				cout << targetPage_leaf.record[i].key << " ";
			}
		}
		else{
			enqueue(targetPage.farLeft_pageNum);
			for(i = 0; i < targetPage.num_keys; i++){
				cout << targetPage.node[i].key << " ";
				enqueue(targetPage.node[i].pageNum);
			}
		}
		cout << "| ";
	}
	delete n;
	cout << endl;
}

void enqueue(pagenum_t new_pagenum){
	treeNode* c;
	treeNode* new_treenode = new treeNode;
	
	new_treenode->pagenum = new_pagenum;
	new_treenode->next = NULL;
	
	if (queue == NULL){
		queue = new_treenode;
	}
	else{
		c = queue;
		while(c->next != NULL){
			c = c->next;
		}
		c->next = new_treenode;
		new_treenode->next = NULL;
	}
}

treeNode dequeue(void){
    treeNode n = *queue;
	treeNode* tmp = queue;
    queue = queue->next;
	delete tmp;
    n.next = NULL;
    return n;
}

int path_to_root(int table_id, pagenum_t rootPageNUm, pagenum_t childPageNum){
	int length = 0;
	pagenum_t c = childPageNum;
	while(c != rootPageNUm){
		InternalPage child;
		buffer_read_page(table_id, c, &child);
		buffer_complete_read_without_write(table_id, c);
		c = child.parent_pageNum;
		length++;
	}
	return length;
}

void scan(int table_id){
	HeaderPage headerPage;
	buffer_read_page(table_id, 0, &headerPage);
	buffer_complete_read_without_write(table_id, 0);

	InternalPage rootPage;
	buffer_read_page(table_id, headerPage.root_pageNum, &rootPage);
	buffer_complete_read_without_write(table_id, headerPage.root_pageNum);

	if (rootPage.isLeaf == 1){
		LeafPage rootPage_leaf;
		memcpy(&rootPage_leaf, &rootPage, PAGE_SIZE);
		for(int i = 0; i < rootPage.num_keys; i++){
			cout << rootPage_leaf.record[i].key << " ";
		}
		cout << endl;
		return;
	}
	pagenum_t farleftPagenum = rootPage.farLeft_pageNum;
	InternalPage internalPage;
	buffer_read_page(table_id, farleftPagenum, &internalPage);
	buffer_complete_read_without_write(table_id, farleftPagenum);
	while(internalPage.isLeaf != 1){
		farleftPagenum = internalPage.farLeft_pageNum;
		buffer_read_page(table_id, farleftPagenum, &internalPage);
		buffer_complete_read_without_write(table_id, farleftPagenum);
	}
	LeafPage leafPage;
	memcpy(&leafPage, &internalPage, PAGE_SIZE);
	
	pagenum_t sibling_pageNum = leafPage.sibling_pageNum; 

	while(sibling_pageNum != 0){
		for(int i = 0; i < leafPage.num_keys; i++){
			cout << leafPage.record[i].key << " ";
		}
		buffer_read_page(table_id, sibling_pageNum, &leafPage);
		buffer_complete_read_without_write(table_id, sibling_pageNum);
		sibling_pageNum = leafPage.sibling_pageNum;
	}
	for(int i = 0; i < leafPage.num_keys; i++){
			cout << leafPage.record[i].key << " ";
	}
	cout << endl;
}