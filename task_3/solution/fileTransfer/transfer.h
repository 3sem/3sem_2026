#ifndef FIFO_TRANSFER_H
#define FIFO_TRANSFER_H

void send(const char* fifoName, const char* inputFileName); 
void receive(const char* fifoName, const char* outputFileName);

#endif /* FIFO_TRANSFER_H */
