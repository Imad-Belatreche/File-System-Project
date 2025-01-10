#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCKS 1000
#define MAX_FILENAME_LENGTH 50

// Structure Definitions
typedef struct
{
    int blockNumber;
    int isFree;
    char fileName[MAX_FILENAME_LENGTH];
    int recordCount;
} Block;

typedef struct
{
    char name[MAX_FILENAME_LENGTH];
    int sizeInBlocks;
    int sizeInRecords;
    int firstBlock;
    char globalOrganization[20];   // e.g., "Contiguous" or "Chained"
    char internalOrganization[20]; // e.g., "Sorted" or "Unsorted"
} Metadata;

// Global Variables
Block memory[MAX_BLOCKS];
Metadata files[MAX_BLOCKS];
int totalBlocks = 0;
int blockFactor = 0;
int fileCount = 0;

// Function Prototypes
void initializeMemory();
void createFile();
void displayMemoryState();
void displayMetadata();
void insertRecord();
void searchRecord();
void deleteRecord();
void defragmentFile();
void compactMemory();
void deleteFile();
void renameFile();
void clearMemory();
void mainMenu();

// Function Implementations
void initializeMemory()
{
    printf("Enter total number of blocks: ");
    scanf("%d", &totalBlocks);
    printf("Enter block factor (records per block): ");
    scanf("%d", &blockFactor);

    for (int i = 0; i < totalBlocks; i++)
    {
        memory[i].blockNumber = i;
        memory[i].isFree = 1;
        strcpy(memory[i].fileName, "");
        memory[i].recordCount = 0;
    }

    printf("Memory initialized with %d blocks.\n", totalBlocks);
}

void createFile()
{
    if (fileCount >= MAX_BLOCKS)
    {
        printf("File limit reached. Cannot create more files.\n");
        return;
    }

    Metadata newFile;
    printf("Enter file name: ");
    scanf("%s", newFile.name);
    printf("Enter number of records: ");
    scanf("%d", &newFile.sizeInRecords);
    printf("Enter global organization (Contiguous/Chained): ");
    scanf("%s", newFile.globalOrganization);
    printf("Enter internal organization (Sorted/Unsorted): ");
    scanf("%s", newFile.internalOrganization);

    int requiredBlocks = (newFile.sizeInRecords + blockFactor - 1) / blockFactor;
    newFile.sizeInBlocks = requiredBlocks;

    int startBlock = -1, consecutiveBlocks = 0;
    for (int i = 0; i < totalBlocks; i++)
    {
        if (memory[i].isFree)
        {
            if (startBlock == -1)
                startBlock = i;
            consecutiveBlocks++;
            if (consecutiveBlocks == requiredBlocks)
                break;
        }
        else
        {
            startBlock = -1;
            consecutiveBlocks = 0;
        }
    }

    if (consecutiveBlocks < requiredBlocks)
    {
        printf("Not enough contiguous blocks available. Consider compacting the memory.\n");
        return;
    }

    newFile.firstBlock = startBlock;
    for (int i = startBlock; i < startBlock + requiredBlocks; i++)
    {
        memory[i].isFree = 0;
        strcpy(memory[i].fileName, newFile.name);
        memory[i].recordCount = blockFactor;
    }

    files[fileCount++] = newFile;
    printf("File '%s' created successfully.\n", newFile.name);
}

void displayMemoryState()
{
    printf("\nMemory State:\n");
    for (int i = 0; i < totalBlocks; i++)
    {
        if (memory[i].isFree)
        {
            printf("Block %d: Free\n", i);
        }
        else
        {
            printf("Block %d: File '%s', Records %d\n", i, memory[i].fileName, memory[i].recordCount);
        }
    }
}

void displayMetadata()
{
    printf("\nMetadata of Files:\n");
    printf("%-15s %-10s %-10s %-10s %-20s %-20s\n", "File Name", "Blocks", "Records", "First Block", "Global Org.", "Internal Org.");
    for (int i = 0; i < fileCount; i++)
    {
        printf("%-15s %-10d %-10d %-10d %-20s %-20s\n", files[i].name, files[i].sizeInBlocks, files[i].sizeInRecords, files[i].firstBlock, files[i].globalOrganization, files[i].internalOrganization);
    }
}

void insertRecord()
{
    char fileName[MAX_FILENAME_LENGTH];
    printf("Enter the file name to insert a record into: ");
    scanf("%s", fileName);

    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(files[i].name, fileName) == 0)
        {
            if (files[i].sizeInRecords < files[i].sizeInBlocks * blockFactor)
            {
                for (int j = files[i].firstBlock; j < files[i].firstBlock + files[i].sizeInBlocks; j++)
                {
                    if (memory[j].recordCount < blockFactor)
                    {
                        memory[j].recordCount++;
                        files[i].sizeInRecords++;
                        printf("Record inserted into block %d.\n", j);
                        return;
                    }
                }
            }
            else
            {
                printf("No space available in the file for a new record.\n");
            }
            return;
        }
    }
    printf("File not found.\n");
}

void searchRecord()
{
    char fileName[MAX_FILENAME_LENGTH];
    int recordID;
    printf("Enter the file name to search in: ");
    scanf("%s", fileName);
    printf("Enter the record ID to search for: ");
    scanf("%d", &recordID);

    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(files[i].name, fileName) == 0)
        {
            int recordIndex = 0;
            for (int j = files[i].firstBlock; j < files[i].firstBlock + files[i].sizeInBlocks; j++)
            {
                if (recordID <= recordIndex + memory[j].recordCount)
                {
                    printf("Record found in block %d at position %d.\n", j, recordID - recordIndex);
                    return;
                }
                recordIndex += memory[j].recordCount;
            }
            printf("Record not found in the file.\n");
            return;
        }
    }
    printf("File not found.\n");
}

void deleteRecord()
{
    char fileName[MAX_FILENAME_LENGTH];
    int recordID;
    printf("Enter the file name to delete a record from: ");
    scanf("%s", fileName);
    printf("Enter the record ID to delete: ");
    scanf("%d", &recordID);

    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(files[i].name, fileName) == 0)
        {
            int recordIndex = 0;
            for (int j = files[i].firstBlock; j < files[i].firstBlock + files[i].sizeInBlocks; j++)
            {
                if (recordID <= recordIndex + memory[j].recordCount)
                {
                    memory[j].recordCount--;
                    files[i].sizeInRecords--;
                    printf("Record deleted from block %d.\n", j);
                    return;
                }
                recordIndex += memory[j].recordCount;
            }
            printf("Record not found in the file.\n");
            return;
        }
    }
    printf("File not found.\n");
}

void defragmentFile()
{
    printf("Defragmentation functionality to be implemented.\n");
}

void compactMemory()
{
    printf("Memory compaction functionality to be implemented.\n");
}

void deleteFile()
{
    char fileName[MAX_FILENAME_LENGTH];
    printf("Enter the file name to delete: ");
    scanf("%s", fileName);

    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(files[i].name, fileName) == 0)
        {
            for (int j = files[i].firstBlock; j < files[i].firstBlock + files[i].sizeInBlocks; j++)
            {
                memory[j].isFree = 1;
                strcpy(memory[j].fileName, "");
                memory[j].recordCount = 0;
            }
            files[i] = files[fileCount - 1];
            fileCount--;
            printf("File '%s' deleted successfully.\n", fileName);
            return;
        }
    }
    printf("File not found.\n");
}

void renameFile()
{
    char oldName[MAX_FILENAME_LENGTH], newName[MAX_FILENAME_LENGTH];
    printf("Enter the current file name: ");
    scanf("%s", oldName);
    printf("Enter the new file name: ");
    scanf("%s", newName);

    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(files[i].name, oldName) == 0)
        {
            strcpy(files[i].name, newName);
            for (int j = files[i].firstBlock; j < files[i].firstBlock + files[i].sizeInBlocks; j++)
            {
                strcpy(memory[j].fileName, newName);
            }
            printf("File renamed to '%s'.\n", newName);
            return;
        }
    }
    printf("File not found.\n");
}

void clearMemory()
{
    for (int i = 0; i < totalBlocks; i++)
    {
        memory[i].isFree = 1;
        strcpy(memory[i].fileName, "");
        memory[i].recordCount = 0;
    }
    fileCount = 0;
    printf("Memory cleared successfully.\n");
}

void mainMenu()
{
    int choice;
    do
    {
        printf("\nMain Menu:\n");
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
        printf("12. Clear Memory\n");
        printf("13. Quit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
            initializeMemory();
            break;
        case 2:
            createFile();
            break;
        case 3:
            displayMemoryState();
            break;
        case 4:
            displayMetadata();
            break;
        case 5:
            insertRecord();
            break;
        case 6:
            searchRecord();
            break;
        case 7:
            deleteRecord();
            break;
        case 8:
            defragmentFile();
            break;
        case 9:
            compactMemory();
            break;
        case 10:
            deleteFile();
            break;
        case 11:
            renameFile();
            break;
        case 12:
            clearMemory();
            break;
        case 13:
            printf("Exiting program.\n");
            break;
        default:
            printf("Invalid choice. Try again.\n");
        }
    } while (choice != 13);
}

int main()
{
    mainMenu();
    return 0;
}
