#include <string.h>
#include "bpt.h"
#include <iostream>

using namespace std;

extern int fd;

int open_table(char* path) {
	open_file(path);
	
	HeaderPage headerPage;
	file_read_page(0, &headerPage);
	if(headerPage.num_page == 0){
		headerPage.num_page = 1;
		headerPage.free_pageNum = 0;
		headerPage.root_pageNum = 0;
		file_write_page(0, &headerPage);
	}

	if (fd == -1) {
		return -1;
	}
	return fd;
}

int db_find(int64_t key, char* ret_val) {
	if (fd == -1){
		cout << "Table is closed" << endl;
		return -1;
	}

	HeaderPage headerPage;
	LeafPage target_leafPage;

	file_read_page(0, &headerPage);
	if (headerPage.root_pageNum == 0) {
		return -1;
	}
	pagenum_t leafPageNum = find_leafPage(headerPage.root_pageNum, key);
	file_read_page(leafPageNum, &target_leafPage);
	int i = search_index_location_leaf(target_leafPage.record, key, target_leafPage.num_keys);
	if (i == -1) {
		return -1;
	}
	strcpy(ret_val, target_leafPage.record[i].value);
	return 0;
}

pagenum_t find_leafPage(pagenum_t root_pageNum, int64_t key) {
	pagenum_t LeafPage_num;
	InternalPage targetPage;
	int area;
	file_read_page(root_pageNum, &targetPage);
	
	if (targetPage.isLeaf == 1) {
		return root_pageNum;
	}

	while(targetPage.isLeaf != 1) {
		area = search_index_area_internal(targetPage.node, key, targetPage.num_keys);
		if (area == -1){
			LeafPage_num = targetPage.farLeft_pageNum;
		}
		LeafPage_num = targetPage.node[area].pageNum;
		file_read_page(LeafPage_num, &targetPage);
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

int db_insert(int64_t key, char* value){
	if(fd == -1){
		cout << "Table is closed" << endl;
		return -1;
	}

	char ret_val[120];
	if (db_find(key,ret_val) == 0){
		return -1;
	}
	
	Record new_record = make_record(key, value);

	HeaderPage headerPage;
	file_read_page(0, &headerPage);
	if (headerPage.root_pageNum == 0){
		make_new_tree(new_record);
		return 0;
	}

	pagenum_t leafPageNum = find_leafPage(headerPage.root_pageNum, key);
	LeafPage leafPage;
	file_read_page(leafPageNum, &leafPage);

	if (leafPage.num_keys < MAX_KEY_LEAF){
		int area = search_index_area_leaf(leafPage.record, new_record.key, leafPage.num_keys) + 1;
		int i;
		for(i = leafPage.num_keys; i > area; i--){
			leafPage.record[i] = leafPage.record[i-1];
		}
		leafPage.record[i] = new_record;
		leafPage.num_keys++;

		file_write_page(leafPageNum, &leafPage);
	}
	else{
		insert_into_leafPage_after_splitting(leafPageNum, new_record);
	}
	
	return 0;
}

Record make_record(int64_t key, char* value){
	Record record;
	record.key = key;
	strcpy(record.value, value);
	return record;
}

pagenum_t make_new_tree(Record new_record){
	pagenum_t rootPagenum = file_alloc_page();
	
	HeaderPage headerPage;
	LeafPage rootPage;

	file_read_page(0, &headerPage);
	file_read_page(rootPagenum, &rootPage);

	rootPage.isLeaf = 1;
	rootPage.num_keys = 1;
	rootPage.parent_pageNum = 0;
	rootPage.record[0] = new_record;
	rootPage.sibling_pageNum = 0;

	headerPage.root_pageNum = rootPagenum;

	file_write_page(0, &headerPage);
	file_write_page(rootPagenum, &rootPage);

	return rootPagenum;
}

pagenum_t insert_into_leafPage_after_splitting(pagenum_t splitted_leafPageNum, Record new_record){
	pagenum_t new_paegNum = file_alloc_page();

	LeafPage splitted_leafPage, new_leafPage;
	file_read_page(splitted_leafPageNum, &splitted_leafPage);
	file_read_page(new_paegNum, &new_leafPage);

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

	file_write_page(splitted_leafPageNum, &splitted_leafPage);
	file_write_page(new_paegNum, &new_leafPage);

	insert_into_parent(splitted_leafPageNum, new_paegNum, new_leafPage.record[0].key);

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

pagenum_t insert_into_parent(pagenum_t leftPageNum, pagenum_t rightPageNum, int64_t key){
	InternalPage leftPage;
	file_read_page(leftPageNum, &leftPage);

	pagenum_t parentPageNum = leftPage.parent_pageNum;

	if(parentPageNum == 0){
		pagenum_t new_rootPagNum = file_alloc_page();

		HeaderPage headerPage;
		file_read_page(0, &headerPage);

		InternalPage rightPage, rootPage;
		file_read_page(rightPageNum, &rightPage);
		file_read_page(new_rootPagNum, &rootPage);

		rootPage.farLeft_pageNum = leftPageNum;
		rootPage.node[0].pageNum = rightPageNum;
		rootPage.node[0].key = key;
		rootPage.isLeaf = 0;
		rootPage.num_keys = 1;
		rootPage.parent_pageNum = 0;
		leftPage.parent_pageNum = new_rootPagNum;
		rightPage.parent_pageNum = new_rootPagNum;
		headerPage.root_pageNum = new_rootPagNum;

		file_write_page(0, &headerPage);
		file_write_page(new_rootPagNum, &rootPage);
		file_write_page(leftPageNum, &leftPage);
		file_write_page(rightPageNum, &rightPage);

		return new_rootPagNum;
	}

	InternalPage parentPage;
	file_read_page(parentPageNum, &parentPage);

	int area = search_index_area_internal(parentPage.node, key, parentPage.num_keys) + 1;
	if(parentPage.num_keys < MAX_KEY_INTERNAL){
		for(int i = parentPage.num_keys; i > area; i--){
			parentPage.node[i] = parentPage.node[i-1];
		}
		parentPage.node[area].key = key;
		parentPage.node[area].pageNum = rightPageNum;
		parentPage.num_keys++;
		file_write_page(parentPageNum, &parentPage);
		
		return parentPageNum;
	}
	else{
		insert_into_internalPage_after_splitting(parentPageNum, area, key, rightPageNum);
		return parentPageNum;
	}
}


pagenum_t insert_into_internalPage_after_splitting(pagenum_t splitted_internalPageNum, int area, int64_t key, pagenum_t rightPageNum){
	pagenum_t new_internalPageNum = file_alloc_page();

	InternalPage new_internalPage, splitted_internalPage;
	file_read_page(new_internalPageNum, &new_internalPage);
	file_read_page(splitted_internalPageNum, &splitted_internalPage);

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

	file_write_page(splitted_internalPageNum, &splitted_internalPage);
	file_write_page(new_internalPageNum, &new_internalPage);

	InternalPage childPage;
	pagenum_t childPageNum;

	childPageNum = new_internalPage.farLeft_pageNum;
	file_read_page(childPageNum, &childPage);
	childPage.parent_pageNum = new_internalPageNum;
	file_write_page(childPageNum, &childPage);
	for(i = 0; i < new_internalPage.num_keys; i++){
		childPageNum = new_internalPage.node[i].pageNum;
		file_read_page(childPageNum, &childPage);
		childPage.parent_pageNum = new_internalPageNum;
		file_write_page(childPageNum, &childPage);
	}

	insert_into_parent(splitted_internalPageNum, new_internalPageNum, k_prime);

	return splitted_internalPageNum;
}

int db_delete(int64_t key){
	if (fd == -1){
		cout << "Table is closed" << endl;
		return -1;
	}

	pagenum_t key_leafPageNum;
	HeaderPage headerPage;
	file_read_page(0, &headerPage);

	if(headerPage.root_pageNum == 0){
		return -1;
	}

	char ret_val[120];
	if(db_find(key, ret_val) == 0){
		key_leafPageNum = find_leafPage(headerPage.root_pageNum, key);
		delete_entry(headerPage.root_pageNum, key_leafPageNum, key);
		return 0;
	}
	else{
		return -1;
	}
}

pagenum_t delete_entry(pagenum_t rootPageNum, pagenum_t targetPageNum, int64_t key){
	targetPageNum = remove_entry_from_page(targetPageNum, key);

	if (targetPageNum == rootPageNum){
		return adjust_root(targetPageNum);
	}

	InternalPage targetPage, parentPage;
	file_read_page(targetPageNum, &targetPage);
	if (targetPage.num_keys > 0){
		return rootPageNum;
	}

	int targetPage_index, neighborPage_index, k_prime_index;
	int64_t k_prime;
	pagenum_t neighborPageNum;
	pagenum_t parentPageNum = targetPage.parent_pageNum;

	file_read_page(parentPageNum, &parentPage);

	targetPage_index = search_index_area_internal(parentPage.node, key, parentPage.num_keys);
	neighborPage_index = targetPage_index == -1 ? 0 : targetPage_index - 1;
	k_prime_index = targetPage_index == -1 ? 0 : targetPage_index;
	k_prime = parentPage.node[k_prime_index].key;
	neighborPageNum = neighborPage_index == -1 ? parentPage.farLeft_pageNum : parentPage.node[neighborPage_index].pageNum;

	InternalPage neighborPage;
	file_read_page(neighborPageNum, &neighborPage);

	if(neighborPage.isLeaf == 0 && neighborPage.num_keys == MAX_KEY_INTERNAL){
		return redistribute(targetPageNum, neighborPageNum, targetPage_index, k_prime, k_prime_index);
	}
	return delayed_merge(targetPageNum, neighborPageNum, targetPage_index, k_prime);
}

pagenum_t remove_entry_from_page(pagenum_t targetPageNum, int64_t key){
	int i = 0;

	InternalPage targetPage;
	file_read_page(targetPageNum, &targetPage);

	if (targetPage.isLeaf == 1){
		LeafPage targetPage_leaf;
		file_read_page(targetPageNum, &targetPage_leaf);
		
		i = search_index_location_leaf(targetPage_leaf.record, key, targetPage_leaf.num_keys);
	
		for(++i; i < targetPage_leaf.num_keys; i++){
			targetPage_leaf.record[i - 1] = targetPage_leaf.record[i];
		}
		targetPage_leaf.num_keys--;
		for(i = targetPage_leaf.num_keys; i < MAX_KEY_LEAF; i++){
			targetPage_leaf.record[i] = {0};
		}
		file_write_page(targetPageNum, &targetPage_leaf);
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
	file_write_page(targetPageNum, &targetPage);
	return targetPageNum;
}

pagenum_t adjust_root(pagenum_t rootPagNum){
	InternalPage rootPage, new_rootPage;
	HeaderPage headerPage;
	file_read_page(rootPagNum, &rootPage);

	pagenum_t new_rootPageNum;

	if (rootPage.num_keys > 0){
		return rootPagNum;
	}

	if (rootPage.isLeaf == 0){
		new_rootPageNum = rootPage.farLeft_pageNum;
		file_free_page(rootPagNum);

		file_read_page(0, &headerPage);
		file_read_page(new_rootPageNum, &new_rootPage);

		new_rootPage.parent_pageNum = 0;
		headerPage.root_pageNum = new_rootPageNum;

		file_write_page(0, &headerPage);
		file_write_page(new_rootPageNum, &new_rootPage);

		return new_rootPageNum;
	}
	else{
		file_free_page(rootPagNum);
		file_read_page(0, &headerPage);
		headerPage.root_pageNum = 0;
		file_write_page(0, &headerPage);

		return 0;
	}

}

pagenum_t delayed_merge(pagenum_t targetPageNum, pagenum_t neighborPageNum, int targetPage_index, int64_t k_prime){
	InternalPage targetPage, parentPage;
	file_read_page(targetPageNum, &targetPage);

	pagenum_t parentPageNum = targetPage.parent_pageNum;
	file_read_page(parentPageNum, &parentPage);

	if (targetPage.isLeaf == 1){
		LeafPage targetPage_leaf, neighborPage_leaf;
		file_read_page(targetPageNum, &targetPage_leaf);
		file_read_page(neighborPageNum, &neighborPage_leaf);

		if (targetPage_index != -1){
			neighborPage_leaf.sibling_pageNum = targetPage_leaf.sibling_pageNum;
			file_write_page(neighborPageNum, &neighborPage_leaf);
		}
		else{
			InternalPage rootPage;
			HeaderPage headerPage;
			file_read_page(0, &headerPage);
			file_read_page(headerPage.root_pageNum, &rootPage);

			int area_targetSubTree = search_index_area_internal(rootPage.node, k_prime, rootPage.num_keys);
			if (area_targetSubTree > -1){
				pagenum_t pagenum_leftSubTree = area_targetSubTree == 0 ? rootPage.farLeft_pageNum : rootPage.node[area_targetSubTree - 1].pageNum;
				InternalPage leftSubTreePage;
				file_read_page(pagenum_leftSubTree, &leftSubTreePage);
				while(leftSubTreePage.isLeaf != 1){
					pagenum_leftSubTree = leftSubTreePage.node[leftSubTreePage.num_keys - 1].pageNum;
					file_read_page(pagenum_leftSubTree, &leftSubTreePage);
				}
				LeafPage leftSubTreePage_leaf;
				file_read_page(pagenum_leftSubTree, &leftSubTreePage_leaf);
				leftSubTreePage_leaf.sibling_pageNum = targetPage_leaf.sibling_pageNum;
				file_write_page(pagenum_leftSubTree, &leftSubTreePage_leaf);
			}

			parentPage.farLeft_pageNum = neighborPageNum;
			file_write_page(parentPageNum, &parentPage);
		}
		file_free_page(targetPageNum);
	}
	else{
		InternalPage neighborPage;
		file_read_page(neighborPageNum, &neighborPage);

		if (targetPage_index != -1){
			neighborPage.node[neighborPage.num_keys].key = k_prime;
			neighborPage.node[neighborPage.num_keys].pageNum = targetPage.farLeft_pageNum;
			neighborPage.num_keys++;

			file_free_page(targetPageNum);

			InternalPage childPage;
			pagenum_t childPageNum = neighborPage.node[neighborPage.num_keys - 1].pageNum;
			file_read_page(childPageNum, &childPage);
			childPage.parent_pageNum = neighborPageNum;
			
			file_write_page(childPageNum, &childPage);
			file_write_page(neighborPageNum, &neighborPage);
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
			file_read_page(childPageNum, &childPage);
			childPage.parent_pageNum = neighborPageNum;

			file_write_page(childPageNum, &childPage);
			file_write_page(neighborPageNum, &neighborPage);
			file_write_page(parentPageNum, &parentPage);
		}
	}

	HeaderPage headerPage;
	file_read_page(0, &headerPage);

	delete_entry(headerPage.root_pageNum, parentPageNum, k_prime);
	return targetPageNum;
}

pagenum_t redistribute(pagenum_t targetPageNum, pagenum_t neighborPagNum, int targetPage_index, int64_t k_prime, int k_prime_index){
	InternalPage targetPage, neighborPage, parentPage;
	file_read_page(targetPageNum, &targetPage);
	file_read_page(neighborPagNum, &neighborPage);
	file_read_page(targetPage.parent_pageNum, &parentPage);

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
		file_read_page(childPageNum, &childPage);
		childPage.parent_pageNum = targetPageNum;

		file_write_page(childPageNum, &childPage);
		file_write_page(neighborPagNum, &neighborPage);
		file_write_page(targetPageNum, &targetPage);
		file_write_page(targetPage.parent_pageNum, &parentPage);
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
		file_read_page(childPageNum, &childPage);
		childPage.parent_pageNum = targetPageNum;

		file_write_page(childPageNum, &childPage);
		file_write_page(neighborPagNum, &neighborPage);
		file_write_page(targetPageNum, &targetPage);
		file_write_page(targetPage.parent_pageNum, &parentPage);
	}
	return targetPageNum;
}

treeNode* queue;

void print_tree(){
	treeNode* n = new treeNode;

	int i = 0;
	int rank = 0;
	int new_rank = 0;

	HeaderPage headerPage;
	file_read_page(0, &headerPage);

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
		file_read_page(targetPageNum, &targetPage);
		LeafPage targetPage_leaf;

		if(targetPage.parent_pageNum != 0){
			InternalPage parentPage;
			file_read_page(targetPage.parent_pageNum, &parentPage);
			if (parentPage.farLeft_pageNum == targetPageNum){
				new_rank = path_to_root(headerPage.root_pageNum, targetPageNum);
				if(new_rank != rank){
					rank = new_rank;
					cout << endl;
				}
			}
		}
		if (targetPage.isLeaf == 1){
			file_read_page(targetPageNum, &targetPage_leaf);
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

int path_to_root(pagenum_t rootPageNUm, pagenum_t childPageNum){
	int length = 0;
	pagenum_t c = childPageNum;
	while(c != rootPageNUm){
		InternalPage child;
		file_read_page(c, &child);
		c = child.parent_pageNum;
		length++;
	}
	return length;
}

void scan(){
	HeaderPage headerPage;
	file_read_page(0, &headerPage);

	InternalPage rootPage;
	file_read_page(headerPage.root_pageNum, &rootPage);

	if (rootPage.isLeaf == 1){
		LeafPage rootPage_leaf;
		file_read_page(headerPage.root_pageNum, &rootPage_leaf);
		for(int i = 0; i < rootPage.num_keys; i++){
			cout << rootPage_leaf.record[i].key << " ";
		}
		cout << endl;
		return;
	}

	pagenum_t farleftPagenum = rootPage.farLeft_pageNum;
	InternalPage internalPage;
	file_read_page(farleftPagenum, &internalPage);
	while(internalPage.isLeaf != 1){
		farleftPagenum = internalPage.farLeft_pageNum;
		file_read_page(farleftPagenum, &internalPage);
	}
	LeafPage leafPage;
	file_read_page(farleftPagenum, &leafPage);

	while(leafPage.sibling_pageNum != 0){
		for(int i = 0; i < leafPage.num_keys; i++){
			cout << leafPage.record[i].key << " ";
		}
		file_read_page(leafPage.sibling_pageNum, &leafPage);
	}
	for(int i = 0; i < leafPage.num_keys; i++){
			cout << leafPage.record[i].key << " ";
	}
	cout << endl;
}