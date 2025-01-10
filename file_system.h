#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <stdbool.h>

#define MAX_FILENAME 50
#define MAX_FILES 100
#define MAX_RECORDS 1000

// Colors for visualization
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

typedef struct {
    int id;
    char data[50];
    bool is_deleted;
} Record;

typedef struct {
    char filename[MAX_FILENAME];
    int block_count;
    int record_count;
    int first_block;
    bool is_contiguous;
    bool is_sorted;
} Metadata;

typedef struct {
    int next_block;
    Record *records;
    int record_count;
    char owner_file[MAX_FILENAME];
} Block;

typedef struct {
    Block *blocks;
    bool *allocation_table;
    int total_blocks;
    int block_size;
    Metadata *file_metadata;
    int file_count;
} FileSystem;

// Function declarations
FileSystem *init_filesystem(int total_blocks, int block_size);
void free_filesystem(FileSystem *fs);
int create_file(FileSystem *fs, const char *filename, int record_count, bool is_contiguous, bool is_sorted);
int insert_record(FileSystem *fs, const char *filename, Record record);
int search_record(FileSystem *fs, const char *filename, int id, int *block_num, int *offset);
void delete_record_logical(FileSystem *fs, const char *filename, int id);
void delete_record_physical(FileSystem *fs, const char *filename, int id);
void defragment_file(FileSystem *fs, const char *filename);
void rename_file(FileSystem *fs, const char *old_name, const char *new_name);
void delete_file(FileSystem *fs, const char *filename);
void compact_memory(FileSystem *fs);
void clear_filesystem(FileSystem *fs);
void display_memory_state(FileSystem *fs);
void display_metadata(FileSystem *fs);
void generate_sample_data(FileSystem *fs, const char *filename);
void menu(FileSystem *fs);

#endif // FILE_SYSTEM_H

