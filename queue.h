#ifndef __QUEUE_H__
#define __QUEUE_H__

/**
 * @file queue.h
 * @author David Enberg david.enberg@aalto.fi
 * @brief Quick implementation of a queue structure
 * @version 0.1
 * @date 2022-11-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 100

typedef struct Queue {
  int size;
  int front;
  int rear;
  char** q;
} Queue;

struct Queue createQueue(int number);

char* peek(struct Queue queue);

bool isEmpty(struct Queue queue);

bool isFull(struct Queue queue);

bool insert(struct Queue* queue, char* c);

char* removeData(struct Queue* queue);

#endif  // __QUEUE_H__