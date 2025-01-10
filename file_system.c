#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

FileSystem *init_filesystem(int total_blocks, int block_size)
{
    FileSystem *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!fs)
        return NULL;

    fs->total_blocks = total_blocks;
    fs->block_size = block_size;
    fs->file_count = 0;

    fs->allocation_table = (bool *)calloc(total_blocks, sizeof(bool));
    if (!fs->allocation_table)
    {
        free(fs);
        return NULL;
    }
    fs->allocation_table[0] = true; // Reserve first block for allocation table

    fs->blocks = (Block *)malloc(total_blocks * sizeof(Block));
    if (!fs->blocks)
    {
        free(fs->allocation_table);
        free(fs);
        return NULL;
    }
    for (int i = 0; i < total_blocks; i++)
    {
        fs->blocks[i].records = (Record *)malloc(block_size * sizeof(Record));
        if (!fs->blocks[i].records)
        {
            for (int j = 0; j < i; j++)
            {
                free(fs->blocks[j].records);
            }
            free(fs->blocks);
            free(fs->allocation_table);
            free(fs);
            return NULL;
        }
        fs->blocks[i].record_count = 0;
        fs->blocks[i].next_block = -1;
        strcpy(fs->blocks[i].owner_file, "");
    }

    fs->file_metadata = (Metadata *)malloc(MAX_FILES * sizeof(Metadata));
    if (!fs->file_metadata)
    {
        for (int i = 0; i < total_blocks; i++)
        {
            free(fs->blocks[i].records);
        }
        free(fs->blocks);
        free(fs->allocation_table);
        free(fs);
        return NULL;
    }

    return fs;
}

void free_filesystem(FileSystem *fs)
{
    if (!fs)
        return;
    for (int i = 0; i < fs->total_blocks; i++)
    {
        free(fs->blocks[i].records);
    }
    free(fs->blocks);
    free(fs->allocation_table);
    free(fs->file_metadata);
    free(fs);
    printf("Filesystem resources freed.\n");
}

int create_file(FileSystem *fs, const char *filename, int record_count, bool is_contiguous, bool is_sorted)
{
    if (fs->file_count >= MAX_FILES)
        return -1;

    int records_per_block = fs->block_size;
    int blocks_needed = (record_count + records_per_block - 1) / records_per_block;

    int free_blocks = 0;
    for (int i = 1; i < fs->total_blocks; i++)
    {
        if (!fs->allocation_table[i])
            free_blocks++;
    }

    if (free_blocks < blocks_needed)
    {
        printf("Not enough space. Would you like to compact memory? (y/n): ");
        char response;
        scanf(" %c", &response);
        if (response == 'y' || response == 'Y')
        {
            compact_memory(fs);
            free_blocks = 0;
            for (int i = 1; i < fs->total_blocks; i++)
            {
                if (!fs->allocation_table[i])
                    free_blocks++;
            }
            if (free_blocks < blocks_needed)
                return -1;
        }
        else
        {
            return -1;
        }
    }

    Metadata *meta = &fs->file_metadata[fs->file_count];
    strncpy(meta->filename, filename, MAX_FILENAME - 1);
    meta->filename[MAX_FILENAME - 1] = '\0';
    meta->block_count = blocks_needed;
    meta->record_count = record_count;
    meta->is_contiguous = is_contiguous;
    meta->is_sorted = is_sorted;

    if (is_contiguous)
    {
        int start_block = -1;
        int consecutive_free = 0;

        for (int i = 1; i < fs->total_blocks && start_block == -1; i++)
        {
            if (!fs->allocation_table[i])
            {
                consecutive_free = 1;
                for (int j = i + 1; j < fs->total_blocks && consecutive_free < blocks_needed; j++)
                {
                    if (!fs->allocation_table[j])
                        consecutive_free++;
                    else
                        break;
                }
                if (consecutive_free >= blocks_needed)
                    start_block = i;
            }
        }

        if (start_block == -1)
            return -1;

        meta->first_block = start_block;
        for (int i = 0; i < blocks_needed; i++)
        {
            fs->allocation_table[start_block + i] = true;
            strncpy(fs->blocks[start_block + i].owner_file, filename, MAX_FILENAME - 1);
            fs->blocks[start_block + i].owner_file[MAX_FILENAME - 1] = '\0';
        }
    }
    else
    {
        int prev_block = -1;
        int allocated = 0;

        for (int i = 1; i < fs->total_blocks && allocated < blocks_needed; i++)
        {
            if (!fs->allocation_table[i])
            {
                if (prev_block == -1)
                    meta->first_block = i;
                else
                    fs->blocks[prev_block].next_block = i;

                fs->allocation_table[i] = true;
                strncpy(fs->blocks[i].owner_file, filename, MAX_FILENAME - 1);
                fs->blocks[i].owner_file[MAX_FILENAME - 1] = '\0';
                prev_block = i;
                allocated++;
            }
        }
    }

    fs->file_count++;
    return 0;
}

int insert_record(FileSystem *fs, const char *filename, Record record)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
        return -1;

    Metadata *meta = &fs->file_metadata[file_index];
    int current_block = meta->first_block;
    while (current_block != -1)
    {
        if (fs->blocks[current_block].record_count < fs->block_size)
        {
            int insert_pos = fs->blocks[current_block].record_count;

            if (meta->is_sorted)
            {
                for (insert_pos = 0; insert_pos < fs->blocks[current_block].record_count; insert_pos++)
                {
                    if (fs->blocks[current_block].records[insert_pos].id > record.id)
                    {
                        break;
                    }
                }
                for (int i = fs->blocks[current_block].record_count; i > insert_pos; i--)
                {
                    fs->blocks[current_block].records[i] = fs->blocks[current_block].records[i - 1];
                }
            }

            fs->blocks[current_block].records[insert_pos] = record;
            fs->blocks[current_block].record_count++;
            return 0;
        }
        if (meta->is_contiguous)
        {
            current_block++;
            if (current_block >= meta->first_block + meta->block_count)
                break;
        }
        else
        {
            current_block = fs->blocks[current_block].next_block;
        }
    }
    return -1;
}

int search_record(FileSystem *fs, const char *filename, int id, int *block_num, int *offset)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
        return -1;

    Metadata *meta = &fs->file_metadata[file_index];
    int current_block = meta->first_block;

    while (current_block != -1)
    {
        for (int i = 0; i < fs->blocks[current_block].record_count; i++)
        {
            if (!fs->blocks[current_block].records[i].is_deleted && fs->blocks[current_block].records[i].id == id)
            {
                *block_num = current_block;
                *offset = i;
                return 0;
            }
        }

        if (meta->is_contiguous)
        {
            current_block++;
            if (current_block >= meta->first_block + meta->block_count)
                break;
        }
        else
        {
            current_block = fs->blocks[current_block].next_block;
        }
    }

    return -1; // Record not found
}

void delete_record_logical(FileSystem *fs, const char *filename, int id)
{
    int block_num, offset;
    if (search_record(fs, filename, id, &block_num, &offset) == 0)
    {
        fs->blocks[block_num].records[offset].is_deleted = true;
        printf("Record logically deleted.\n");
    }
    else
    {
        printf("Record not found.\n");
    }
}

void delete_record_physical(FileSystem *fs, const char *filename, int id)
{
    int block_num, offset;
    if (search_record(fs, filename, id, &block_num, &offset) == 0)
    {
        for (int i = offset; i < fs->blocks[block_num].record_count - 1; i++)
        {
            fs->blocks[block_num].records[i] = fs->blocks[block_num].records[i + 1];
        }
        fs->blocks[block_num].record_count--;
        printf("Record physically deleted.\n");
    }
    else
    {
        printf("Record not found.\n");
    }
}

void defragment_file(FileSystem *fs, const char *filename)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
        return;

    Metadata *meta = &fs->file_metadata[file_index];
    int current_block = meta->first_block;

    while (current_block != -1)
    {
        int write_pos = 0;
        for (int read_pos = 0; read_pos < fs->blocks[current_block].record_count; read_pos++)
        {
            if (!fs->blocks[current_block].records[read_pos].is_deleted)
            {
                fs->blocks[current_block].records[write_pos++] = fs->blocks[current_block].records[read_pos];
            }
        }
        fs->blocks[current_block].record_count = write_pos;

        if (meta->is_contiguous)
        {
            current_block++;
            if (current_block >= meta->first_block + meta->block_count)
                break;
        }
        else
        {
            current_block = fs->blocks[current_block].next_block;
        }
    }

    printf("File defragmented.\n");
}

void compact_memory(FileSystem *fs)
{
    int free_index = 1; // Start after allocation table
    for (int i = 1; i < fs->total_blocks; i++)
    {
        if (fs->allocation_table[i])
        {
            if (i != free_index)
            {
                fs->blocks[free_index] = fs->blocks[i];
                fs->allocation_table[free_index] = true;
                fs->allocation_table[i] = false;

                for (int j = 0; j < fs->file_count; j++)
                {
                    Metadata *meta = &fs->file_metadata[j];
                    if (meta->first_block == i)
                    {
                        meta->first_block = free_index;
                    }
                    else if (meta->is_contiguous && meta->first_block < i && i < meta->first_block + meta->block_count)
                    {
                        // Update the block count for contiguous files
                        meta->block_count -= (i - free_index);
                    }
                }
            }
            free_index++;
        }
    }
    printf("Memory compacted successfully.\n");
}

void display_memory_state(FileSystem *fs)
{
    for (int i = 0; i < fs->total_blocks; i++)
    {
        if (fs->allocation_table[i])
        {
            printf(RED "Block %d: Occupied by %s (%d records)\n" RESET,
                   i, fs->blocks[i].owner_file, fs->blocks[i].record_count);
        }
        else
        {
            printf(GREEN "Block %d: Free\n" RESET, i);
        }
    }
}

void display_metadata(FileSystem *fs)
{
    printf("Filename\tBlocks\tRecords\tFirst Block\tContiguous\tSorted\n");
    for (int i = 0; i < fs->file_count; i++)
    {
        Metadata *meta = &fs->file_metadata[i];
        printf("%s\t\t%d\t%d\t%d\t\t%s\t\t%s\n",
               meta->filename,
               meta->block_count,
               meta->record_count,
               meta->first_block,
               meta->is_contiguous ? "Yes" : "No",
               meta->is_sorted ? "Yes" : "No");
    }
}

void delete_file(FileSystem *fs, const char *filename)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
    {
        printf("File not found.\n");
        return;
    }

    Metadata *meta = &fs->file_metadata[file_index];
    int current_block = meta->first_block;

    while (current_block != -1)
    {
        fs->allocation_table[current_block] = false;
        fs->blocks[current_block].record_count = 0;
        strcpy(fs->blocks[current_block].owner_file, "");
        int next_block = fs->blocks[current_block].next_block;
        fs->blocks[current_block].next_block = -1;
        current_block = next_block;
    }

    for (int i = file_index; i < fs->file_count - 1; i++)
    {
        fs->file_metadata[i] = fs->file_metadata[i + 1];
    }
    fs->file_count--;
    printf("File deleted successfully.\n");
}

void rename_file(FileSystem *fs, const char *old_name, const char *new_name)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, old_name) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
    {
        printf("File not found.\n");
        return;
    }

    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, new_name) == 0)
        {
            printf("A file with the new name already exists.\n");
            return;
        }
    }

    strncpy(fs->file_metadata[file_index].filename, new_name, MAX_FILENAME - 1);
    fs->file_metadata[file_index].filename[MAX_FILENAME - 1] = '\0';
    for (int i = 0; i < fs->total_blocks; i++)
    {
        if (strcmp(fs->blocks[i].owner_file, old_name) == 0)
        {
            strncpy(fs->blocks[i].owner_file, new_name, MAX_FILENAME - 1);
            fs->blocks[i].owner_file[MAX_FILENAME - 1] = '\0';
        }
    }
    printf("File renamed successfully.\n");
}

void clear_filesystem(FileSystem *fs)
{
    for (int i = 0; i < fs->total_blocks; i++)
    {
        fs->allocation_table[i] = false;
        fs->blocks[i].record_count = 0;
        fs->blocks[i].next_block = -1;
        strcpy(fs->blocks[i].owner_file, "");
    }
    fs->file_count = 0;
    printf("Filesystem cleared.\n");
}

void generate_sample_data(FileSystem *fs, const char *filename)
{
    int file_index = -1;
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            file_index = i;
            break;
        }
    }
    if (file_index == -1)
    {
        printf("File not found.\n");
        return;
    }

    Metadata *meta = &fs->file_metadata[file_index];
    srand(time(NULL));

    for (int i = 0; i < meta->record_count; i++)
    {
        Record record;
        record.id = i + 1;
        sprintf(record.data, "Sample Data %d", i + 1);
        record.is_deleted = false;
        insert_record(fs, filename, record);
    }

    printf("Sample data generated for file %s.\n", filename);
}
