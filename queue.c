#include "queue.h"

struct Queue createQueue() {
  struct Queue queue;
  queue.q = (char**)malloc(SIZE * sizeof(char*));
  for (int i = 0; i < SIZE; i++) {
    queue.q[i] = malloc(100);
  }
  queue.size = 0;
  queue.front = 0;
  queue.rear = -1;
  queue.serving_cust = false;
  return queue;
}

char* peek(struct Queue queue) { return queue.q[queue.front]; }

bool isEmpty(struct Queue queue) { return queue.size == 0; }

bool isFull(struct Queue queue) { return queue.size == SIZE; }

bool insert(struct Queue* queue, char* c) {
  if (!isFull(*queue)) {
    if (queue->rear == SIZE - 1) {
      queue->rear = -1;
    }
    strcpy(queue->q[++queue->rear], c);
    queue->size++;
    return true;
  }
  return false;
}

char* removeData(struct Queue* queue) {
  if (queue->size > 0) {
    char* c = queue->q[queue->front++];
    if (queue->front == SIZE) {
      queue->front = 0;
    }

    queue->size--;
    queue->serving_cust = true;
    return c;
  }
  queue->serving_cust = false;
  return NULL;
}