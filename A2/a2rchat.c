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
const char* out_fifos[] = {"fifo-1.out","fifo-2.out","fifo-3.out","fifo-4.out","fifo-5.out"};

int usage() {
  printf("usage: a2chat -s baseName [nclient]\n");
  return -1;
}

int server_read(char* cmd) {
  return 0;
}

int begin_server(char* name, int nclient) {
  printf("Chat server begins [nclient = %d]\n", nclient);
  char in_fifo[100];
  int fd;


  pid_t pid;
  pid = fork();
  if(pid == 0) {
    static char cmd[BUFMAX];

    // temp
    _Exit(EXIT_SUCCESS);

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
    while(1) {
      // poll inFIFOs
      int i, nread;

      /* fd = open(in_fifos[0], O_WRONLY); */
      /* write(fd, "Hi", sizeof("Hi")); */
      /* close(fd); */

      for(i=0;i<NMAX;i++) {
        char msg[BUFMAX];
        fd = open(in_fifos[i],O_RDONLY);
        if((nread = read(fd, msg, BUFMAX)) == -1)
          perror("server: read error\nError ");
        close(fd);
        printf("[server] read '%s' from '%s'\n", msg, in_fifos[i]);
        server_read(msg);
        msg[0] = '\0';
      }
      wait(NULL);
      return 0;
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
  printf("Chat client begins\n");
  static char cmd[BUFMAX];
  static char username[BUFMAX];
  char write_buf[BUFMAX];
  char read_buf[BUFMAX];
  int i;
  int fd[2];
  // client loop
  while(1) {
    printf("a2chat_client: ");
    scanf("%s", cmd);

    if(strcmp(cmd,"open") == 0) {
      scanf("%s", username);
      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        fd[0] = open(in_fifos[i], O_WRONLY);
        if(lockf(fd[0], F_TEST, 0) == 0) { //unlocked
          lockf(fd[0], F_LOCK, BUFMAX);
          printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifos[i], getpid());
          // tell server this client has locked the fifo
          strcat(write_buf, "LOCKED ");
          strcat(write_buf, username);
          write(fd[0], write_buf, sizeof(write_buf));
          // await server response
          fd[1] = open(out_fifos[i], O_RDONLY);
          read(fd[1], read_buf, sizeof(read_buf));
          // unlock and close file if server denies access
          break;
        }
        if(i == NMAX-1)
          printf("All FIFOs are currently in use. Please try again later.\n");
      }
    }

    if(strcmp(cmd,"done") == 0) {
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
