#ifndef INPUT_H
#define INPUT_H

typedef struct input_t {
    char *inputBuf;
    size_t commandsNum;
    char ***tokens;
} input_t;

size_t getNumOfChar(const char *str, char searchedChar);

size_t getNumOfTokens(const char *str);

void inputCleanup(input_t *input);

char** getCommands (input_t *input);

int getTokens(input_t * input, char** commands);

int getInput(input_t *input);

#endif
