#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 5

typedef struct {
    int buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} CircularBuffer;

void initializerBuffer(CircularBuffer *cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

int bufferFull(CircularBuffer *cb) {
    return cb->count == BUFFER_SIZE;
}

int bufferEmpty(CircularBuffer *cb) {
    return cb->count == 0;
}

void enqueue(CircularBuffer *cb, int value) {
    if(bufferFull(cb)) {
        printf("Buffer overflow\n");
        return;
    } 
    cb->buffer[cb->tail] = value;
    cb->tail = (cb->tail + 1) % BUFFER_SIZE;
    cb->count++;
    
}

int dequeue(CircularBuffer *cb) {
    int value = -1;
    if(bufferEmpty(cb)) {
        printf("Buffer underflow\n");
        return value;
    }
    value = cb->buffer[cb->head];
    cb->head = (cb->head + 1) % BUFFER_SIZE;
    cb->count--;
    return value;
}

void printBuffer(CircularBuffer *cb) {
    printf("Buffer contents: ");
    for(int i =0; i < cb->count; i++) {
        printf("%d ", cb->buffer[(cb->head + i) % BUFFER_SIZE]);
    }
    printf("\n");
}

int main() {
    CircularBuffer cb;
    initializerBuffer(&cb);

    enqueue(&cb, 1);
    enqueue(&cb, 2);
    enqueue(&cb, 7);
    enqueue(&cb, 1);
    enqueue(&cb, 2);
    enqueue(&cb, 7);
    

    printBuffer(&cb);

    dequeue(&cb);
    printBuffer(&cb);
    dequeue(&cb);
    printBuffer(&cb);
    dequeue(&cb);
    dequeue(&cb);

}