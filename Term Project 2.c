#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define A 1024
#define B 100
#define C 100
#define MAX_ARGS 100

char *history_storage[C];
int history_count = 0;


//  Ahnaf starts here: Basic Shell Functionality & Built-in Commands

void handle_sigint(int signum) {
    write(STDOUT_FILENO, "\nsh> ", 5);
}

void add_to_history(const char *command) {
    if (history_count < C)
        history_storage[history_count++] = strdup(command);
}

void print_history() {
    for (int i = 0; i < history_count; i++)
        printf("%d: %s\n", i + 1, history_storage[i]);
}

void parse_and_execute(char *line); // Forward declaration

int main() {
    signal(SIGINT, handle_sigint);

    char input_buffer[A];

    while (1) {
        printf("sh> ");
        if (!fgets(input_buffer, A, stdin)) break;

        if (strcmp(input_buffer, "exit\n") == 0) break;

        input_buffer[strcspn(input_buffer, "\n")] = 0;
        if (strlen(input_buffer) == 0)
            continue;

        add_to_history(input_buffer);
        parse_and_execute(input_buffer);
    }

    return 0;
}

//  Ahnaf ends here



// Leon starts here: Redirection & Piping

void execute_single_command(char *cmd) {
    char *args[MAX_ARGS];
    int argc = 0;
    int in_redir = 0, out_redir = 0, append_redir = 0;
    char *input_file = NULL, *output_file = NULL;

    char *token = strtok(cmd, " \t\n");
    while (token) {
        if (strcmp(token, "<") == 0) {
            in_redir = 1;
            input_file = strtok(NULL, " \t\n");
        } else if (strcmp(token, ">>") == 0) {
            append_redir = 1;
            output_file = strtok(NULL, " \t\n");
        } else if (strcmp(token, ">") == 0) {
            out_redir = 1;
            output_file = strtok(NULL, " \t\n");
        } else {
            args[argc++] = token;
        }
        token = strtok(NULL, " \t\n");
    }
    args[argc] = NULL;

    if (args[0] == NULL) return;

    pid_t pid = fork();
    if (pid == 0) {
        if (in_redir && input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) { perror("input"); exit(1); }
            dup2(fd, STDIN_FILENO); close(fd);
        }

        if ((out_redir || append_redir) && output_file) {
            int fd;
            if (append_redir) {
                fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            if (fd < 0) { perror("output"); exit(1); }
            dup2(fd, STDOUT_FILENO); close(fd);
        }

        execvp(args[0], args);
        perror("execvp");
        exit(1);
    } else {
        waitpid(pid, NULL, 0);
    }
}

void execute_pipeline(char *line) {
    char *commands[10];
    int num_cmds = 0;

    commands[num_cmds++] = strtok(line, "|");
    while ((commands[num_cmds] = strtok(NULL, "|")) != NULL)
        num_cmds++;

    int fd[2], in_fd = 0;

    for (int i = 0; i < num_cmds; i++) {
        pipe(fd);
        pid_t pid = fork();

        if (pid == 0) {
            if (i > 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
            }
            close(fd[0]);

            execute_single_command(commands[i]);
            exit(0);
        } else {
            waitpid(pid, NULL, 0);
            close(fd[1]);
            in_fd = fd[0];
        }
    }
}

//Leon ends here


//  Pranty starts here: Signal Handling, Logical Operators, History

void handle_input_line(char *line) {
    char *and_part = strtok(line, "&&");

    while (and_part) {
        char *semi_part = strtok(and_part, ";");

        while (semi_part) {
            while (*semi_part == ' ') semi_part++;

            if (strlen(semi_part) == 0) {
                semi_part = strtok(NULL, ";");
                continue;
            }

            if (strcmp(semi_part, "history") == 0) {
                print_history();
            }
            else if (strchr(semi_part, '|')) {
                execute_pipeline(semi_part);
            }
            else {
                execute_single_command(semi_part);
            }

            semi_part = strtok(NULL, ";");
        }

        and_part = strtok(NULL, "&&");
    }
}

void parse_and_execute(char *line) {
    handle_input_line(line);
}

// Pranty ends here

