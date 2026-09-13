#include <stdio.h>
#include <unistd.h>

#include "fullDuplexPipe.h"

#include "linuxCmd.h"

int main(){

    struct fDupPipe_t* fDupPipe = fDupPipeCtor();    

    

    fDupPipeDtor(fDupPipe);
}

