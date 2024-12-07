#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "header.h"

#define OUTPUT_LENGTH 41
#define MAX_LINE_LENGTH 256
#define MAX_LINE 1024
#define OBJECTS_PATH ".trackit/objects/"
#define HEAD_FILE_PATH ".trackit/HEAD"
#define INDEX_FILE_PATH ".trackit/INDEX"

int init()
{
    const char *main_dir = ".trackit";
    const char *obj_dir = OBJECTS_PATH;
    const char *HEAD = HEAD_FILE_PATH;
    const char *INDEX = INDEX_FILE_PATH;

    if (access(main_dir, F_OK) == 0)
    {
        printf("Trackit is already initialised in this folder\n");
        return 0;
    }

    if (mkdir(main_dir) == -1)
    {
        printf("Error creating directory");
        return 0;
    }
    else
    {
        printf("Directory '%s' created successfully.\n", main_dir);
        if (mkdir(obj_dir) == -1)
        {
            printf("Error creating directory");
            return 0;
        }
        else
        {
            printf("Directory '%s' created successfully.\n", obj_dir);
        }
    }

    FILE *file = fopen(HEAD, "w");
    if (file == NULL)
    {
        printf("Error creating file");
        return 0;
    }
    else
    {
        printf("File '%s' created successfully.\n", HEAD);
        fclose(file);
    }

    file = fopen(INDEX, "w");
    if (file == NULL)
    {
        printf("Error creating file");
        return 0;
    }
    else
    {
        printf("File '%s' created successfully.\n", INDEX);
        fclose(file);
    }

    return 1;
}

char *prependDotSlash(const char *filename)
{
    size_t length = strlen(filename);
    char *newFilename = (char *)malloc(length + 3);

    if (newFilename == NULL)
    {
        return NULL;
    }

    // Copy "./" to the new string
    strcpy(newFilename, "./");

    // Concatenate the original filename
    strcat(newFilename, filename);

    return newFilename;
}

char *generateHash(const char *filePath)
{
    FILE *file = fopen(filePath, "rb");
    if (file == NULL)
    {
        return "Error: Could not open file.";
    }

    // Initialize hash value with a constant prime number
    unsigned long long hash = 5381;
    int c;

    // Process file path as part of the hash
    for (const char *ptr = filePath; *ptr != '\0'; ptr++)
    {
        hash = ((hash << 5) + hash) + *ptr; // hash * 33 + character
    }

    // Process file content as part of the hash
    while ((c = fgetc(file)) != EOF)
    {
        hash = ((hash << 5) + hash) + c;
    }

    fclose(file);

    // Allocate memory for the output string
    char *output = (char *)malloc((OUTPUT_LENGTH + 1) * sizeof(char));
    if (output == NULL)
    {
        return "Error: Memory allocation failed.";
    }

    // Convert the hash value to a fixed-length hexadecimal string
    snprintf(output, OUTPUT_LENGTH + 1, "%0*llX", OUTPUT_LENGTH, hash); // Fixed-width hex format

    return output;
}

int add(int fileCount, char *filePath[])
{
    char hashedFilePath[256];
    char buffer[1024];
    int bytesRead;
    char *newFilepath;
    char *hash;
    char originalFilePath[256];

    for (int i = 0; i < fileCount; i++)
    {
        struct stat statBuffer;
        if (!stat(filePath[i], &statBuffer) == 0)
        {
            printf("File %s does not exists.\n", filePath[i]);
            continue;
        }

        newFilepath = prependDotSlash(filePath[i]);
        hash = generateHash(newFilepath);

        // Create the path for the hashed file
        snprintf(hashedFilePath, sizeof(hashedFilePath), "%s%s", OBJECTS_PATH, hash);

        // Check if the file already exists
        FILE *existingFile = fopen(hashedFilePath, "rb");
        if (existingFile != NULL)
        {
            // File exists, close the file and skip this iteration
            fclose(existingFile);
            printf("File with hash %s already exists in the staging area. Skipping %s.\n", hash, filePath[i]);
            free(newFilepath); // Free dynamically allocated memory
            free(hash);        // Free dynamically allocated memory
            continue;          // Skip to the next file
        }

        sprintf(originalFilePath, "./%s", filePath[i]);
        FILE *file = fopen(originalFilePath, "rb");
        FILE *file2 = fopen(hashedFilePath, "wb");
        if (file == NULL || file2 == NULL)
        {
            printf("Error creating file");
            return 1;
        }
        else
        {
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
            {
                if (fwrite(buffer, 1, bytesRead, file2) != bytesRead)
                {
                    printf("Error writing to target file");
                    fclose(file);
                    fclose(file2);
                    return 1;
                }
            }
            fclose(file);
            fclose(file2);
            printf("Added %s to staging area\n", filePath[i]);
        }

        updateStagingArea(newFilepath, hash);

        // Free dynamically allocated memory
        free(newFilepath);
        free(hash);
    }

    return 0;
}

index *getStoredIndex()
{
    FILE *file = fopen(INDEX_FILE_PATH, "r");
    char line[512]; // Buffer for each line in the file

    index *newindex = (index *)malloc(sizeof(index));
    newindex->head = newindex->tail = NULL;

    if (file == NULL)
    {
        printf("Error opening file %s\n", INDEX_FILE_PATH);
        return newindex;
    }

    while (fgets(line, sizeof(line), file))
    {
        char filepath[256];
        char hash[100];

        // Use sscanf to parse the line in the format: "<filepath>" <hash>
        if (sscanf(line, "\"%[^\"]\" %s", filepath, hash) == 2)
        {
            node *current_node = (node *)malloc(sizeof(node));
            strcpy(current_node->filepath, filepath);
            strcpy(current_node->hash, hash);
            current_node->next = NULL;

            // Add the node to the linked list
            if (newindex->head == NULL)
            {
                newindex->head = newindex->tail = current_node;
            }
            else
            {
                newindex->tail->next = current_node;
                newindex->tail = current_node;
            }
        }
    }

    fclose(file);
    return newindex;
}

void storeToIndex(index *stagging)
{
    FILE *file = fopen(INDEX_FILE_PATH, "w");
    if (file == NULL)
    {
        printf("Error opening file %s\n", INDEX_FILE_PATH);
        return;
    }

    node *current = stagging->head;
    while (current != NULL)
    {
        // Write each entry as "<filepath>" <hash>
        fprintf(file, "\"%s\" %s\n", current->filepath, current->hash);
        current = current->next;
    }

    fclose(file);
}

void updateStagingArea(char *filename, char *hash)
{
    index *stagging = getStoredIndex();

    // Check if the file already exists in the index with the same hash
    node *temp = stagging->head;
    while (temp != NULL)
    {
        if (strcmp(temp->filepath, filename) == 0 && strcmp(temp->hash, hash) == 0)
        {
            printf("File '%s' with hash '%s' is already in the index. Skipping...\n", filename, hash);
            // Free the linked list and exit if the entry is found
            node *to_free;
            while (stagging->head != NULL)
            {
                to_free = stagging->head;
                stagging->head = stagging->head->next;
                free(to_free);
            }
            free(stagging);
            return;
        }
        temp = temp->next;
    }

    // If the entry doesn't exist, create a new node for the file
    node *newNode = (node *)malloc(sizeof(node));
    strcpy(newNode->filepath, filename);
    strcpy(newNode->hash, hash);
    newNode->next = NULL;

    // Add new node to the end of the linked list
    if (stagging->tail == NULL)
    {
        stagging->head = stagging->tail = newNode;
    }
    else
    {
        stagging->tail->next = newNode;
        stagging->tail = newNode;
    }

    // Display the updated staging area
    node *tp = stagging->head;
    printf("Updated Staging area:\n");
    while (tp != NULL)
    {
        printf("Filepath: %s, Hash: %s\n", tp->filepath, tp->hash);
        tp = tp->next;
    }

    // Store the updated staging area in the index file
    storeToIndex(stagging);

    // Free the allocated memory for the index
    node *to_free;
    while (stagging->head != NULL)
    {
        to_free = stagging->head;
        stagging->head = stagging->head->next;
        free(to_free);
    }
    free(stagging);
}

char *getCurrentTimestamp()
{
    // Allocate memory for the timestamp string
    char *timestamp = malloc(20 * sizeof(char));
    if (timestamp == NULL)
    {
        return NULL; // Return NULL if memory allocation fails
    }

    // Get the current time
    time_t now = time(NULL);
    struct tm *localTime = localtime(&now);

    // Format the timestamp into the allocated string
    strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", localTime);

    return timestamp;
}

char *getParentHash(const char *fileName)
{
    // Open the file in read mode
    FILE *file = fopen(fileName, "r");
    if (file == NULL)
    {
        return NULL; // Return NULL if the file can't be opened
    }

    // Move to the end of the file to get the file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file); // Go back to the beginning of the file

    // If the file is empty, return NULL
    if (fileSize == 0)
    {
        fclose(file);
        return NULL;
    }

    // Allocate memory for the content
    char *content = malloc((fileSize + 1) * sizeof(char));
    if (content == NULL)
    {
        fclose(file);
        return NULL; // Return NULL if memory allocation fails
    }

    // Read the file content into the allocated memory
    fread(content, sizeof(char), fileSize, file);
    content[fileSize] = '\0';

    fclose(file);

    return content;
}

// store the index file (will return 0 if the file with hash already exits)
int storeIndexFile(char *hash)
{
    char hashedFilePath[256];
    char buffer[1024];
    int bytesRead;
    snprintf(hashedFilePath, sizeof(hashedFilePath), ".trackit/objects/%s", hash);

    // Check if the file already exists
    FILE *existingFile = fopen(hashedFilePath, "rb");
    if (existingFile != NULL)
    {
        printf("File already exists");
        fclose(existingFile);
        // empty the file
        FILE *file0 = fopen(INDEX_FILE_PATH, "w");
        fclose(file0);
        return 0;
    }
    else
    {
        FILE *file = fopen(INDEX_FILE_PATH, "rb");
        FILE *file2 = fopen(hashedFilePath, "wb");
        if (file == NULL || file2 == NULL)
        {
            printf("Error creating file");
            return 0;
        }
        else
        {
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
            {
                if (fwrite(buffer, 1, bytesRead, file2) != bytesRead)
                {
                    printf("Error writing to target file");
                    fclose(file);
                    fclose(file2);
                    return 1;
                }
            }
            fclose(file);
            fclose(file2);
        }

        // empty the file
        FILE *file3 = fopen(INDEX_FILE_PATH, "w");
        fclose(file3);
    }

    return 1;
}

void commitFiles(char *message)
{
    FILE *file = fopen(INDEX_FILE_PATH, "r");
    if (file == NULL)
    {
        printf("Commit Failed");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    if (fileSize == 0)
    {
        fclose(file);
        printf("No files in stagging area");
        return;
    }

    char *timeStamp = getCurrentTimestamp();

    char *indexHash = generateHash(INDEX_FILE_PATH);
    storeIndexFile(indexHash);

    char *parentHash = getParentHash(HEAD_FILE_PATH);

    FILE *indexFile = fopen(INDEX_FILE_PATH, "w");
    fprintf(indexFile, "%s\n%s\n%s\n%s\n", timeStamp, message, indexHash, parentHash);
    fclose(indexFile);

    char *commitHash = generateHash(INDEX_FILE_PATH);

    FILE *commitFile = fopen(HEAD_FILE_PATH, "w");
    fprintf(commitFile, "%s", commitHash);
    fclose(commitFile);

    if (!storeIndexFile(commitHash))
    {
        printf("Commit Already exists");
        return;
    }
    else
    {
        printf("Commit Successful");
    }
}

commit *loadCommit(const char *commitHash)
{
    char commitFilePath[MAX_LINE_LENGTH];
    snprintf(commitFilePath, sizeof(commitFilePath), "%s%s", OBJECTS_PATH, commitHash);

    FILE *file = fopen(commitFilePath, "r");
    if (file == NULL)
    {
        printf("Error: Could not open commit file %s\n", commitFilePath);
        return NULL;
    }

    commit *newCommit = (commit *)malloc(sizeof(commit));
    if (!newCommit)
    {
        printf("Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Reading commit fields line by line
    char line[MAX_LINE_LENGTH];

    // Read and allocate each field
    if (fgets(line, sizeof(line), file))
    {
        newCommit->timestamp = strdup(line);
        strtok(newCommit->timestamp, "\n"); // Remove newline
    }
    if (fgets(line, sizeof(line), file))
    {
        newCommit->message = strdup(line);
        strtok(newCommit->message, "\n");
    }
    if (fgets(line, sizeof(line), file))
    {
        newCommit->stagingFiles = strdup(line);
        strtok(newCommit->stagingFiles, "\n");
    }
    if (fgets(line, sizeof(line), file))
    {
        newCommit->parentCommit = strdup(line);
        strtok(newCommit->parentCommit, "\n");
    }
    else
    {
        newCommit->parentCommit = NULL;
    }

    fclose(file);
    newCommit->next = NULL;
    return newCommit;
}

// Function to get the latest commit hash from the HEAD file
char *getHeadCommitHash()
{
    FILE *file = fopen(HEAD_FILE_PATH, "r");
    if (file == NULL)
    {
        printf("Error: Could not open HEAD file\n");
        return NULL;
    }

    char *commitHash = (char *)malloc(OUTPUT_LENGTH + 1);
    if (fgets(commitHash, OUTPUT_LENGTH + 1, file) == NULL)
    {
        free(commitHash);
        commitHash = NULL;
    }
    else
    {
        strtok(commitHash, "\n"); // Remove newline
    }
    fclose(file);
    return commitHash;
}

// Function to print the commit history in reverse order
void printCommitHistory(commit *head)
{
    commit *current = head;
    int i = 1;
    while (current != NULL)
    {
        printf("Commit %d:\n", i++);
        printf("Timestamp: %s\n", current->timestamp);
        printf("Message: %s\n", current->message);
        printf("Staging Files Hash: %s\n", current->stagingFiles);
        printf("Parent Commit: %s\n", current->parentCommit ? current->parentCommit : "None");
        printf("--------------------------------------\n");
        current = current->next;
    }
}

// Function to free allocated memory
void freeCommitHistory(commit *head)
{
    while (head != NULL)
    {
        commit *temp = head;
        head = head->next;
        free(temp->timestamp);
        free(temp->message);
        free(temp->stagingFiles);
        free(temp->parentCommit);
        free(temp);
    }
}

// Main log function
void logHistory()
{
    char *commitHash = getHeadCommitHash();
    if (commitHash == NULL)
    {
        printf("No commits found.\n");
        return;
    }

    commit *head = NULL;

    // Load each commit and add it to the list
    while (commitHash != NULL)
    {
        commit *newCommit = loadCommit(commitHash);
        if (newCommit == NULL)
        {
            free(commitHash);
            break;
        }

        // Insert at the beginning of the linked list
        newCommit->next = head;
        head = newCommit;

        // Check if this commit has a parent
        if (newCommit->parentCommit && strcmp(newCommit->parentCommit, "(null)") != 0)
        {
            commitHash = strdup(newCommit->parentCommit);
        }
        else
        {
            free(commitHash);
            commitHash = NULL;
        }
    }

    // Print commit history
    printCommitHistory(head);

    // Free memory
    freeCommitHistory(head);
}

// Function to delete a directory and its contents
void delete_directory(const char *path) {
    if (!path) {
        fprintf(stderr, "Error: NULL path provided to delete_directory\n");
        return;
    }

    struct dirent *entry;
    DIR *dir = opendir(path);
    char full_path[1024];

    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }        

        // Construct full path
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat path_stat;
        if (stat(full_path, &path_stat) == -1) {
            perror("Failed to get file status");
            continue;
        }

        if (S_ISDIR(path_stat.st_mode)) {
            // Recursively delete subdirectory
            delete_directory(full_path);
        } else {
            // Delete file
            if (remove(full_path) != 0) {
                perror("Failed to delete file");
            } else {
                printf("Deleted file: %s\n", full_path);
            }
        }
    }

    closedir(dir);

    // Remove the now-empty directory
    if (rmdir(path) != 0) {
        perror("Failed to delete directory");
    } else {
        printf("Deleted directory: %s\n", path);
    }
}


void revert() {
    // Load current commit hash from head file
    FILE *headFile = fopen(HEAD_FILE_PATH, "r");
    if (!headFile) {
        printf("Error: Head file not found.\n");
        return;
    }

    char commitHash[MAX_LINE_LENGTH];
    if (!fgets(commitHash, MAX_LINE_LENGTH, headFile)) {
        printf("Error: Head is empty.\n");
        fclose(headFile);
        return;
    }
    fclose(headFile);

    strtok(commitHash, "\n"); // Remove trailing newline

    // Load current commit
    commit *currentCommit = loadCommit(commitHash);
    if (!currentCommit) return;

    if (!currentCommit->parentCommit || strcmp(currentCommit->parentCommit, "(null)") == 0) {
        printf("This is the first commit with no parent.\n");
        printf("Are you sure you want to revert to the initial state? (y/n): ");
        char response[10];
        scanf("%9s", response);
        if (strcmp(response, "y") == 0) {
            delete_directory(".trackit");
            init();
        } else {
            printf("Revert canceled.\n");
        }
        free(currentCommit);
        return;
    }

    // Load staging file hash from current commit
    char stagingFilePath[256];
    sprintf(stagingFilePath, "%s%s", OBJECTS_PATH, currentCommit->stagingFiles);
    FILE *stagingFile = fopen(stagingFilePath, "r");
    if (!stagingFile) {
        printf("Error: Staging file '%s' not found.\n", stagingFilePath);
        free(currentCommit);
        return;
    }

    // Delete files listed in staging file
    char line[1024];
    while (fgets(line, sizeof(line), stagingFile)) {
        strtok(line, "\n"); // Remove trailing newline
        //char *filePath = strtok(line, " "); // Extract file path
        char *fileHash = strtok(NULL, " "); // Extract file hash

        if (fileHash) {
            char objectFilePath[256];
            sprintf(objectFilePath, "%s%s", OBJECTS_PATH, fileHash);
            if (unlink(objectFilePath) == 0) {
                printf("Deleted object file: %s\n", objectFilePath);
            } else {
                printf("Error deleting object file: %s\n", objectFilePath);
            }
        }
    }
    fclose(stagingFile);

    // Delete staging file itself
    if (unlink(stagingFilePath) == 0) {
        printf("Deleted staging file: %s\n", stagingFilePath);
    } else {
        printf("Error deleting staging file: %s\n", stagingFilePath);
    }

    // delete commit file itself
    char commitFilePath[512];
    snprintf(commitFilePath, sizeof(commitFilePath), "%s%s", OBJECTS_PATH, commitHash);
    if (unlink(commitFilePath) == 0) {
        printf("Deleted staging file: %s\n", commitFilePath);
    } else {
        printf("Error deleting staging file: %s\n", commitFilePath);
    }

    // Update head file with parent commit hash
    headFile = fopen(HEAD_FILE_PATH, "w");
    if (!headFile) {
        printf("Error: Unable to write to head file.\n");
        free(currentCommit);
        return;
    }
    fprintf(headFile, "%s\n", currentCommit->parentCommit);
    fclose(headFile);

    printf("Reverted to parent commit: %s\n", currentCommit->parentCommit);
    free(currentCommit);
}

void clean_main_directory(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (!dp) {
        perror("Failed to open directory");
        return;
    }

    char full_path[MAX_LINE];
    while ((entry = readdir(dp)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Skip the .trackit folder
        if (strcmp(entry->d_name, ".trackit") == 0) {
            continue;
        }

        // Construct the full path
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            // If it's a directory, recursively delete its contents
            clean_main_directory(full_path);
            rmdir(full_path);
        } else {
            // If it's a file, delete it
            remove(full_path);
        }
    }

    closedir(dp);
}

// Function to create directories recursively
void create_directories(const char *path) {
    char tmp[MAX_LINE];
    snprintf(tmp, sizeof(tmp), "%s", path);
    char *p = tmp;

    // Create directories one by one
    while ((p = strchr(p, '/')) != NULL) {
        *p = '\0';
        mkdir(tmp);
        *p = '/';
        p++;
    }
}

// Function to extract the directory path from a full file path
void extract_directory_path(const char *full_path, char *directory_path) {
    strcpy(directory_path, full_path);
    char *last_slash = strrchr(directory_path, '/');
    if (last_slash) {
        *last_slash = '\0'; // Remove the file name to leave only the directory path
    } else {
        directory_path[0] = '\0'; // No directory path, just a file name
    }
}

// Function to remove double quotes from a string
void remove_quotes(char *str) {
    char *src = str, *dest = str;
    while (*src) {
        if (*src != '"') {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

// Function to restore files from the latest commit
void restore() {
    FILE *commit_file, *staging_file, *object_file, *output_file;
    char head_commit_hash[OUTPUT_LENGTH], staging_file_path[MAX_LINE], stagging_file_hash[OUTPUT_LENGTH];
    char file_path[MAX_LINE], file_hash[OUTPUT_LENGTH];
    char object_file_path[MAX_LINE], directory_path[MAX_LINE];
    char buffer[1024];
    size_t bytes_read;

    // Clean the main directory
    clean_main_directory(".");

    // Open the HEAD file to get the current head commit hash
    commit_file = fopen(HEAD_FILE_PATH, "r");
    if (!commit_file) {
        perror("Failed to open HEAD");
        return;
    }
    fscanf(commit_file, "%s", head_commit_hash);
    fclose(commit_file);

    // Open the head commit file
    snprintf(staging_file_path, sizeof(staging_file_path), "%s%s", OBJECTS_PATH, head_commit_hash);
    commit_file = fopen(staging_file_path, "r");
    if (!commit_file) {
        perror("Failed to open commit file");
        return;
    }

    // Skip timestamp and message lines
    fgets(stagging_file_hash, sizeof(staging_file_path), commit_file);
    fgets(stagging_file_hash, sizeof(staging_file_path), commit_file);

    // Read the staging file path
    fscanf(commit_file, "%s", stagging_file_hash);
    fclose(commit_file);

    // Open the staging file
    snprintf(staging_file_path, sizeof(staging_file_path), "%s%s", OBJECTS_PATH, stagging_file_hash);
    staging_file = fopen(staging_file_path, "r");
    if (!staging_file) {
        perror("Failed to open staging file");
        return;
    }

    // Process each entry in the staging file
    while (fscanf(staging_file, "%s %s", file_path, file_hash) == 2) {
        // Remove quotes from the file path
        remove_quotes(file_path);

        // Extract the directory path
        extract_directory_path(file_path, directory_path);

        // Create directories if the file path contains subdirectories
        if (strlen(directory_path) > 0) {
            create_directories(directory_path);
        }

        // Construct object file path
        snprintf(object_file_path, sizeof(object_file_path), "%s%s", OBJECTS_PATH, file_hash);

        // Open the object file
        object_file = fopen(object_file_path, "rb");
        if (!object_file) {
            perror("Failed to open object file");
            continue;
        }

        // Create and write to the output file
        output_file = fopen(file_path, "wb");
        if (!output_file) {
            perror("Failed to create file in main directory");
            fclose(object_file);
            continue;
        }

        // Read and write the file content in chunks
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), object_file)) > 0) {
            if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read) {
                perror("Error writing to target file");
                fclose(object_file);
                fclose(output_file);
                return;
            }
        }

        fclose(object_file);
        fclose(output_file);
    }

    fclose(staging_file);
    printf("Restore completed successfully.\n");
}


void display_help() {
    printf("Welcome to TrackIt - A Simple Version Control System\n");
    printf("====================================================\n");
    printf("Below are the available commands and their descriptions:\n\n");

    printf("1. groot init\n");
    printf("   - Initializes a new TrackIt repository in the current directory.\n");
    printf("   - Example: groot init\n\n");

    printf("2. groot add <filename>\n");
    printf("   - Adds a file to the staging area for tracking.\n");
    printf("   - Example: groot add myfile.txt\n\n");

    printf("3. groot commit -m \"<message>\"\n");
    printf("   - Saves changes in the staging area as a new commit with a message.\n");
    printf("   - Example: groot commit -m \"Initial commit\"\n\n");

    printf("4. groot revert\n");
    printf("   - Reverts the last commit\n");
    printf("   - Example: groot revert \n\n");

    printf("5. groot restore \n");
    printf("   - Restores all the files from the last commit(Last version)\n");
    printf("   - Example: groot restore \n\n");

    printf("====================================================\n");
    printf("Usage Notes:\n");
    printf("1. Begin by initializing the repository using 'groot init'.\n");
    printf("2. Add files to the staging area before committing them.\n");
    printf("3. Use meaningful commit messages with the '-m' flag in 'groot commit'.\n");
    printf("4. Use 'groot log' to view commit hashes for reverting or restoring files.\n\n");
}
