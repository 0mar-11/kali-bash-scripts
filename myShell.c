#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_HIST 10

char *history[MAX_HIST];
int hist_count = 0;
volatile pid_t child_pid = 0; 

// ================= HISTORY MANAGEMENT =================
void add_to_history(char *line) {
    if (strlen(line) == 0) return; // #include <string.h>
    if (hist_count < MAX_HIST) {
        history[hist_count++] = strdup(line); // memory mangment (Dynamic allocation)
    } else {
        free(history[0]); //  ده Memory Leak Prevention
        for (int i = 1; i < MAX_HIST; i++)
            history[i-1] = history[i];
        history[MAX_HIST-1] = strdup(line);
    }
}

// ================= SIGNAL HANDLING =================
void handle_sigint(int sig) { //sigint interrupt = Ctrl+C
    write(STDOUT_FILENO, "\n", 1);
    if (child_pid > 0) { //parent
        kill(child_pid, SIGINT); // kill the process
    }
}

void handle_sigchld(int sig) { //sigchld interrupt =  when end child process (afteer kill)
    while (waitpid(-1, NULL, WNOHANG) > 0); // no zombie process 
}

// ================= PARSING =================
int parse_args(char *line, char **argv) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

// ================= EXECUTION (FOREGROUND/BACKGROUND + REDIRECTION) =================
int run_command(char **argv, char *in_file, char *out_file, int bg) {
    pid_t pid = fork();
    if (pid < 0) { //error handling -- no memory/  max pid
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        // Child Process
        // Input Redirection < //stdin
        if (in_file != NULL) {
            int fd = open(in_file, O_RDONLY);
            if (fd < 0) { perror(in_file); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        // Output Redirection > //stdout
        if (out_file != NULL) {
            int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);// حثقةهسسهخىس
            if (fd < 0) { perror(out_file); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(argv[0], argv);
        perror(argv[0]); // Only reached if exec fails
        exit(1);
    } else {
        // Parent Process
        child_pid = pid;
        if (bg) {
            printf("Process ID: %d\n", pid);
            child_pid = 0; // Parent doesn't wait for background
        } else {
            waitpid(pid, NULL, 0);
            child_pid = 0;
        }
        return pid;
    }
}

// ================= PIPELINE HANDLING =================
void run_pipeline(char *cmd1_str, char *cmd2_str) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }

    char *argv1[MAX_ARGS], *argv2[MAX_ARGS];
    char c1[MAX_LINE], c2[MAX_LINE];
    strncpy(c1, cmd1_str, MAX_LINE-1); c1[MAX_LINE-1] = '\0';
    strncpy(c2, cmd2_str, MAX_LINE-1); c2[MAX_LINE-1] = '\0';

    parse_args(c1, argv1);
    parse_args(c2, argv2);

    if (argv1[0] == NULL || argv2[0] == NULL) {
        close(pipefd[0]); close(pipefd[1]);
        return;
    }

    // First command (writes to pipe) //stdout
    p1 = fork();
    if (p1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);// [1] = write //stdout to pipe
        close(pipefd[0]); close(pipefd[1]); // [0] read
        execvp(argv1[0], argv1);
        perror(argv1[0]);
        exit(1);
    }

    // Second command (reads from pipe) //stdin
    p2 = fork();
    if (p2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);// [0] read // stdin input from pipe
        close(pipefd[0]); close(pipefd[1]);
        execvp(argv2[0], argv2);
        perror(argv2[0]);
        exit(1);
    }

    // Parent closes pipe ends and waits
    close(pipefd[0]); close(pipefd[1]);
    waitpid(p1, NULL, 0);// no zombie
    waitpid(p2, NULL, 0);//no zombie
}

// ================= MAIN LOOP =================
int main(void) {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];
    int bg;

    signal(SIGINT, handle_sigint);
    signal(SIGCHLD, handle_sigchld);// kill child
    signal(SIGTSTP, SIG_IGN); // Optional: Ignore Ctrl+Z

    while (1) {
        printf("myShell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break; // Ctrl+D
        }

        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        add_to_history(line);

        // 1. Check for Pipe |
        char *pipe_ptr = strchr(line, '|');
        if (pipe_ptr) {
            *pipe_ptr = '\0';
            run_pipeline(line, pipe_ptr + 1);
            continue;
        }

        // 2. Parse arguments safely
        char line_copy[MAX_LINE];
        strncpy(line_copy, line, MAX_LINE-1);
        line_copy[MAX_LINE-1] = '\0';
        int argc = parse_args(line_copy, argv);
        if (argc == 0) continue;

        // 3. Built-in Commands
        if (strcmp(argv[0], "exit") == 0) {
            for (int i = 0; i < hist_count; i++) free(history[i]);
            exit(0);
        }
        if (strcmp(argv[0], "cd") == 0) {
            if (argv[1] == NULL) fprintf(stderr, "cd: missing argument\n");
            else if (chdir(argv[1]) != 0) perror("cd");
            continue;
        }
        if (strcmp(argv[0], "pwd") == 0) {
            char cwd[MAX_LINE];
            if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
            else perror("pwd");
            continue;
        }
        if (strcmp(argv[0], "history") == 0) {
            for (int i = 0; i < hist_count; i++) printf("%d %s\n", i+1, history[i]);
            continue;
        }

        // 4. Check for Background &
        bg = 0;
        if (argc > 0 && strcmp(argv[argc-1], "&") == 0) {
            bg = 1;
            argv[argc-1] = NULL;
            argc--;
        }

        // 5. Check for Redirection < and >
        char *in_file = NULL;
        char *out_file = NULL;
        int new_argc = 0;
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], ">") == 0 && i + 1 < argc) {
                out_file = argv[i+1];
                i++; // Skip filename
            } else if (strcmp(argv[i], "<") == 0 && i + 1 < argc) {
                in_file = argv[i+1];
                i++; // Skip filename
            } else {
                argv[new_argc++] = argv[i];
            }
        }
        argv[new_argc] = NULL;
        if (argv[0] == NULL) continue;

        // 6. Execute external command
        run_command(argv, in_file, out_file, bg);
    }
    return 0;
}
