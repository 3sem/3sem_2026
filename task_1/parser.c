#include "parser.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ARGS_CAP 16
#define INITIAL_CMDS_CAP 8

void free_argv(char **argv) {
    if (!argv) {
        return;
    }
    for (size_t i = 0; argv[i] != NULL; ++i) {
        free(argv[i]);
    }
    free(argv);
}

void free_pipeline(char ***cmds) {
    if (!cmds) {
        return;
    }
    for (size_t i = 0; cmds[i] != NULL; ++i) {
        free_argv(cmds[i]);
    }
    free(cmds);
}

char **parse_single_command(const char *cmd_str) {
    size_t cap = INITIAL_ARGS_CAP;
    size_t argc = 0;
    char **argv = malloc(cap * sizeof(char *));
    if (!argv) {
        perror("malloc");
        return NULL;
    }

    size_t len = strlen(cmd_str);
    char *token_buf = malloc(len + 1);
    if (!token_buf) {
        perror("malloc");
        free(argv);
        return NULL;
    }

    const char *p = cmd_str;
    while (*p != '\0') {
        while (isspace((unsigned char)*p)) {
            ++p;
        }
        if (*p == '\0') {
            break;
        }

        size_t t_idx = 0;
        char quote = '\0';

        while (*p != '\0') {
            if (quote == '\0' && isspace((unsigned char)*p)) {
                break;
            }
            if (quote == '\0' && (*p == '\'' || *p == '"')) {
                quote = *p++;
                continue;
            }
            if (quote != '\0' && *p == quote) {
                quote = '\0';
                ++p;
                continue;
            }
            if (*p == '\\' && quote != '\'' && p[1] != '\0') {
                ++p;
                token_buf[t_idx++] = *p++;
                continue;
            }
            token_buf[t_idx++] = *p++;
        }

        if (quote != '\0') {
            fprintf(stderr, "mysh: синтаксическая ошибка: незакрытая кавычка (%c)\n", quote);
            free(token_buf);
            argv[argc] = NULL;
            free_argv(argv);
            return NULL;
        }

        token_buf[t_idx] = '\0';
        char *arg = strdup(token_buf);
        if (!arg) {
            perror("strdup");
            free(token_buf);
            argv[argc] = NULL;
            free_argv(argv);
            return NULL;
        }

        if (argc + 1 >= cap) {
            cap *= 2;
            char **new_argv = realloc(argv, cap * sizeof(char *));
            if (!new_argv) {
                perror("realloc");
                free(arg);
                free(token_buf);
                argv[argc] = NULL;
                free_argv(argv);
                return NULL;
            }
            argv = new_argv;
        }
        argv[argc++] = arg;
    }

    free(token_buf);
    argv[argc] = NULL;
    return argv;
}

char ***parse_pipeline(const char *line, size_t *out_num_cmds) {
    size_t cap = INITIAL_CMDS_CAP;
    size_t num_cmds = 0;
    char ***cmds = malloc(cap * sizeof(char **));
    if (!cmds) {
        perror("malloc");
        return NULL;
    }

    char quote = '\0';
    const char *seg_start = line;
    const char *p = line;

    for (;; ++p) {
        if (*p == '\\' && quote != '\'' && p[1] != '\0') {
            ++p;
            continue;
        }
        if (*p == '\'' || *p == '"') {
            if (quote == '\0') {
                quote = *p;
            } else if (quote == *p) {
                quote = '\0';
            }
            continue;
        }

        if (*p == '|' && quote == '\0') {
            size_t seg_len = (size_t)(p - seg_start);
            char *segment = malloc(seg_len + 1);
            if (!segment) {
                perror("malloc");
                cmds[num_cmds] = NULL;
                free_pipeline(cmds);
                return NULL;
            }
            memcpy(segment, seg_start, seg_len);
            segment[seg_len] = '\0';

            char **argv = parse_single_command(segment);
            free(segment);
            if (!argv || argv[0] == NULL) {
                fprintf(stderr, "mysh: синтаксическая ошибка: пустая команда в конвейере около '|'\n");
                free_argv(argv);
                cmds[num_cmds] = NULL;
                free_pipeline(cmds);
                return NULL;
            }

            if (num_cmds + 1 >= cap) {
                cap *= 2;
                char ***new_cmds = realloc(cmds, cap * sizeof(char **));
                if (!new_cmds) {
                    perror("realloc");
                    free_argv(argv);
                    cmds[num_cmds] = NULL;
                    free_pipeline(cmds);
                    return NULL;
                }
                cmds = new_cmds;
            }
            cmds[num_cmds++] = argv;
            seg_start = p + 1;
        }

        if (*p == '\0') {
            size_t seg_len = (size_t)(p - seg_start);
            char *segment = malloc(seg_len + 1);
            if (!segment) {
                perror("malloc");
                cmds[num_cmds] = NULL;
                free_pipeline(cmds);
                return NULL;
            }
            memcpy(segment, seg_start, seg_len);
            segment[seg_len] = '\0';

            char **argv = parse_single_command(segment);
            free(segment);
            if (!argv) {
                cmds[num_cmds] = NULL;
                free_pipeline(cmds);
                return NULL;
            }

            if (argv[0] == NULL) {
                free_argv(argv);
                if (num_cmds > 0) {
                    fprintf(stderr, "mysh: синтаксическая ошибка: пустая команда в конце конвейера\n");
                    cmds[num_cmds] = NULL;
                    free_pipeline(cmds);
                    return NULL;
                }
                free(cmds);
                *out_num_cmds = 0;
                return NULL;
            }

            if (num_cmds + 1 >= cap) {
                cap *= 2;
                char ***new_cmds = realloc(cmds, cap * sizeof(char **));
                if (!new_cmds) {
                    perror("realloc");
                    free_argv(argv);
                    cmds[num_cmds] = NULL;
                    free_pipeline(cmds);
                    return NULL;
                }
                cmds = new_cmds;
            }
            cmds[num_cmds++] = argv;
            break;
        }
    }

    cmds[num_cmds] = NULL;
    *out_num_cmds = num_cmds;
    return cmds;
}