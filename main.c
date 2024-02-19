#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "main.h"
#include "built_in_functions.h"


int parser(char* command)
{
    command = strtok(command, NEWLINE);
    if (command == NULL) {
        return 0;
    }
    int saved_out = -1;
    int saved_in = -1;
    int fd_out = -1;
    int fd_in = -1;
    parse_operators(&command, &saved_in, &fd_in, &saved_out, &fd_out);
    char* token = strtok(command, DELIMITERS);
    char* argv[MAX_ARGC];
    size_t argc = 0;
    char* arg;
    while ((arg = strtok(NULL, DELIMITERS)) != NULL) {
        argv[argc] = strdup(arg);
        argc++;
    }
    if (strcmp(token, "exit") == 0) {
        return -1;
    } else if (strcmp(token, "echo") == 0) {
        echo(argc, argv);
    } else if (strcmp(token, "pwd") == 0) {
        pwd(argc, argv);
    } else if (strcmp(token, "ls") == 0) {
        list_directories(argc, argv);
    } else if (strcmp(token, "cd") == 0) {
        change_directory(argc, argv);
    } else if (strcmp(token, "cp") == 0) {
        copy_files(argc, argv);
    } else {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            for (int i = argc; i > 0; i--) {
                argv[i] = argv[i - 1];
            }
            argv[0] = malloc(strlen(token) + 1);
            strcpy(argv[0], token);
            argc++;
            argv[argc] = NULL;
            if (execvp(token, argv) == -1) {
                fprintf(stderr, "command not found: %s\n", token);
                exit(0);
            }
            free(argv[0]);
        } else
            wait(NULL);
    }
    for (size_t i = 0; i < argc; i++) {
        free(argv[i]);
    }
    if (arg != NULL)
        free(arg);
    if (fd_out != -1) {
        close(fd_out);
        dup2(saved_out, STDOUT_FILENO);
    }
    if (fd_in != -1) {
        close(fd_in);
        dup2(saved_in, STDIN_FILENO);
    }
    return 0;
}

int main()
{
    size_t len = 0;
    char* command = NULL;
    char* cwd;
    while (1) {
        get_current_directory(&cwd);
        fprintf(stdout, "%s>", cwd);
        if (getline(&command, &len, stdin) == -1) {
            perror("failed to getline");
            continue;
        };
        if (parser(command) == -1) {
            break;
        }
    }
    return 0;
}