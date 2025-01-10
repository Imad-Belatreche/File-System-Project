# File Management System

This is a simple File Management System written in C language. It simulates a file system with basic functionalities such as creating files, inserting records, searching records, deleting records, defragmenting files, compacting memory, and more.

## Features

- Initialize memory for the file system
- Create files with specified record count, contiguity, and sorting options
- Insert records into files
- Search for records by ID
- Logically and physically delete records
- Defragment files to remove logically deleted records
- Compact memory to optimize space usage
- Display the current state of memory and file metadata
- Delete files and rename files
- Generate sample data for testing

## File Structure

- `file_system.c`: Contains the implementation of the file system functions.
- `file_system.h`: Contains the declarations of the file system functions and data structures.
- `main.c`: Contains the main function and the menu for interacting with the file system.
- `README.md`: This file.

## How to Use

1. Compile the project using a C compiler. For example:
    ```sh
    gcc main.c file_system.c -o file_system
    ```

2. Run the compiled executable:
    ```sh
    ./file_system
    ```

3. Follow the menu options to interact with the file system.

## Menu Options

1. **Initialize Memory**: Initialize the file system with a specified number of blocks and block size.
2. **Create File**: Create a new file with a specified name, record count, contiguity, and sorting options.
3. **Display Memory State**: Display the current state of memory blocks.
4. **Display Metadata**: Display metadata of all files in the file system.
5. **Insert Record**: Insert a new record into a specified file.
6. **Search Record**: Search for a record by ID in a specified file.
7. **Delete Record**: Delete a record by ID in a specified file (logical or physical deletion).
8. **Defragment File**: Defragment a specified file to remove logically deleted records.
9. **Compact Memory**: Compact memory to optimize space usage.
10. **Delete File**: Delete a specified file from the file system.
11. **Rename File**: Rename a specified file.
12. **Clear Filesystem**: Clear all files and reset the file system.
13. **Generate Sample Data**: Generate sample data for a specified file.
14. **Quit**: Exit the file system simulator.

## Data Structures

- `Record`: Represents a record in a file.
- `Metadata`: Represents metadata of a file.
- `Block`: Represents a block in the file system.
- `FileSystem`: Represents the file system.