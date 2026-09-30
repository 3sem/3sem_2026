#ifndef PROCESS_H
#define PROCESS_H

typedef struct process_t {
    int fdIn;
    int fdOut;
} process_t;

process_t* processCtor ();

void processDtor (process_t* process);

int connectProcesses(process_t *first, process_t *second);

ssize_t processRead(process_t* process, char* buf, size_t size);

int processWrite(process_t* process, const char* buf, size_t size);

void processFinishWrite(process_t* process);

int processExchange(process_t* process, char* buf, size_t size, int lastBlock);

#endif
