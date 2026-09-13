#include <malloc.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "fullDuplexPipe.h"

#include "linuxCmd.h"

struct fDupPipe_t* fDupPipeCtor(){
    fDupPipe_t* fDupPipe = (fDupPipe_t*) calloc(1, sizeof(fDupPipe_t));
    if (!fDupPipe) return NULL;

    if (createPipe(fDupPipe->pipeDown) != 0) {
        free(fDupPipe);
        return NULL;
    }
    if (createPipe(fDupPipe->pipeUp) != 0) {
        close(fDupPipe->pipeDown[0]);
        close(fDupPipe->pipeDown[1]);
        free(fDupPipe);
        return NULL;
    }
    
    fDupPipe->operations.fork      = fDupPipeFork;

    fDupPipe->operations.readDown  = fDupPipeReadDown;
    fDupPipe->operations.readUp    = fDupPipeReadUp;

    fDupPipe->operations.writeDown = fDupPipeWriteDown;
    fDupPipe->operations.writeUp   = fDupPipeWriteUp;
    
    fDupPipe->isForked             = false;
    fDupPipe->childPid             = 0;

    return fDupPipe;
}

void fDupPipeDtor(struct fDupPipe_t* fDupPipe){
    if(fDupPipe->isForked){
        if(fDupPipe->childPid == 0){
            close(fDupPipe->pipeDown[0]);
            close(fDupPipe->pipeUp[1]);
        }
        else if(fDupPipe->childPid > 0){
            close(fDupPipe->pipeDown[1]);
            close(fDupPipe->pipeUp[0]);

            while(waitpid(fDupPipe->childPid, NULL, 0) < 0 && errno == EINTR) {}
        }
    }
    else{
        close(fDupPipe->pipeDown[0]);
        close(fDupPipe->pipeDown[1]);

        close(fDupPipe->pipeUp[0]);
        close(fDupPipe->pipeUp[1]);
    }

    free(fDupPipe);
    fDupPipe = NULL;
}

pid_t fDupPipeFork(struct fDupPipe_t* fDupPipe){
    assert(fDupPipe);

    pid_t pid = forkProc();

    if(pid == 0){
        close(fDupPipe->pipeDown[1]);
        close(fDupPipe->pipeUp[0]);
    }
    else if(pid > 0){
        close(fDupPipe->pipeDown[0]);
        close(fDupPipe->pipeUp[1]);

    }
    
    fDupPipe->childPid = pid;
    fDupPipe->isForked = true;


    return pid; 
}

ssize_t fDupPipeWriteDown(struct fDupPipe_t* fDupPipe, const void* buf, size_t count){
    assert(fDupPipe);
    assert(buf);

    return write(fDupPipe->pipeDown[1], buf, count);
}

ssize_t fDupPipeWriteUp(struct fDupPipe_t* fDupPipe, const void* buf, size_t count){
    assert(fDupPipe);
    assert(buf);

    return write(fDupPipe->pipeUp[1], buf, count);
}

ssize_t fDupPipeReadDown(struct fDupPipe_t* fDupPipe, void* buf, size_t count){
    assert(fDupPipe);
    assert(buf);

    return read(fDupPipe->pipeDown[0], buf, count);
}

ssize_t fDupPipeReadUp(struct fDupPipe_t* fDupPipe, void* buf, size_t count){
    assert(fDupPipe);
    assert(buf);

    return read(fDupPipe->pipeUp[0], buf, count);
}
