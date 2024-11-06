#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

#define OUTPUT_LENGTH 40
#define MAX_LINE_LENGTH 256
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
        snprintf(hashedFilePath, sizeof(hashedFilePath), ".trackit/objects/%s", hash);

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
    char commitFilePath[512];
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