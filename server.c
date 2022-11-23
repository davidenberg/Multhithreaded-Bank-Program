#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define BUFSIZE 255

struct client {
  long int message_type;
  char mtext[100];
};

int main(int argc, char** argv) {
  const char* fname = "runfile";
  FILE* runfile = fopen(fname, "w");
  pid_t pid = getpid();
  struct client recv_client;
  int output, input;
  char* path_in;
  char* path_out;
  char* temp_path = malloc(100);
  char* buf = malloc(BUFSIZE);

  key_t key;
  int msgid;

  key = ftok("progfile", 65);

  msgid = msgget(key, 0666 | IPC_CREAT);

  fprintf(runfile, "%d\n", msgid);
  fprintf(runfile, "%d\n", pid);

  fclose(runfile);

  if (msgrcv(msgid, &recv_client, sizeof(recv_client.mtext), 1, 0) == -1) {
    printf("something went wrong when receiving message\n");
    exit(1);
  }
  printf("received path %s\n", recv_client.mtext);
  strcpy(temp_path, recv_client.mtext);
  const char delim[2] = "|";
  path_out = strtok(temp_path, delim);
  path_in = strtok(NULL, delim);
  output = open(path_out, O_WRONLY);
  input = open(path_in, O_RDONLY);
  if (output < 0) {
    printf("the value of errno is %s\n", strerror(errno));
  }
  int n = write(output, "ready\n", 8);
  close(output);
  printf("wrote %d bytes to pipe\n", n);
  if (n < 0) {
    printf("the value of errno is %s\n", strerror(errno));
  }
  n = read(input, buf, 255);
  if (n >= 0) printf("got message %s\n", buf);
  return 0;
}