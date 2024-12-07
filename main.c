#include <stdio.h>
#include "header.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error: No command provided.\n");
        printf("Usage: %s <command>\n", argv[0]);
        printf("Available commands: init, add, commit, log\n");
        return 1;
    }

    if (strcmp(argv[1], "help") == 0){
        display_help();
    }else if (strcmp(argv[1], "init") == 0)
    {
        int a = init();
        if (a != 0)
        {
            printf("Trackit repo initialised successfully\n");
        }
        else
        {
            printf("Trackit repo initialisation failed\n");
        }
    }
    else if (strcmp(argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            printf("Error: No files provided to add.\n");
            return 1;
        }
        int n = add(argc - 2, &argv[2]);
        if (n == 0)
        {
            printf("Files added to staging area succesfully\n");
        }
        else
        {
            printf("Files could not add to stagging area\n");
        }
    }
    else if (strcmp(argv[1], "commit") == 0)
    {
        if (argc < 4 || strcmp(argv[2], "-m") != 0)
        {
            printf("Error: No commit message provided.\n");
            printf("Usage: %s commit -m \"Your commit message\"\n", argv[0]);
            return 1;
        }

        char *commitMessage = argv[3];
        commitFiles(commitMessage);
    }
    else if (strcmp(argv[1], "log") == 0){
        logHistory();
    }else if (strcmp(argv[1], "revert") == 0){
        revert();
    }
    else if (strcmp(argv[1], "restore") == 0){
        restore();
    }
    else
    {
        printf("Error: Unknown command '%s'.\n", argv[1]);
        printf("Available commands: init, commit, push\n");
        return 1;
    }

    return 0;
}
