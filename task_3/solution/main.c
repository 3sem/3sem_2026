#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#include "fileTransfer/transfer.h"

static struct option long_options[] = {
    {"send",    no_argument,       0, 's'},
    {"read",    no_argument,       0, 'r'},
    {"input",   required_argument, 0, 'i'},
    {"output",  required_argument, 0, 'o'},
    {0, 0, 0, 0} 
};

enum Mode{
    SEND,
    READ
};

void flagsHandle(int argc, char* argv[], const char** inputFileName, const char** outputFileName, enum Mode* mode);

int main(int argc, char* argv[]){
    const char* inputFileName  = "input.bin";
    const char* outputFileName = "output.bin";

    enum Mode mode = SEND; 

    flagsHandle(argc, argv, &inputFileName, &outputFileName, &mode);

    char fifoName[] = "/tmp/myFifo";

    switch(mode){
        case SEND:
            send(fifoName, inputFileName);
            break;
        case READ:
            receive(fifoName, outputFileName);
            break;
    }

    return 0;
}

void flagsHandle(int argc, char* argv[], const char** inputFileName, const char** outputFileName, enum Mode* mode){
    assert(argv);
    assert(inputFileName);
    assert(outputFileName);
    assert(mode);

    int opt = 0;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "sro:i:", long_options, &option_index)) != -1) {
        switch (opt){
            case 's':
                *mode = SEND;
                break;
            case 'r':
                *mode = READ;
                break;
            case 'i':
                *inputFileName = optarg;
                printf("%s\n", *inputFileName); 
                break;
            case 'o':
                *outputFileName = optarg;
                printf("%s\n", *outputFileName); 
                break;
            default:
                break;
        }
    }
}
