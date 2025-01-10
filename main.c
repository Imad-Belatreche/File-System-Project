
#include "file_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int get_integer_input()
{
    int value;
    char input[100];

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        char *endptr;
        value = strtol(input, &endptr, 10);
        if (*endptr != '\n' && *endptr != '\0')
        {
            return -1; // Invalid input
        }
        return value;
    }
    return -1; // Error reading input
}

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

        choice = get_integer_input();

        if (choice == -1)
        {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            int blocks, size;
            printf("Enter total blocks: ");
            blocks = get_integer_input();
            if (blocks <= 0)
            {
                printf("Invalid input. Please enter a positive integer for total blocks.\n");
                break;
            }
            printf("Enter block size: ");
            size = get_integer_input();
            if (size <= 0)
            {
                printf("Invalid input. Please enter a positive integer for block size.\n");
                break;
            }

            if (fs)
                free_filesystem(fs);
            fs = init_filesystem(blocks, size);
            if (fs)
            {
                printf("Filesystem initialized.\n");
            }
            else
            {
                printf("Failed to initialize filesystem.\n");
            }
            break;
        }
        case 2:
        {
            char filename[MAX_FILENAME];
            int records;
            int contiguous, sorted;

            printf("Enter filename: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present

            printf("Enter record count: ");
            records = get_integer_input();
            if (records <= 0)
            {
                printf("Invalid record count. Please enter a positive integer.\n");
                break;
            }

            printf("Is the file contiguous? (1 for yes, 0 for no): ");
            contiguous = get_integer_input();
            if (contiguous != 0 && contiguous != 1)
            {
                printf("Invalid input. Please enter 0 or 1.\n");
                break;
            }

            printf("Is the file sorted? (1 for yes, 0 for no): ");
            sorted = get_integer_input();
            if (sorted != 0 && sorted != 1)
            {
                printf("Invalid input. Please enter 0 or 1.\n");
                break;
            }

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
            printf("Enter filename: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present

            printf("Enter record ID: ");
            record.id = get_integer_input();
            if (record.id <= 0)
            {
                printf("Invalid record ID. Please enter a positive integer.\n");
                break;
            }

            printf("Enter record data: ");
            if (fgets(record.data, sizeof(record.data), stdin) == NULL)
            {
                printf("Error reading record data.\n");
                break;
            }
            record.data[strcspn(record.data, "\n")] = 0; // Remove newline if present

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
            printf("Enter filename: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present

            printf("Enter record ID: ");
            id = get_integer_input();
            if (id <= 0)
            {
                printf("Invalid record ID. Please enter a positive integer.\n");
                break;
            }

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
            printf("Enter filename: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present

            printf("Enter record ID: ");
            id = get_integer_input();
            if (id <= 0)
            {
                printf("Invalid record ID. Please enter a positive integer.\n");
                break;
            }

            printf("Enter deletion type (1 for logical, 2 for physical): ");
            deletion_type = get_integer_input();
            if (deletion_type != 1 && deletion_type != 2)
            {
                printf("Invalid deletion type. Please enter 1 or 2.\n");
                break;
            }

            if (deletion_type == 1)
            {
                delete_record_logical(fs, filename, id);
            }
            else
            {
                delete_record_physical(fs, filename, id);
            }
            break;
        }
        case 8:
        {
            char filename[MAX_FILENAME];
            printf("Enter filename to defragment: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present
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
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present
            delete_file(fs, filename);
            break;
        }
        case 11:
        {
            char old_name[MAX_FILENAME], new_name[MAX_FILENAME];
            printf("Enter old filename: ");
            if (fgets(old_name, sizeof(old_name), stdin) == NULL)
            {
                printf("Error reading old filename.\n");
                break;
            }
            old_name[strcspn(old_name, "\n")] = 0; // Remove newline if present

            printf("Enter new filename: ");
            if (fgets(new_name, sizeof(new_name), stdin) == NULL)
            {
                printf("Error reading new filename.\n");
                break;
            }
            new_name[strcspn(new_name, "\n")] = 0; // Remove newline if present

            rename_file(fs, old_name, new_name);
            break;
        }
        case 12:
            clear_filesystem(fs);
            break;
        case 13:
        {
            char filename[MAX_FILENAME];
            printf("Enter filename to generate sample data: ");
            if (fgets(filename, sizeof(filename), stdin) == NULL)
            {
                printf("Error reading filename.\n");
                break;
            }
            filename[strcspn(filename, "\n")] = 0; // Remove newline if present
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
