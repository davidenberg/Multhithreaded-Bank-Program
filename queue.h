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

/**
 * @brief Create a Queue struct
 *
 * @return struct Queue
 */
struct Queue createQueue();

/**
 * @brief return first element of queue
 *
 * @param queue
 * @return char*
 */
char* peek(struct Queue queue);

/**
 * @brief Checks if given queue struct is empty
 *
 * @param queue
 * @return true if empty
 * @return false if not empty
 */
bool isEmpty(struct Queue queue);

/**
 * @brief Check if given queue struct is full
 *
 * @param queue
 * @return true if full
 * @return false if not full
 */
bool isFull(struct Queue queue);

/**
 * @brief Insert string into given queue struct
 *
 * @param queue
 * @param c
 * @return true if insertion was successful
 * @return false otherwise
 */

bool insert(struct Queue* queue, char* c);

/**
 * @brief Pops first element from queue
 *
 * @param queue
 * @return char* pointer to the removed element, if unsuccessful return NULL
 */
char* removeData(struct Queue* queue);

#endif  // __QUEUE_H__