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

#define MAX_PATH_LENGTH 100
#define DELIMITERS " "
#define NEWLINE "\n"
#define MAX_ARGC 256
#define MAX_ARGV_SIZE 1024

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