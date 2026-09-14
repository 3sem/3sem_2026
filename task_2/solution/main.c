#include <stdio.h>

#include "echoTest.h"

int main(int argc, char* argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return 1;
    }

    return runEchoTest(argv[1], argv[2]);
}
