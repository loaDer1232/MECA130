#ifndef STACK_H
#define STACK_H

#define MAX 256

typedef struct {
  void *items[MAX];
  int top = -1;
  int size = 0;
} Stack;

bool isFull(Stack *stack);
bool isEmpty(Stack *stack);
void push(Stack *stack, void *value);
void *pop(Stack *stack);
void *peek(Stack *stack);

#endif
