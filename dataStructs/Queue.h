#ifndef QUEUE_H
#define QUEUE_H

#define MAX 256

typedef struct {
  void *items[MAX];
  int front = 0;
  int back = -1;
  int size = 0;
} Queue;

bool isFull(Queue *queue);
bool isEmpty(Queue *queue);
void enqueue(Queue *queue, void *value); // appends value to end of queue
void *
dequeue(Queue *queue);    // returns first value and removes it from the queue
void *peek(Queue *queue); // return first value but does not remove it
void *seeLast(Queue *queue); // returns last value in queue; does not remove it
#endif
