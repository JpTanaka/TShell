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


void echo(size_t argc, char** argv)
{
    int print_newline = 1;
    while (argc > 1) {
        if (argv[0][0] == '-') {
            if (argv[0][1] == 'n') {
                print_newline = 0;
            } else {
                break;
            }
            argc--;
            argv++;
        } else {
            break;
        }
    }
    for (size_t i = 0; i < argc; i++) {
        if (i >= 1)
            fprintf(stdout, DELIMITERS);
        fprintf(stdout, "%s", argv[i]);
    }
    if (print_newline) {
        fprintf(stdout, "\n");
    }
}

void get_current_directory(char** cwd)
{
    if (((*cwd) = getcwd(NULL, 0)) == NULL) {
        perror("current directory not found");
    }
}

void pwd()
{
    char* cwd;
    get_current_directory(&cwd);
    if (cwd == NULL)
        return;
    fprintf(stdout, "%s\n", cwd);
}

void list_directories()
{
    char* cwd = NULL;
    get_current_directory(&cwd);
    if (cwd == NULL)
        return;
    DIR* fd = opendir(cwd);
    if(fd == NULL) {
        perror("directory could not be opened");
    }
    struct dirent* dir;
    int jump_line = 0;
    while ((dir = readdir(fd)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;
        fprintf(stdout, "%s%s", jump_line ? "\n" : "", dir->d_name);
        if (!jump_line)
            jump_line = 1;
    }
    fprintf(stdout, "\n");
}

void copy_files(size_t argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "number of arguments (%ld) must be greater than 2", argc);
        return;
    }
    char* cwd;
    get_current_directory(&cwd);
    struct stat s;
    stat(cwd, &s);
    int fd_src = open(argv[0], O_RDONLY);
    int fd_dest = open(argv[1], O_WRONLY | O_APPEND | O_CREAT | O_EXCL, s.st_mode);
    ssize_t buf_length = 1024;
    char buf_read[buf_length];
    ssize_t bytes_read;
    do {
        bytes_read = read(fd_src, buf_read, buf_length);
        if(write(fd_dest, buf_read, bytes_read) == -1) {
            perror("failed to copy");
        };
    } while (bytes_read == buf_length);
    close(fd_src);
    close(fd_dest);
}

void change_directory(size_t argc, char** argv)
{
    if (argc == 0) {
        return;
    }
    if (chdir(argv[0]) == -1) {
        fprintf(stderr, "cd: %s %s\n", strerror(errno), argv[0]);
    };
}

void parse_operators(char** command, int* saved_in, int* fd_in, int* saved_out, int* fd_out)
{
    char char_redirect_out = '>';
    char char_redirect_in = '<';
    char* cwd;
    char* filename;
    get_current_directory(&cwd);
    struct stat s;
    stat(cwd, &s);
    char* pos = strchr(*command, char_redirect_out);
    if (pos != NULL) {
        filename = strtok(pos + 1, DELIMITERS);
        *fd_out = open(filename, O_WRONLY | O_CREAT, s.st_mode);
        *saved_out = dup(STDOUT_FILENO);
        dup2(*fd_out, STDOUT_FILENO);
        pos[0] = '\0';
    }
    pos = strchr(*command, char_redirect_in);
    if (pos != NULL) {
        filename = strtok(pos + 1, DELIMITERS);
        *fd_in = open(filename, O_RDONLY, s.st_mode);
        *saved_in = dup(STDIN_FILENO);
        dup2(*fd_in, STDIN_FILENO);
        pos[0] = '\0';
    }
}