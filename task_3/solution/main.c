#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "fileTransfer/transfer.h"

static struct option long_options[] = {
    {"send",       no_argument,       0, 's'},
    {"read",       no_argument,       0, 'r'},
    {"file",       required_argument, 0, 'f'},
    {"chunk-size", required_argument, 0, 'b'},
    {0, 0, 0, 0}
};

enum Mode{
    SEND,
    READ
};

static int parseChunkSize(const char* text, size_t* chunkSize){
    assert(text);
    assert(chunkSize);

    if(*text == '\0') return -1;
    for(const char* digit = text; *digit != '\0'; digit++){
        if(*digit < '0' || *digit > '9') return -1;
    }

    char* end = NULL;
    errno = 0;
    uintmax_t value = strtoumax(text, &end, 10);
    if(errno == ERANGE || *end != '\0' || value == 0 || value > PTRDIFF_MAX){
        return -1;
    }

    *chunkSize = (size_t)value;
    return 0;
}

static int flagsHandle(int argc, char* argv[], const char** fileName,
                       enum Mode* mode, size_t* chunkSize){
    assert(argv);
    assert(fileName);
    assert(mode);
    assert(chunkSize);

    int opt = 0;
    int option_index = 0;
    while((opt = getopt_long(argc, argv, "srf:b:", long_options, &option_index)) != -1){
        switch(opt){
            case 's':
                *mode = SEND;
                break;
            case 'r':
                *mode = READ;
                break;
            case 'f':
                *fileName = optarg;
                break;
            case 'b':
                if(parseChunkSize(optarg, chunkSize) == -1){
                    fprintf(stderr, "Invalid chunk size: %s (expected positive bytes)\n", optarg);
                    return -1;
                }
                break;
            default:
                return -1;
        }
    }

    if(optind != argc){
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[]){
    const char* fileName = "input.bin";
    enum Mode mode = SEND;
    size_t chunkSize = 4096;

    if(flagsHandle(argc, argv, &fileName, &mode, &chunkSize) == -1){
        fprintf(stderr, "Usage: %s --send|--read --file PATH [--chunk-size BYTES]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int result = mode == SEND ? send(fileName, chunkSize) : receive(fileName, chunkSize);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
