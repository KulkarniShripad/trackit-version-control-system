#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <openssl/evp.h>

typedef struct node
{
    char filepath[100];
    char hash[40];
    struct node *next;
} node;

typedef struct index
{
    struct node *head;
    struct node *tail;
} index;

typedef struct commit
{
    char *timestamp;
    char *message;
    char *stagingFiles;
    char *parentCommit;
    struct commit *next;
} commit;

//function for initialization
int init();


//functions for add 
char *prependDotSlash(const char *filename);
char *generateHash(const char *filepath);
int add(int fileCount, char *filePath[]);
index *getStoredIndex();
void storeToIndex(index *stagging);
void updateStagingArea(char *filename, char *hash);


//functions for commit
char *getCurrentTimestamp();
char *getParentHash(const char *fileName);
int storeIndexFile(char *hash);
void commitFiles(char *message);


//functions for log
commit *loadCommit(const char *commitHash);
char *getHeadCommitHash();
void printCommitHistory(commit *head);
void freeCommitHistory(commit *head);
void logHistory();