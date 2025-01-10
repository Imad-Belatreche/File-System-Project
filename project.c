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
void menu(FileSystem *fs);

// Input utility functions
int get_int_input(const char *prompt, int min, int max);
void get_string_input(const char *prompt, char *buffer, size_t size);

// Implementations of utility functions
int get_int_input(const char *prompt, int min, int max)
{
    int value;
    while (true)
    {
        printf("%s", prompt);
        if (scanf("%d", &value) == 1 && value >= min && value <= max)
        {
            while (getchar() != '\n')
                ; // Clear input buffer
            return value;
        }
        printf(RED "Invalid input. Please enter a number between %d and %d." RESET "\n", min, max);
        while (getchar() != '\n')
            ; // Clear invalid input
    }
}

void get_string_input(const char *prompt, char *buffer, size_t size)
{
    printf("%s", prompt);
    if (fgets(buffer, size, stdin))
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';
    }
    else
    {
        printf(RED "Error reading input." RESET "\n");
    }
}

// Initialize the file system
FileSystem *init_filesystem(int total_blocks, int block_size)
{
    if (total_blocks <= 0 || block_size <= 0)
    {
        printf("Invalid block count or size\n");
        return NULL;
    }

    FileSystem *fs = (FileSystem *)malloc(sizeof(FileSystem));
    if (!fs)
    {
        printf("Failed to allocate filesystem\n");
        return NULL;
    }

    fs->total_blocks = total_blocks;
    fs->block_size = block_size;
    fs->file_count = 0;

    // Allocate and initialize allocation table
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

    // Initialize each block
    for (int i = 0; i < total_blocks; i++)
    {
        fs->blocks[i].records = (Record *)malloc(block_size * sizeof(Record));
        if (!fs->blocks[i].records)
        {
            // Clean up previously allocated blocks
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
        fs->blocks[i].owner_file[0] = '\0'; // Initialize empty string safely
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

// Free memory used by the filesystem
void free_filesystem(FileSystem *fs)
{
    if (!fs)
        return;
    if (fs->blocks)
    {
        for (int i = 0; i < fs->total_blocks; i++)
        {
            if (fs->blocks[i].records)
            {
                free(fs->blocks[i].records);
            }
        }
        free(fs->blocks);
    }

    if (fs->allocation_table)
        free(fs->allocation_table);
    if (fs->file_metadata)
        free(fs->file_metadata);
    free(fs);
    printf("Filesystem resources freed.\n");
}

// Create a new file
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
    meta->filename[MAX_FILENAME - 1] = '\0'; // Ensure null-termination
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
            strcpy(fs->blocks[start_block + i].owner_file, filename);
        }

        for (int i = 0; i < blocks_needed; i++)
        {
            fs->allocation_table[start_block + i] = true;
            strcpy(fs->blocks[start_block + i].owner_file, filename);
            if (i < blocks_needed - 1)
            {
                fs->blocks[start_block + i].next_block = start_block + i + 1;
            }
            else
            {
                fs->blocks[start_block + i].next_block = -1;
            }
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

// Search for a record
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

// Defragment a file
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

// Compact the memory
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
                }
            }
            free_index++;
        }
    }
    printf("Memory compacted successfully.\n");
}

// Display memory state
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

// Display metadata
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

void generate_sample_data(FileSystem *fs, const char *filename)
{
    srand(time(NULL));
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
    for (int i = 0; i < meta->record_count; i++)
    {
        Record record;
        record.id = i + 1;
        sprintf(record.data, "Data_%d", i + 1);
        record.is_deleted = false;
        insert_record(fs, filename, record);
    }
}

// Delete a file
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

// Rename a file
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
        if (strlen(new_name) >= MAX_FILENAME)
        {
            printf("New filename is too long\n");
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

// Clear the filesystem
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

// Menu to interact with the filesystem
void menu(FileSystem *fs)
{
    if (!fs)
    {
        printf("Error: Filesystem not initialized\n");
        return;
    }

    int choice;
    char filename[MAX_FILENAME];

    do
    {
        printf("\n--- File System Simulator ---\n");
        printf("1. Initialize Memory\n");
        printf("2. Create File\n");
        printf("3. Display Memory State\n");
        printf("4. Display Metadata\n");
        printf("5. Insert Record\n");
        printf("6. Search Record\n");
        printf("7. Delete Record\n");
        printf("8. Defragment File\n");
        printf("9. Compact Memory\n");
        printf("10. Delete File\n");
        printf("11. Rename File\n");
        printf("12. Clear Filesystem\n");
        printf("13. Generate Sample Data\n");
        printf("14. Quit\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1)
        {
            // Clear input buffer on invalid input
            while (getchar() != '\n')
                ;
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            int blocks, size;
            printf("Enter total blocks and block size: ");
            if (scanf("%d %d", &blocks, &size) != 2 || blocks <= 0 || size <= 0)
            {
                printf("Invalid input. Both values must be positive.\n");
                while (getchar() != '\n')
                    ; // Clear input buffer
                break;
            }

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
        {
            char filename[MAX_FILENAME];
            Record record;
            printf("Enter filename and record ID: ");
            scanf("%s %d", filename, &record.id);
            printf("Enter record data: ");
            scanf("%s", record.data);
            record.is_deleted = false;
            if (insert_record(fs, filename, record) == 0)
            {
                printf("Record inserted successfully.\n");
            }
            else
            {
                printf("Failed to insert record.\n");
            }
            break;
        }
        case 6:
        {
            char filename[MAX_FILENAME];
            int id, block_num, offset;
            printf("Enter filename and record ID: ");
            scanf("%s %d", filename, &id);
            if (search_record(fs, filename, id, &block_num, &offset) == 0)
            {
                printf("Record found in block %d at offset %d.\n", block_num, offset);
            }
            else
            {
                printf("Record not found.\n");
            }
            break;
        }
        case 7:
        {
            char filename[MAX_FILENAME];
            int id, deletion_type;
            printf("Enter filename and record ID: ");
            scanf("%s %d", filename, &id);
            printf("Enter deletion type (1 for logical, 2 for physical): ");
            scanf("%d", &deletion_type);
            if (deletion_type == 1)
            {
                delete_record_logical(fs, filename, id);
            }
            else if (deletion_type == 2)
            {
                delete_record_physical(fs, filename, id);
            }
            else
            {
                printf("Invalid deletion type.\n");
            }
            break;
        }
        case 8:
        {
            char filename[MAX_FILENAME];
            printf("Enter filename to defragment: ");
            scanf("%s", filename);
            defragment_file(fs, filename);
            break;
        }
        case 9:
            compact_memory(fs);
            break;
        case 10:
        {
            char filename[MAX_FILENAME];
            printf("Enter filename to delete: ");
            scanf("%s", filename);
            delete_file(fs, filename);
            break;
        }
        case 11:
        {
            char old_name[MAX_FILENAME], new_name[MAX_FILENAME];
            printf("Enter old filename and new filename: ");
            scanf("%s %s", old_name, new_name);
            rename_file(fs, old_name, new_name);
            break;
        }
        case 12:
            clear_filesystem(fs);
            printf("Filesystem cleared.\n");
            break;
        case 13:
        {
            char filename[MAX_FILENAME];
            printf("Enter filename for sample data: ");
            scanf("%s", filename);
            generate_sample_data(fs, filename);
            break;
        }
        case 14:
            printf("Exiting simulator...\n");
            break;
        default:
            printf("Invalid choice. Try again.\n");
        }
    } while (choice != 14);
}

int main()
{
    // Initialize with default size, user can reinitialize later
    FileSystem *fs = init_filesystem(100, 10);
    if (!fs)
    {
        printf("Failed to initialize filesystem\n");
        return 1;
    }
    menu(fs);
    free_filesystem(fs);
    return 0;
}