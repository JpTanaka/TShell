#include <errno.h> // handle errors
#include <stdio.h> //why h?
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 100

int write_to_console(int fd, char* str) { return write(fd, str, strlen(str)); }

int main()
{
    size_t len = 0;
    char* command = NULL;
    char* delim = " \n";
    while (1) {
        write_to_console(STDOUT_FILENO, "> ");
        getline(&command, &len, stdin);
        char* command_copy = strdup(command); // Is there a better way to handle this?
        char* token = strtok(command, delim);
        if (strcmp(token, "exit") == 0) {
            return 0;
        } else if (strcmp(token, "echo") == 0) {
            write_to_console(STDOUT_FILENO, command_copy + 4);
            continue;
        } else if (strcmp(token, "pwd") == 0) {
            char* cwd = NULL;
            if ((cwd = getcwd(NULL, 0)) == NULL) {
                write_to_console(STDOUT_FILENO,
                    "current directory was not found\n");
            } else {
                write_to_console(STDOUT_FILENO, cwd);
            }
        } else {
            char* not_found = malloc(sizeof(char) * (19 + strlen(token)));
            strcpy(not_found, "command not found: ");
            strcat(not_found, token);
            write(STDERR_FILENO, not_found, strlen(not_found));
        }
        write_to_console(STDOUT_FILENO, "\n");
    }

    return 0;
}