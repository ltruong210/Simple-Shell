#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

struct str_array {
        char **array;
        int size;
        int bg;
};

struct str_array get_command_and_args();
void free_str_array(char **str_array, int size);
void print_str_array(char **str_array, int size);
void handler(int sig);
void unix_error(char *msg);

int main() {
        pid_t pid;
        int status;

        pid_t *pid_array = NULL, *pid_array_temp = NULL;
        size_t pid_array_index = 0;
        size_t pid_array_size = 10;
        pid_array = malloc(pid_array_size * sizeof(pid_t));

        while (1) {
                struct str_array command_and_args;
                command_and_args = get_command_and_args();
                if (!command_and_args.array) {
                        printf("\n\nOut of memory NULL detected.\n\n");
                        free_str_array(command_and_args.array, command_and_args.size);
                        continue;
                } else if (command_and_args.size <= 0) {
                        free_str_array(command_and_args.array, command_and_args.size);
                        continue;
                } else {
                        if (strlen(command_and_args.array[0]) >= 4 && strncmp(command_and_args.array[0], "quit", 4) == 0) {
                                free_str_array(command_and_args.array, command_and_args.size);
                                break;
                        } else {
                                errno = 0;
                                if (command_and_args.bg == 1) {
                                        if (signal(SIGCHLD, handler) == SIG_ERR) {
                                                unix_error("\nsignal error\n");
                                                free_str_array(command_and_args.array, command_and_args.size);
                                                continue;
                                        }
                                }
                                pid = fork();
                                if (pid == 0) {
                                        if (execv(command_and_args.array[0], command_and_args.array) < 0) {
                                                exit(0);
                                        }
                                } else if (pid < 0) {
                                        unix_error("Fork error");
                                } 

                                if (command_and_args.bg == 0) {
                                        if ((pid = waitpid(pid, &status, 0)) > 0) {
                                                if (!WIFEXITED(status)) {
                                                        printf("child %d terminated abnormally\n", pid);
                                                }
                                        }
                                        if (errno != 0 && errno != ECHILD) {
                                                unix_error("waitpid error");
                                        }
                                } else {
                                        if (pid_array_index >= pid_array_size) {
                                                pid_array_size = pid_array_size * 2;
                                                pid_array_temp = realloc(pid_array, pid_array_size * sizeof(pid_t));
                                                if (!pid_array_temp) {
                                                                free(pid_array);
                                                                pid_array = NULL;
                                                                printf("\n\nMemory error reallocating for pid_array, reinitializing pid_array...\n");
                                                                pid_array_size = 1;
                                                                pid_array_temp = NULL;
                                                                pid_array_index = 0;
                                                                pid_array = malloc(pid_array_size * sizeof(pid_t));
                                                                break;
                                                } else if (pid_array != pid_array_temp) {
                                                                pid_array = pid_array_temp;
                                                }
                                                pid_array_temp = NULL;
                                        }
                                        pid_array[pid_array_index++] = pid;

                                }
                        }
                }
                free_str_array(command_and_args.array, command_and_args.size);

        }

        size_t i;
        for (i = 0; i < pid_array_index; i++) {
                kill(pid_array[i], SIGINT);
        }

        free(pid_array);
        exit(0);
}

struct str_array get_command_and_args() {
        int bg = 0;
        char **command_and_args = NULL, **str_array_temp = NULL;
        size_t str_array_size = 100, str_array_index = 0, occupation = 0;
        command_and_args = malloc(str_array_size);

        char *current_str = NULL, *char_array_temp = NULL;
        size_t char_array_size = 10, char_array_index = 0;
        current_str = malloc(char_array_size);
        memset(current_str, '\0', char_array_size);

        int current_char = EOF;
        printf("\n\nprompt> ");
        while (current_char) {
                        current_char = getchar();
                        if (current_char == EOF || current_char == '\n') {
                                        current_char = 0;
                        }

                        if (isspace(current_char) == 0 && current_char != 0) { 
                                        if (char_array_size <= char_array_index) {
                                                        char_array_size = 2 * char_array_size;
                                                        char_array_temp = realloc(current_str, char_array_size);
                                                        if (!char_array_temp) {
                                                                        free(current_str);
                                                                        current_str = NULL;
                                                                        free_str_array(command_and_args, str_array_index);
                                                                        command_and_args = NULL;
                                                                        break;
                                                        } else if (current_str != char_array_temp) {
                                                                        current_str = char_array_temp;
                                                        }
                                                        char_array_temp = NULL;
                                        }

                                        current_str[char_array_index++] = current_char;
                        } else if (char_array_index > 0 && isspace(current_str[char_array_index - 1]) == 0 ) {
                                        occupation = occupation + char_array_size;
                                        if (str_array_size <= occupation) {
                                                        str_array_size = 2 * occupation;
                                                        str_array_temp = realloc(command_and_args, str_array_size);
                                                        if (!str_array_temp) {
                                                                        free(current_str);
                                                                        current_str = NULL;
                                                                        free_str_array(command_and_args, str_array_index);
                                                                        command_and_args = NULL;
                                                                        struct str_array results = {NULL, -1, -1};
                                                                        return results;
                                                        } else if (command_and_args != str_array_temp) {
                                                                        command_and_args = str_array_temp;
                                                        }
                                                        str_array_temp = NULL;
                                        }
                                        
                                        command_and_args[str_array_index] = malloc(char_array_size);
                                        strcpy(command_and_args[str_array_index++], current_str);

                                        bg = 0;
                                        if (current_str[char_array_index - 1] == '&') {
                                                bg = 1;
                                        } else {
                                        }
                                        memset(current_str, '\0', char_array_size);
                                        char_array_index = 0;
                        }
        }
        
        if (bg == 1) {
                if (command_and_args[str_array_index - 1][strlen(command_and_args[str_array_index - 1]) - 1] == '&') {
                        command_and_args[str_array_index - 1][strlen(command_and_args[str_array_index - 1]) - 1] = '\0';
                        if (command_and_args[str_array_index - 1][0] == '\0') {
                                free(command_and_args[--str_array_index]);
                        }
                }                        
        }
        command_and_args[str_array_index] = NULL;
        free(current_str);
        struct str_array results = {command_and_args, str_array_index, bg};

        return results;
}

void free_str_array(char **str_array, int size) {
        int i;
        for (i = 0; i < size; i++) {
                free(str_array[i]);
        }
        free(str_array);
}

void print_str_array(char **str_array, int size) {
        int i;
        printf("\n\nPrinting str_array of size %i\n", (int) size);
        for (i = 0; i < size; i++) {
                printf("index %i has str %s\n", (int) i, str_array[i]);
        }
        printf("\nFinished printing str_array of size %i\n", (int) size);
}

void handler(int sig) {
        pid_t pid;
        int status;
        int counter = 0;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                counter = counter + 1;
        }
        if (errno != 0 && errno != ECHILD) {
                unix_error("waitpid error");
        }

        return;

}

void unix_error(char *msg) {
        fprintf(stderr, "\n%s: %s\n", msg, strerror(errno));
}