#pragma once
#include <stdint.h>

int init_db(int num_buf, int flag, int log_num, char* log_path, char* logmsg_path);
int shutdown_db();

int open_table(char* path);
int close_table(int table_id);

int db_find(int table_id, int64_t key, char* ret_val, int trx_id);
int db_update(int table_id, int64_t key, char* values, int trx_id);

int db_insert(int table_id, int64_t key, char* value);
int db_delete(int table_id, int64_t key);

int trx_begin();
int trx_commit(int trx_id);
int trx_abort(int trx_id);