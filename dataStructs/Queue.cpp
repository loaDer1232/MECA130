#include "Queue.h"

bool isFull(Queue *queue) { return (queue->size == MAX); }
bool isEmpty(Queue *queue) { return (queue->size == 0); }

void enqueue(Queue *queue, void *value) {
  queue->back = (queue->back + 1) % MAX;
  queue->items[queue->back] = value;
  queue->size++;
}

void *dequeue(Queue *queue) {
  void *value = queue->items[queue->front];
  queue->front = (queue->front + 1) % MAX;
  queue->size--;
  return value;
}

void *peek(Queue *queue) { return queue->items[queue->front]; }

void *seeLast(Queue *queue) { return queue->items[queue->back]; }
