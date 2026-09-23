#include <stdio.h>
#include <getopt.h>
#include <assert.h>

#include "fileTransfer/transfer.h"

static struct option long_options[] = {
    {"send",    no_argument,       0, 's'},
    {"read",    no_argument,       0, 'r'},
    {"file",    required_argument, 0, 'f'},
    {0, 0, 0, 0} 
};

enum Mode{
    SEND,
    READ
};

void flagsHandle(int argc, char* argv[], const char** fileName, enum Mode* mode);

int main(int argc, char* argv[]){
    const char* fileName  = "input.bin";

    enum Mode mode = SEND; 

    flagsHandle(argc, argv, &fileName, &mode);

    char fifoName[] = "/tmp/myFifo";

    switch(mode){
        case SEND:
            send(fifoName, fileName);
            break;
        case READ:
            receive(fifoName, fileName);
            break;
    }

    return 0;
}

void flagsHandle(int argc, char* argv[], const char** fileName, enum Mode* mode){
    assert(argv);
    assert(fileName);
    assert(mode);

    int opt = 0;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "srf:", long_options, &option_index)) != -1) {
        switch (opt){
            case 's':
                *mode = SEND;
                break;
            case 'r':
                *mode = READ;
                break;
            case 'f':
                *fileName = optarg;
                printf("%s\n", *fileName); 
                break;
            default:
                break;
        }
    }
}
