#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#include "queue.h"

#define BUFSIZE 255

#define QUEUESIZE 4

#define ACC_CAPACITY 1000

#define CHECK_ALLOC(ptr)     \
  if ((ptr) == NULL) {       \
    perror("Malloc failed"); \
    exit(EXIT_FAILURE);      \
  }

struct client {
  long int message_type;
  char mtext[100];
};

struct account {
  int accnumber;
  int balance;
  pthread_rwlock_t lock;
} account;

struct account_storage {
  int accnumber;
  int balance;
} account_storage;

struct for_thread {
  struct Queue* q;
  int pipe;
};

struct for_master {
  int pipe1, pipe2, pipe3, pipe4;
};

struct account** accounts;

int query = 0;
int cont = 0;
int shutdown = 0;
int desk1, desk2, desk3, desk4;

void sig_int(int signum) {
  printf("Caught signal SIGINT, shutting down\n");
  char buf[100];
  shutdown = 1;
  read(desk1, buf, 100);
  printf("Desk 1 shutdown\n");
  read(desk2, buf, 100);
  printf("Desk 2 shutdown\n");
  read(desk3, buf, 100);
  printf("Desk 3 shutdown\n");
  read(desk4, buf, 100);
  printf("Desk 4 shutdown\n");
}

void master_query(int* bal, int fd) {
  if (query) {
    char* out = calloc(sizeof(char), 100);
    CHECK_ALLOC(out);
    sprintf(out, "%d", *bal);
    write(fd, out, 100);
    free(out);
    while (cont == 0) usleep(1000);
  }
}

int interact_with_client(int input, int output, int* bal) {
  int quit = 0;
  char* buf = malloc(BUFSIZE);
  CHECK_ALLOC(buf);
  if (buf == NULL) {
    return -1;
  }
  while (!quit) {
    if (read(input, buf, BUFSIZE)) {
      switch (buf[0]) {
        case 'q':
          printf("processing command 'q'\n");
          quit = 1;
          printf("done with client\n");
          break;
        case 'l': {
          printf("processing command 'l'\n");
          int accno = -1;
          if (sscanf(buf, "l %d", &accno) == 1) {
            if (accno < ACC_CAPACITY && accno >= 0) {
              pthread_rwlock_rdlock(&accounts[accno]->lock);
              int balance = accounts[accno]->balance;
              char* out = calloc(sizeof(char), BUFSIZE);
              CHECK_ALLOC(out);
              sprintf(out, "%d", balance);
              write(output, out, BUFSIZE);
              free(out);
              pthread_rwlock_unlock(&accounts[accno]->lock);
            } else {
              write(output, "No account with that number in record", BUFSIZE);
            }
          } else
            write(output, "fail: Error in command\n", BUFSIZE);
          break;
        }
        case 'w': {
          printf("processing command 'w'\n");
          int accno = -1;
          int amount = 0;
          if (sscanf(buf, "w %d %d", &accno, &amount) == 2) {
            if (accno < ACC_CAPACITY && accno >= 0) {
              pthread_rwlock_wrlock(&accounts[accno]->lock);
              if (accounts[accno]->balance >= amount) {
                *bal -= amount;
                accounts[accno]->balance -= amount;
                char* out = calloc(sizeof(char), BUFSIZE);
                CHECK_ALLOC(out);
                sprintf(out,
                        "Withdrew %d from account %d, remaining balance %d",
                        amount, accno, accounts[accno]->balance);
                write(output, out, BUFSIZE);
                free(out);
              } else {
                char* out = calloc(sizeof(char), BUFSIZE);
                CHECK_ALLOC(out);
                sprintf(out,
                        "Current balance %d is not sufficient for withdrawal",
                        accounts[accno]->balance);
                write(output, out, BUFSIZE);
                free(out);
              }
              pthread_rwlock_unlock(&accounts[accno]->lock);
            } else {
              write(output, "No account with that number in record", BUFSIZE);
            }
          } else
            write(output, "fail: Error in command", BUFSIZE);
          break;
        }

        case 't': {
          int accno1 = -1;
          int accno2 = -1;
          int amount = 0;
          printf("processing command 't'\n");
          if (sscanf(buf, "t %d %d %d", &accno1, &accno2, &amount) == 3) {
            if (accno1 >= 0 && accno1 < ACC_CAPACITY && accno2 >= 0 &&
                accno2 < ACC_CAPACITY && (accno1 != accno2)) {
              pthread_rwlock_wrlock(&accounts[accno1]->lock);
              pthread_rwlock_wrlock(&accounts[accno2]->lock);
              if (accounts[accno1]->balance >= amount) {
                accounts[accno1]->balance -= amount;
                accounts[accno2]->balance += amount;
                char* out = calloc(sizeof(char), BUFSIZE);
                CHECK_ALLOC(out);
                sprintf(out, "Transferred %d from account %d to account %d",
                        amount, accno1, accno2);
                write(output, out, BUFSIZE);
                free(out);
              } else {
                char* out = calloc(sizeof(char), BUFSIZE);
                CHECK_ALLOC(out);
                sprintf(out,
                        "Current balance %d of account %d is not sufficient "
                        "for transfer",
                        accounts[accno1]->balance, accno1);
                write(output, out, BUFSIZE);
                free(out);
              }
              pthread_rwlock_unlock(&accounts[accno1]->lock);
              pthread_rwlock_unlock(&accounts[accno2]->lock);
            } else {
              write(output,
                    "No account with that number in record or the account "
                    "numbers belonged to the same account",
                    BUFSIZE);
            }
          } else {
            write(output, "fail: Error in command", BUFSIZE);
          }
          break;
        }

        case 'd': {
          printf("processing command 'd'\n");
          int accno = -1;
          int amount = 0;
          if (sscanf(buf, "d %d %d", &accno, &amount) == 2) {
            if (accno < ACC_CAPACITY && accno >= 0) {
              pthread_rwlock_wrlock(&accounts[accno]->lock);
              *bal += amount;
              accounts[accno]->balance += amount;
              char* out = calloc(sizeof(char), BUFSIZE);
              CHECK_ALLOC(out);
              sprintf(out, "Deposited %d to account %d, new balance %d", amount,
                      accno, accounts[accno]->balance);
              write(output, out, BUFSIZE);
              free(out);
              pthread_rwlock_unlock(&accounts[accno]->lock);

            } else {
              write(output, "No account with that number in record", BUFSIZE);
            }
          } else
            write(output, "fail: Error in command", BUFSIZE);
          break;
        }

        default:
          break;
      }
    }
  }
  free(buf);
  return 0;
}

int establish_client_conn(const char* path, int* bal) {
  char* path_in;
  char* path_out;
  int output, input;
  char temp_path[100];
  strcpy(temp_path, path);

  const char delim[2] = "|";
  path_out = strtok(temp_path, delim);
  path_in = strtok(NULL, delim);
  output = open(path_out, O_WRONLY);
  input = open(path_in, O_RDONLY);

  if (output < 0 || input < 0) {
    goto err_exit;
  }

  if (write(output, "ready\n", 8)) {
    if (interact_with_client(input, output, bal) < 0) {
      goto err_exit;
    }
  } else {
    printf("error in writing to client \n");
    goto err_exit;
  }
  close(output);
  close(input);
  return 0;

err_exit:
  printf("at err exit\n");
  close(output);
  close(input);
  return -1;
}

void* init_thread(void* vargp) {
  struct for_thread* my_inf = (struct for_thread*)vargp;
  struct Queue* queue = my_inf->q;
  int my_bal = 0;
  int* bal_pointer = &my_bal;
  int fd = my_inf->pipe;
  char* paths;

  for (;;) {
    if ((paths = removeData(queue)) != NULL) {
      printf("got path %s from queue, queue.front is %d\n", paths,
             queue->front);
      establish_client_conn(paths, bal_pointer);
    }
    master_query(bal_pointer, fd);
    if (shutdown) {
      write(fd, "Shutdown", BUFSIZE);
      break;
    }
    // printf("no client in queue\n");
    sleep(1);
  }
  pthread_detach(pthread_self());
  return (void*)0;
}

void enqueue(char* path, struct Queue** queues) {
  int shortest = 100;
  int shortest_idx = 0;
  for (int i = 0; i < QUEUESIZE; i++) {
    if ((queues[i])->size < shortest) {
      shortest = (queues[i])->size;
      shortest_idx = i;
    }
  }
  printf("inserting path into queue %d\n", shortest_idx);
  if (!insert(queues[shortest_idx], path)) {
    printf("could not insert path\n");
  }
}

void cleanup(void* arg) { free((char*)arg); }

void* master_thread(void* vargp) {
  char buf1[100], buf2[100], buf3[100], buf4[100];
  char* buf = calloc(sizeof(char), BUFSIZE);
  CHECK_ALLOC(buf);
  pthread_cleanup_push(cleanup, (void*)buf);

  if (vargp) {
    struct for_master* fm = (struct for_master*)vargp;
    desk1 = fm->pipe1;
    desk2 = fm->pipe2;
    desk3 = fm->pipe3;
    desk4 = fm->pipe4;
    for (;;) {
      if (fgets(buf, BUFSIZE, stdin) == NULL) break;
      switch (buf[0]) {
        case 'l': {
          cont = 0;
          query = 1;
          read(fm->pipe1, buf1, 100);
          read(fm->pipe2, buf2, 100);
          read(fm->pipe3, buf3, 100);
          read(fm->pipe4, buf4, 100);

          printf("Balances\nDesk 1: %s\nDesk 2: %s\nDesk 3: %s\nDesk 4: %s\n",
                 buf1, buf2, buf3, buf4);

          query = 0;
          cont = 1;
        }
        default:
          break;
      }
    }
  }
  pthread_cleanup_pop(0);
  free(buf);
  pthread_detach(pthread_self());
  return (void*)0;
}
int main(int argc, char** argv) {
  const char* fname = "runfile";
  const char* acc_file = "accounts";
  FILE* accfile;
  FILE* runfile = fopen(fname, "w");
  char* buf = malloc(BUFSIZE);
  CHECK_ALLOC(buf);

  pid_t pid = getpid();
  struct account_storage* acc_input = malloc(sizeof(struct account));

  int pipe1[2], pipe2[2], pipe3[3], pipe4[4];

  struct client recv_client;
  struct Queue q1 = createQueue(1);
  struct Queue q2 = createQueue(2);
  struct Queue q3 = createQueue(3);
  struct Queue q4 = createQueue(4);

  struct Queue** queues = malloc(sizeof(struct Queue*) * QUEUESIZE);
  CHECK_ALLOC(queues);

  accounts = malloc(sizeof(struct account*) * ACC_CAPACITY);
  CHECK_ALLOC(accounts);

  for (int i = 0; i < ACC_CAPACITY; i++) {
    accounts[i] = malloc(sizeof(struct account));
    CHECK_ALLOC(accounts[i]);
    accounts[i]->accnumber = i;
    accounts[i]->balance = 0;
    pthread_rwlock_init(&accounts[i]->lock, NULL);
  }

  if ((accfile = fopen(acc_file, "r")) != NULL) {
    for (int i = 0; i < ACC_CAPACITY; i++) {
      fread(acc_input, sizeof(struct account_storage), 1, accfile);
      accounts[i]->accnumber = acc_input->accnumber;
      accounts[i]->balance = acc_input->balance;
    }
    fclose(accfile);
  } else
    printf("Could not open storage of accounts\n");

  free(acc_input);

  queues[0] = &q1;
  queues[1] = &q2;
  queues[2] = &q3;
  queues[3] = &q4;

  pthread_t tid1, tid2, tid3, tid4, mtid;

  key_t key;
  int msgid;

  key = ftok("progfile", 65);

  msgid = msgget(key, 0666 | IPC_CREAT);

  fprintf(runfile, "%d\n", msgid);
  fprintf(runfile, "%d\n", pid);

  fclose(runfile);

  pipe(pipe1);
  pipe(pipe2);
  pipe(pipe3);
  pipe(pipe4);

  struct for_thread ft1 = {queues[0], pipe1[1]};
  struct for_thread ft2 = {queues[1], pipe2[1]};
  struct for_thread ft3 = {queues[2], pipe3[1]};
  struct for_thread ft4 = {queues[3], pipe4[1]};

  struct for_master fm = {pipe1[0], pipe2[0], pipe3[0], pipe4[0]};

  pthread_create(&mtid, NULL, master_thread, (void*)&fm);

  pthread_create(&tid1, NULL, init_thread, (void*)(&ft1));
  pthread_create(&tid2, NULL, init_thread, (void*)(&ft2));
  pthread_create(&tid3, NULL, init_thread, (void*)(&ft3));
  pthread_create(&tid4, NULL, init_thread, (void*)(&ft4));

  signal(SIGINT, sig_int);

  for (;;) {
    if (msgrcv(msgid, &recv_client, sizeof(recv_client.mtext), 1, IPC_NOWAIT) ==
        -1) {
      if (errno != ENOMSG) {
        printf("something went wrong when receiving message\n");
      }
    } else {
      strcpy(buf, recv_client.mtext);
      enqueue(buf, queues);
      errno = 0;
    }
    if (shutdown) {
      pthread_cancel(mtid);
      pthread_detach(mtid);
      break;
    }
    usleep(1000);
  }
  free(buf);
  struct account_storage** acc_storage;
  acc_storage = malloc(sizeof(struct account_storage) * ACC_CAPACITY);
  CHECK_ALLOC(acc_storage);

  for (int i = 0; i < ACC_CAPACITY; i++) {
    acc_storage[i] = malloc(sizeof(struct account_storage));
    CHECK_ALLOC(acc_storage[i]);
  }
  for (int i = 0; i < ACC_CAPACITY; i++) {
    acc_storage[i]->accnumber = accounts[i]->accnumber;
    acc_storage[i]->balance = accounts[i]->balance;
  }

  accfile = fopen(acc_file, "w");
  if (accfile == NULL) printf("could not open accfile\n");
  for (int i = 0; i < ACC_CAPACITY; i++) {
    if (fwrite(acc_storage[i], sizeof(struct account_storage), 1, accfile) ==
        0) {
      printf("could not write to file\n");
    }
    pthread_rwlock_destroy(&accounts[i]->lock);
    free(accounts[i]);
  }
  fclose(accfile);

  free(accounts);

  for (int i = 0; i < ACC_CAPACITY; i++) {
    free(acc_storage[i]);
  }
  free(acc_storage);

  for (int i = 0; i < QUEUESIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      free(queues[i]->q[j]);
    }
    free(queues[i]->q);
  }
  free(queues);
  remove(fname);
  return 0;
}