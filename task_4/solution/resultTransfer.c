#include "resultTransfer.h"
#include "ipc/shMem.h"

#include <stdio.h>

static key_t resultKey(void)
{
    FILE* file = fopen(RESULT_SHMEM_PATH, "ab");
    if(file == NULL){
        perror("shared memory key file");
        return (key_t)-1;
    }
    if(fclose(file) != 0){
        perror("key file close");
        return (key_t)-1;
    }
    key_t key = ftok(RESULT_SHMEM_PATH, 'S');
    if(key == (key_t)-1) perror("ftok");
    return key;
}

int sendResult(const char* expression, const double borders[2],
               const double result[2], size_t chunkSize){
    char string[RESULT_STRING_SIZE];
    int length = snprintf(string, sizeof(string),"integral of %s in [%lf, %lf] = %lf +- %lf\n", expression, 
                                                                                                borders[0], borders[1], 
                                                                                                result[0], result[1]);
    if(length < 0 || (size_t)length >= sizeof(string)){
        fprintf(stderr, "Result string is too long\n");
        return -1;
    }

    key_t key = resultKey();
    if(key == (key_t)-1) return -1;
    fprintf(stderr, "Sending result; waiting for receiver...\n");

    int status = shMemSendString(key, string, chunkSize);
    if(status == -1) perror("send result");
    
    return status;
}

int receiveResult(size_t chunkSize)
{
    key_t key = resultKey();
    if(key == (key_t)-1) return -1;

    char string[RESULT_STRING_SIZE];
    if(shMemReadString(key, string, sizeof(string), chunkSize) == -1){
        perror("receive result");
        return -1;
    }

    if(fputs(string, stdout) == EOF || fflush(stdout) == EOF){
        perror("print result");
        return -1;
    }

    return 0;
}
