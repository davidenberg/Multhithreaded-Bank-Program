#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFSIZE 255

struct client {
  long int message_type;
  char mtext[100];
};

int input, output;

/*Signal handler to tell the server we are exiting*/
void sig_int(int signum) {
  printf("\nCaught signal SIGINT, disconnecting...\n");
  write(output, "q", BUFSIZE);
  printf("Disconnected\n");
  exit(0);
}
/**
 * @brief Connects to server by passing 2 named pipes via a message queue
 *
 * @param fname file that contains msgqid
 * @param input the variable which will store our input side of the named pipe
 * @param output the variable which will store our output side of the named pip
 * @return int
 */
int connect_to_server(const char* fname, int* input, int* output) {
  FILE* runfile;
  int msgqid = -1;
  struct client c;

  pid_t pid = getpid();
  char path_in[50];
  char path_out[50];
  char to_child[100];
  char* buf = malloc(BUFSIZE);
  /*Use pid to guarantee unique name for named pipe*/
  sprintf(path_in, "/tmp/fifoin%d", pid);
  sprintf(path_out, "/tmp/fifoout%d", pid);
  mkfifo(path_in, 0666);
  mkfifo(path_out, 0666);

  strcat(to_child, path_in);
  strcat(to_child, "|");
  strcat(to_child, path_out);
  strcpy(c.mtext, to_child);
  c.message_type = 1;

  sleep(1);

  runfile = fopen(fname, "r");
  fscanf(runfile, "%d", &msgqid);
  fclose(runfile);

  /*Send the named pipes to server*/
  if (msgsnd(msgqid, &c, sizeof(c.mtext), 0) < 0) {
    free(buf);
    return -1;
  }

  *input = open(path_in, O_RDONLY);
  *output = open(path_out, O_WRONLY);
  if (*input < 0 || *output < 0) return -1;
  /*Block until we get write from server*/
  int n = read(*input, buf, 49);
  if (n >= 0 && !strcmp(buf, "ready\n")) {
    free(buf);
    return msgqid;
  }
  free(buf);
  return -1;
}

int main(int argc, char** argv) {
  setvbuf(stdin, NULL, _IOLBF, 0);
  setvbuf(stdout, NULL, _IOLBF, 0);
  /*This file with fname is an indication if the
  server is running, also contains the msgqueueid
  that is use to pass the named pipe that will
  be used for ipc with the server*/
  const char* fname = "runfile";
  int msgqid;

  if (access(fname, F_OK) == 0) {
    char* buf = calloc(sizeof(char), BUFSIZE);
    int quit = 0;

    if ((msgqid = connect_to_server(fname, &input, &output)) >= 0) {
      /*Connect to server uses blocking function call read() which means
      that we won't enter before server is ready to communicate*/
      printf("ready\n");
      /*Init signal handler*/
      signal(SIGINT, sig_int);
      while (!quit) {
        /*Parse commands and communicate with server*/
        if (fgets(buf, BUFSIZE, stdin) == NULL) break;
        switch (buf[0]) {
          case 'q':
            quit = 1;
            write(output, "q", BUFSIZE);
            break;
          case 'l': {
            int accno = -1;
            if (sscanf(buf, "l %d", &accno) == 1) {
              write(output, buf, BUFSIZE);
              read(input, buf, BUFSIZE);
              printf("%s\n", buf);
            } else
              printf("fail: Error in command\n");
            break;
          }
          case 'w': {
            int accno = -1;
            int amount = 0;
            if ((sscanf(buf, "w %d %d", &accno, &amount)) == 2) {
              write(output, buf, BUFSIZE);
              read(input, buf, BUFSIZE);
              printf("%s\n", buf);
            } else
              printf("fail: Error in command\n");
            break;
          }
          case 'd': {
            int accno = -1;
            int amount = 0;
            if ((sscanf(buf, "d %d %d", &accno, &amount)) == 2) {
              write(output, buf, BUFSIZE);
              read(input, buf, BUFSIZE);
              printf("%s\n", buf);
            } else
              printf("fail: Error in command\n");
            break;
          }
          case 't': {
            int accno1 = -1;
            int accno2 = -1;
            int amount = 0;
            if ((sscanf(buf, "t %d %d %d", &accno1, &accno2, &amount)) == 3) {
              write(output, buf, BUFSIZE);
              read(input, buf, BUFSIZE);
              printf("%s\n", buf);
            } else
              printf("fail: Error in command\n");
            break;
          }
          default:
            printf("fail: Unknown command\n");
            break;
        }
      }
    }
    free(buf);
    close(input);
    close(output);
  } else {
    /*The runfile does not exists -> server is not running*/
    printf("server is not running\n");
  }
  return 0;
}