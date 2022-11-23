#include <errno.h>
#include <fcntl.h>
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

int main(int argc, char** argv) {
  const char* fname = "runfile";
  int msgid;
  FILE* runfile;

  struct client c;
  if (access(fname, F_OK) == 0) {
    int input, output;
    pid_t pid = getpid();
    char path_in[50];
    char path_out[50];
    char to_child[100];
    char* buf = malloc(BUFSIZE);
    sprintf(path_in, "/tmp/fifoin%d", pid);
    sprintf(path_out, "/tmp/fifoout%d", pid);
    mkfifo(path_in, 0666);
    mkfifo(path_out, 0666);

    strcat(to_child, path_in);
    strcat(to_child, " ");
    strcat(to_child, path_out);
    strcpy(c.mtext, to_child);
    c.message_type = 1;
    sleep(1);

    runfile = fopen(fname, "r");
    fscanf(runfile, "%d", &msgid);
    fclose(runfile);
    printf("got msgid %d\n", msgid);
    if (msgsnd(msgid, &c, sizeof(c.mtext), 0) < 0) {
      printf("erro in msgsnd() %s\n", strerror(errno));
    }
    printf("sent path %s\n", c.mtext);
    input = open(path_in, O_RDONLY);
    output = open(path_out, O_WRONLY);

    sleep(1);

    char buff[50];
    int n = read(input, buff, 49);
    if (strcmp(buff, "ready\n")) {
      printf("ready\n");
      int quit = 0;
      while (!quit) {
        if (fgets(buf, BUFSIZE, stdin) == NULL) break;
        switch (buf[0]) {
          case 'q':
            quit = 1;
            break;
          case 'l': {
            int accno = -1;
            if (sscanf(buf, "l %d", &accno) == 1) {
              write(output, buf, BUFSIZE);
            } else
              printf("fail: Error in command\n");
            break;
          }
          case 'w': {
            int accno = -1, amnt = 0;
            if (sscanf(buf, "w %d %d", &accno, &amnt) == 2) {
              printf("ok: did it\n");
            } else
              printf("fail: something went wrong\n");
            break;
          }
          case 't':
            printf("ok: just pretending it worked\n");
            break;
          case 'd':
            printf("ok: trust us, your money is safe\n");
            break;
          default:
            printf("fail: Unknown command\n");
            break;
        }
      }
    }

    close(input);

    printf("read %d bytes from pipe: %s\n", n, buff);
  } else {
    pid_t child = fork();
    if (child == 0) {
      execl("server", "server", NULL);
      return -1;  // couldn't start server
    } else {
      int input, output;
      pid_t pid = getpid();
      char path_in[50];
      char path_out[50];
      char to_child[100];
      char* buf = malloc(BUFSIZE);
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
      fscanf(runfile, "%d", &msgid);
      fclose(runfile);
      printf("got msgid %d\n", msgid);
      if (msgsnd(msgid, &c, sizeof(c.mtext), 0) < 0) {
        printf("erro in msgsnd() %s\n", strerror(errno));
      }
      printf("sent path %s\n", c.mtext);
      input = open(path_in, O_RDONLY);
      output = open(path_out, O_WRONLY);
      int status = 0;

      sleep(1);

      char buff[50];
      int n = read(input, buff, 49);

      if (!strcmp(buff, "ready\n")) {
        printf("ready\n");
        int quit = 0;
        while (!quit) {
          if (fgets(buf, BUFSIZE, stdin) == NULL) break;
          switch (buf[0]) {
            case 'q':
              quit = 1;
              break;
            case 'l': {
              int accno = -1;
              if (sscanf(buf, "l %d", &accno) == 1) {
                write(output, buf, BUFSIZE);
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

      close(input);

      waitpid(child, &status, 0);
      remove(fname);
    }
  }
  return 0;
}