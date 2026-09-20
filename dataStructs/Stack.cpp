#include "Stack.h"

bool isEmpty(Stack *stack) { return (stack->top == -1); }

bool isFull(Stack *stack) { return (stack->top >= MAX - 1); }

void push(Stack *stack, void *value) { stack->items[stack->top++] = value; }

void *pop(Stack *stack) {
  void *value = stack->items[stack->top];
  stack->top--;
  return value;
}

void *peek(Stack *stack) { return stack->items[stack->top]; }
