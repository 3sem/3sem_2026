#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#include "input.h"

size_t getNumOfChar(const char *str, char searchedChar) {
    assert(str);

    size_t numOfChar = 0;

    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] == searchedChar)
            ++numOfChar;
    }

    return numOfChar;
}

size_t getNumOfTokens(const char *str) {
    assert(str);

    size_t numOfTokens = 0;
    int isNewToken = 1;

    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (isspace(str[i])) {
            isNewToken = 1;
        } else if (isNewToken) {
            ++numOfTokens;
            isNewToken = 0;
        }
    }

    return numOfTokens;
}

void inputCleanup(input_t *input) {
    if (!input)
        return;

    if (input->tokens != NULL) {
        for (size_t i = 0; input->tokens[i]; ++i)
            free(input->tokens[i]);

        free(input->tokens);
    }

    free(input->inputBuf);

    input->inputBuf = NULL;
    input->tokens = NULL;
}

char** getCommands (input_t *input) {
    assert(input);

    input->commandsNum = getNumOfChar(input->inputBuf, '|') + 1;
    char **commands = (char **)calloc(input->commandsNum + 1, sizeof(*commands));

    if (!commands) {
        perror("calloc");
        return NULL;
    }

    size_t commandsRead = 0;
    char *curCmd = strtok(input->inputBuf, "|");

    while (curCmd != NULL && commandsRead < input->commandsNum) {
        commands[commandsRead++] = curCmd;
        curCmd = strtok(NULL, "|");
    }

    if (commandsRead != input->commandsNum) {
        fprintf(stderr, "Error: empty command\n");
        free(commands);
        return NULL;
    }

    return commands;
}

int getTokens(input_t * input, char** commands) {
    assert(input);
    assert(commands);
    assert(*commands);

    input->tokens = (char ***)calloc(input->commandsNum + 1, sizeof(*input->tokens));
    if (!input->tokens) {
        perror("calloc");
        return -1;
    }

    const char tokensDelim[] = " \t\r\n";

    for (size_t curCmdIndex = 0; curCmdIndex < input->commandsNum; ++curCmdIndex) {
        size_t numOfTokens = getNumOfTokens(commands[curCmdIndex]);

        if (numOfTokens == 0) {
            fprintf(stderr, "Error: empty command\n");
            return -1;
        }

        input->tokens[curCmdIndex] = (char **)calloc(numOfTokens + 1, sizeof(*input->tokens[curCmdIndex]));
        if (!input->tokens[curCmdIndex]) {
            perror("calloc");
            return -1;
        }

        size_t tokNum = 0;
        char *curTok = strtok(commands[curCmdIndex], tokensDelim);

        while (curTok != NULL) {
            input->tokens[curCmdIndex][tokNum++] = curTok;
            curTok = strtok(NULL, tokensDelim);
        }

        input->tokens[curCmdIndex][tokNum] = NULL;
    }

    input->tokens[input->commandsNum] = NULL;

    return 0;
}


int getInput(input_t *input) {
    assert(input);

    size_t inputSize = 0;
    ssize_t numOfReadSymbols = getline(&input->inputBuf, &inputSize, stdin);
    if (numOfReadSymbols == -1) {
        perror("getline");
        inputCleanup(input);
        return 0;
    }

    char **commands = getCommands(input);
    if (!commands) {
        inputCleanup(input);
        return -1;
    }

    if (getTokens(input, commands) == -1) {
        free(commands);
        inputCleanup(input);
        return -1;
    }

    free(commands);
    return 1;
}
