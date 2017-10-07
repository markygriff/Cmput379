#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>

#define NMAX 5
#define BUFMAX 1024

//temp
const char* in_fifos[] = {"fifo-1.in","fifo-2.in","fifo-3.in","fifo-4.in","fifo-5.in"};
char* in_fifo_pids[] = {NULL, NULL, NULL, NULL, NULL};
const char* out_fifos[] = {"fifo-1.out","fifo-2.out","fifo-3.out","fifo-4.out","fifo-5.out"};

int usage() {
  printf("usage: a2chat -s baseName [nclient]\n");
  return -1;
}

int server_read(char* cmd) {
  return 0;
}

int read_fifo(const char* fifo, char* msg) {
  int fd, nread;
  fd = open(fifo, O_RDONLY);
  if((nread = read(fd, msg, sizeof(msg))) == -1)
    perror("Read Error");
  close(fd);
  return nread;
}

int write_fifo(const char* fifo, char* msg) {
  int fd, nwrote;
  fd = open(fifo, O_WRONLY);
  if((nwrote = write(fd, msg, sizeof(msg))) == -1)
    perror("Write Error");
  close(fd);
  return nwrote;
}

int begin_server(char* name, int nclient) {
  printf("Chat server begins [nclient = %d]\n", nclient);

  pid_t pid;
  pid = fork();
  if(pid == 0) { // stdin poll

    // temp
    _Exit(EXIT_SUCCESS);

    static char cmd[BUFMAX];
    // server loop
    while(1) {
      // poll stdin
      scanf("%s", cmd);
      if(strcmp(cmd, "exit") == 0) {
        printf("exiting server...\n");
        _Exit(EXIT_SUCCESS);
      }
    }
  }
  else {
    char msg[BUFMAX];
    int i, nread, nwrote;
    while(1) {
      nread = 0;
      nwrote = 0;
      // poll inFIFOs
      for(i=0;i<NMAX;i++) {
        memset(msg, 0, sizeof(msg));
        nread = read_fifo(in_fifos[i], msg);
        printf("server: [debug] read '%s' (%d bytes) from '%s'\n", msg, nread, in_fifos[i]);
        // if the message received is greater than 0 bytes...
        if(nread > 0) {
          // fifo doesn't belong to another client
          if(in_fifo_pids[i] == NULL) {
            in_fifo_pids[i] = msg;
          }
          // fifo belongs to a client
          else {
            memset(msg, 0, strlen(msg));
            nwrote = write_fifo(out_fifos[i], msg);
            printf("server: [debug] wrote %s (%d bytes) to outFIFO[%d]\n", msg, nwrote, i);
          }
        }
      }
      sleep(1);
      wait(NULL);
      /* return 0; */
    }
  }



    // process command
    // form a response (if needed)
    // send response to client

    // on success of client command, send client "[server] info"
    // on failure of client command, send client "[server] Error: info"

    // if open...
    // check if there are any unlocked inFIFO's available
    // check if client has a session already
    // check if the server has reached its client limit (nclient)
    // check if the username for the client is already being used
    // make an inFIFO
    // printf("[server] User '%s" connected to FIFO %d\n", users[i], i);

    // if who...
    // printf("[server] Current users:")
    // for 0 -> sizeof(users)
    //   printf("[%d] %s,", i, users[i])

    // if < [chat_line]...
    // server sends "[username] chat_line" to all recipients of the user

    // if done...
    // close inFIFO of user
    // remove client username from users
    // send("[server] done")

    // if exit FROM inFIFO...
    // close inFIFO of user

    // if exit FROM STDIN
    // terminate server

  return 0;
}

int begin_client(char* name) {
  static char msg[BUFMAX];
  char username[BUFMAX] = {'\0'};
  int i, nread, nwrote;
  int fd[2];

  printf("Chat client begins\n");

  // client loop
  while(1) {
    memset(msg, 0, sizeof(msg));

    // check outFIFO for replies
    if(username[0] != '\0') {
      nread = read_fifo(out_fifos[i], msg);
      printf("client: [debug] read '%s' (%d bytes) from outFIFO[%d]\n", msg, nread, i);
    }

    printf("a2chat_client: ");
    scanf("%s", msg);

    // if the client has a username, it has an inFIFO to send msgs
    if(username[0] != '\0') {
      // send message
      printf("client: [debug] [username=%s] sending '%s' to inFIFO[%d]\n",\
        username, msg, i);
      nwrote = write_fifo(in_fifos[i], msg);
      // write server response to stdout
    }

    if(strcmp(msg,"open") == 0) {
      if(username[0] != '\0') {
        printf("This client already has the username '%s'\n", username);
        break;
      }
      scanf("%s", username);
      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        fd[0] = open(in_fifos[i], O_WRONLY);
        if(lockf(fd[0], F_TEST, 0) == 0) { //unlocked
          // lock the inFIFO
          lockf(fd[0], F_LOCK, BUFMAX);
          printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifos[i], getpid());
          // write into the inFIFO to notify the server
          memset(msg, 0, sizeof(msg));
          strcat(msg, username);
          printf("client: [debug] writing '%s' to inFIFO\n", msg);
          write(fd[0], msg, sizeof(msg));
          // await server response
          memset(msg, 0, strlen(msg));
          nread = read_fifo(out_fifos[i], msg);
          printf("client: [debug] read '%s' from outFIFO[%d]\n", msg, i);

          // TODO: unlock and close file if server denies access
          break;
        }
        if(i == NMAX-1)
          printf("All FIFOs are currently in use. Please try again later.\n");
      }
    }

    if(strcmp(msg,"done") == 0) {
      // unlock

      close(fd[0]);
    }
    /// send command to server

    // open [username]:
    //    locksFIFO -> printf("FIFO [%s-%d.in] has been locked by PID [%d]\n", name, i, pid)
    // get server response and display -> printf("%s\n", resp)

    // who
    // to [u1 u2 ...]
    // < [chat_line]
    // close
    // exit


    /// wait for the server response line
    /// process the response line


    /// if in connected state...
    /// poll from outFIFO
    /// process response line
  }
  return 0;
}

int main(int argc, const char* argv[]) {
  if(argc < 2 || argc > 4) { // at least 2 args, max 4 args
    return usage();
  }
  char* baseName = argv[2];
  if((strcmp(argv[1], "-s") == 0) && argc == 4) { // server
    // number of clients
    int nclient;
    if((isdigit(argv[3]) == -1) || (nclient = atoi(argv[3])) > NMAX) {
      printf("Error: invalid nclient\n");
      return usage();
    }
    else
      return begin_server(baseName, nclient);
  }
  else if((strcmp(argv[1], "-c") == 0) && argc == 3) // client
    return begin_client(baseName);
  else
    return usage();

  return 0;
}
