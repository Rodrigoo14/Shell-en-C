#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_LINE 90
#define PATH_MAX 4096
#define HISTORY_COUNT 10

char *history[HISTORY_COUNT];
int history_index = 0;

void parseInput(char *line, char **args) {
    int i = 0;
    char *token = strtok(line, " \n");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " \n");
    }
    args[i] = NULL;
}

void executeCommand(char **args) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Error en fork");
        exit(1);
    } else if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("Error en execvp");
            exit(1);
        }
    } else {
        wait(NULL);
    }
}

void executePipedCommand(char **args1, char **args2) {
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Error en pipe");
        exit(1);
    }
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("Error en fork");
        exit(1);
    } else if (pid1 == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (execvp(args1[0], args1) == -1) {
            perror("Error en execvp");
            exit(1);
        }
    }
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("Error en fork");
        exit(1);
    } else if (pid2 == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        if (execvp(args2[0], args2) == -1) {
            perror("Error en execvp");
            exit(1);
        }
    }
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void addHistory(char *line) {
    if (history[history_index]) {
        free(history[history_index]);
    }
    history[history_index] = strdup(line);
    history_index = (history_index + 1) % HISTORY_COUNT;
}

void showHistory() {
    printf("Historial de comandos:\n");
    for (int i = 0; i < HISTORY_COUNT; i++) {
        int idx = (history_index + i) % HISTORY_COUNT;
        if (history[idx]) {
            printf("%d: %s\n", i + 1, history[idx]);
        }
    }
}

void showHelp() {
    printf("Comandos disponibles:\n");
    printf("cd <dir>: Cambia el directorio actual a <dir>\n");
    printf("exit: Cierra la shell\n");
    printf("history: Muestra el historial de comandos\n");
    printf("help: Muestra esta ayuda\n");
    printf("color <color>: Cambia el color del prompt (rojo, verde, azul, por defecto)\n");
}

void setColor(char *color) {
    if (strcmp(color, "rojo") == 0) {
        printf("\033[0;31m");
    } else if (strcmp(color, "verde") == 0) {
        printf("\033[0;32m");
    } else if (strcmp(color, "azul") == 0) {
        printf("\033[0;34m");
    } else {
        printf("\033[0m");
    }
}

int main() {
    char *args[MAX_LINE / 2 + 1];
    char line[MAX_LINE];
    char cwd[PATH_MAX];
    char *args2[MAX_LINE / 2 + 1];
    char *defaultColor = "\033[0m";

    while (1) {
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("epis::%s> ", cwd);
        } else {
            perror("getcwd() error");
            return 1;
        }

        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                break;
            } else {
                perror("Error leyendo línea");
                continue;
            }
        }

        addHistory(line);

        parseInput(line, args);

        if (args[0] == NULL) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "epis: se requiere un argumento para \"cd\"\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("epis");
                }
            }
            continue;
        }

        if (strcmp(args[0], "history") == 0) {
            showHistory();
            continue;
        }

        if (strcmp(args[0], "help") == 0) {
            showHelp();
            continue;
        }

        if (strcmp(args[0], "color") == 0) {
            if (args[1] == NULL) {
                printf("epis: se requiere un argumento para \"color\"\n");
            } else {
                setColor(args[1]);
            }
            continue;
        }

        int i = 0;
        int pipeFound = 0;
        while (args[i] != NULL) {
            if (strcmp(args[i], "|") == 0) {
                args[i] = NULL;
                pipeFound = 1;
                break;
            }
            i++;
        }

        if (pipeFound) {
            int j = 0;
            i++;
            while (args[i] != NULL) {
                args2[j++] = args[i++];
            }
            args2[j] = NULL;
            executePipedCommand(args, args2);
        } else {
            executeCommand(args);
        }
    }

    for (int i = 0; i < HISTORY_COUNT; i++) {
        free(history[i]);
    }
    
    printf("%s", defaultColor); // Reset color before exit
    return 0;
}
