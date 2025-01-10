#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_FILENAME 50
#define MAX_FILES 100
#define MAX_RECORDS 1000

// Colors for visualization
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

typedef struct
{
    int id;
    char data[50];
    bool is_deleted;
} Record;

typedef struct
{
    char filename[MAX_FILENAME];
    int block_count;
    int record_count;
    int first_block;
    bool is_contiguous;
    bool is_sorted;
} Metadata;

typedef struct
{
    int next_block;
    Record *records;
    int record_count;
    char owner_file[MAX_FILENAME];
} Block;

typedef struct
{
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

// Initialize the file system
FileSystem *init_filesystem(int total_blocks, int block_size)
{
    FileSystem *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!fs)
        return NULL;

    fs->total_blocks = total_blocks;
    fs->block_size = block_size;
    fs->file_count = 0;

    fs->allocation_table = (bool *)calloc(total_blocks, sizeof(bool));
    fs->allocation_table[0] = true; // Reserve first block for allocation table

    fs->blocks = (Block *)malloc(total_blocks * sizeof(Block));
    for (int i = 0; i < total_blocks; i++)
    {
        fs->blocks[i].records = (Record *)malloc(block_size * sizeof(Record));
        fs->blocks[i].record_count = 0;
        fs->blocks[i].next_block = -1;
        strcpy(fs->blocks[i].owner_file, "");
    }

    fs->file_metadata = (Metadata *)malloc(MAX_FILES * sizeof(Metadata));

    return fs;
}

// Create a new file
int create_file(FileSystem *fs, const char *filename, int record_count, bool is_contiguous, bool is_sorted)
{
    if (fs->file_count >= MAX_FILES)
        return -1;

    // Calculate required blocks
    int records_per_block = fs->block_size;
    int blocks_needed = (record_count + records_per_block - 1) / records_per_block;

    // Count free blocks
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
            // Recount free blocks
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

    // Initialize metadata
    Metadata *meta = &fs->file_metadata[fs->file_count];
    strncpy(meta->filename, filename, MAX_FILENAME - 1);
    meta->block_count = blocks_needed;
    meta->record_count = record_count;
    meta->is_contiguous = is_contiguous;
    meta->is_sorted = is_sorted;

    // Allocate blocks
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
            strcpy(fs->blocks[start_block + i].owner_file, filename);
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
                strcpy(fs->blocks[i].owner_file, filename);
                prev_block = i;
                allocated++;
            }
        }
    }

    fs->file_count++;
    return 0;
}

// Insert a record into a file
int insert_record(FileSystem *fs, const char *filename, Record record)
{
    // Find file metadata
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

    // Find block to insert
    int current_block = meta->first_block;
    while (current_block != -1)
    {
        if (fs->blocks[current_block].record_count < fs->block_size)
        {
            // Found space in this block
            int insert_pos = fs->blocks[current_block].record_count;

            if (meta->is_sorted)
            {
                // Find correct position for sorted insertion
                for (insert_pos = 0; insert_pos < fs->blocks[current_block].record_count; insert_pos++)
                {
                    if (fs->blocks[current_block].records[insert_pos].id > record.id)
                    {
                        break;
                    }
                }

                // Shift records to make space
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

    return -1; // No space found
}

// Search for a record by ID
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
        if (meta->is_sorted)
        {
            // Binary search within block
            int left = 0;
            int right = fs->blocks[current_block].record_count - 1;

            while (left <= right)
            {
                int mid = (left + right) / 2;
                if (fs->blocks[current_block].records[mid].id == id &&
                    !fs->blocks[current_block].records[mid].is_deleted)
                {
                    *block_num = current_block;
                    *offset = mid;
                    return 0;
                }
                if (fs->blocks[current_block].records[mid].id < id)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid - 1;
                }
            }
        }
        else
        {
            // Linear search within block
            for (int i = 0; i < fs->blocks[current_block].record_count; i++)
            {
                if (fs->blocks[current_block].records[i].id == id &&
                    !fs->blocks[current_block].records[i].is_deleted)
                {
                    *block_num = current_block;
                    *offset = i;
                    return 0;
                }
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

// Logical deletion of a record
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

// Physical deletion of a record
void delete_record_physical(FileSystem *fs, const char *filename, int id)
{
    int block_num, offset;
    if (search_record(fs, filename, id, &block_num, &offset) == 0)
    {
        // Shift remaining records in the block
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

// Defragment a file (remove logically deleted records)
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
                if (write_pos != read_pos)
                {
                    fs->blocks[current_block].records[write_pos] =
                        fs->blocks[current_block].records[read_pos];
                }
                write_pos++;
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

// Rename a file
void rename_file(FileSystem *fs, const char *old_name, const char *new_name)
{
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, old_name) == 0)
        {
            strncpy(fs->file_metadata[i].filename, new_name, MAX_FILENAME - 1);

            // Update owner_file in blocks
            int current_block = fs->file_metadata[i].first_block;
            while (current_block != -1)
            {
                strcpy(fs->blocks[current_block].owner_file, new_name);

                if (fs->file_metadata[i].is_contiguous)
                {
                    current_block++;
                    if (current_block >= fs->file_metadata[i].first_block +
                                             fs->file_metadata[i].block_count)
                        break;
                }
                else
                {
                    current_block = fs->blocks[current_block].next_block;
                }
            }

            printf("File renamed successfully.\n");
            return;
        }
    }
    printf("File not found.\n");
}

// Delete a file
void delete_file(FileSystem *fs, const char *filename)
{
    for (int i = 0; i < fs->file_count; i++)
    {
        if (strcmp(fs->file_metadata[i].filename, filename) == 0)
        {
            // Free blocks
            int current_block = fs->file_metadata[i].first_block;
            while (current_block != -1)
            {
                fs->allocation_table[current_block] = false;
                strcpy(fs->blocks[current_block].owner_file, "");
                fs->blocks[current_block].record_count = 0;

                if (fs->file_metadata[i].is_contiguous)
                {
                    current_block++;
                    if (current_block >= fs->file_metadata[i].first_block + fs->file_metadata[i].block_count)
                        break;
                }
                else
                {
                    int next_block = fs->blocks[current_block].next_block;
                    current_block = next_block;
                }
            }

            // Remove metadata
            for (int j = i; j < fs->file_count - 1; j++)
            {
                fs->file_metadata[j] = fs->file_metadata[j + 1];
            }
            fs->file_count--;
            printf("File deleted successfully.\n");
            return;
        }
    }
    printf("File not found.\n");
}

// Compact the memory by reorganizing files
void compact_memory(FileSystem *fs)
{
    int free_index = 1; // Start after allocation table
    for (int i = 1; i < fs->total_blocks; i++)
    {
        if (fs->allocation_table[i])
        {
            if (i != free_index)
            {
                // Move block to free_index
                fs->blocks[free_index] = fs->blocks[i];
                fs->allocation_table[free_index] = true;
                fs->allocation_table[i] = false;

                // Update file metadata
                for (int j = 0; j < fs->file_count; j++)
                {
                    Metadata *meta = &fs->file_metadata[j];
                    if (meta->first_block == i)
                    {
                        meta->first_block = free_index;
                    }
                }
            }
            free_index++;
        }
    }
    printf("Memory compacted successfully.\n");
}

// Free memory used by the filesystem
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

// Display memory state (graphical representation)
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

// Display file metadata
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

// Main menu to interact with the filesystem
void menu(FileSystem *fs)
{
    int choice;
    do
    {
        printf("\n--- File System Simulator ---\n");
        printf("1. Initialize Memory\n");
        printf("2. Create File\n");
        printf("3. Display Memory State\n");
        printf("4. Display Metadata\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
        {
            int blocks, size;
            printf("Enter total blocks and block size: ");
            scanf("%d %d", &blocks, &size);
            if (fs)
                free_filesystem(fs);
            fs = init_filesystem(blocks, size);
            printf("Filesystem initialized.\n");
            break;
        }
        case 2:
        {
            char filename[MAX_FILENAME];
            int records;
            bool contiguous, sorted;
            printf("Enter filename, record count, contiguous (1/0), and sorted (1/0): ");
            scanf("%s %d %d %d", filename, &records, (int *)&contiguous, (int *)&sorted);
            if (create_file(fs, filename, records, contiguous, sorted) == 0)
            {
                printf("File created successfully.\n");
            }
            else
            {
                printf("Failed to create file.\n");
            }
            break;
        }
        case 3:
            display_memory_state(fs);
            break;
        case 4:
            display_metadata(fs);
            break;
        case 5:
            printf("Exiting program...\n");
            break;
        default:
            printf("Invalid choice.\n");
        }
    } while (choice != 5);

    free_filesystem(fs);
}

int main()
{
    FileSystem *fs = NULL;
    menu(fs);
    return 0;
}
