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
/* const char* in_fifos[] = {"fifo-1.in","fifo-2.in","fifo-3.in","fifo-4.in","fifo-5.in"}; */
const char* in_fifos[] = {"fifo-1.in",NULL,"fifo-2.in",NULL,"fifo-3.in",NULL,"fifo-4.in",NULL,"fifo-5.in"};
const char* out_fifos[] = {"fifo-1.out","fifo-2.out","fifo-3.out","fifo-4.out","fifo-5.out"};

int usage() {
  printf("usage: a2chat -s baseName [nclient]\n");
  return -1;
}

// Helper function that optimizes string concatenation
/* char* mystrcat( char* dest, char* src ) { */
/*      while(*dest) dest++; */
/*      while(*dest++ = *src++); */
/*      return --dest; */
/* } */

int create_fifos(const char* name, int num) {
  int i, err;
  char new_name[strlen(name) + 6];
  for(i=1;i<=num;i++) {
    sprintf(new_name, "%s-%d.in", name, i);
    mkfifo(new_name, S_IRWXU);
    memset(new_name, 0, strlen(new_name));
    sprintf(new_name, "%s-%d.out", name, i);
    mkfifo(new_name, S_IRWXU);
    memset(new_name, 0, strlen(new_name));
  }
  err = 0;
  return err;
}

int read_fifo(const char* fifo, char* msg, size_t size) {
  int fd, nread;
  fd = open(fifo, O_RDONLY);
  if((nread = read(fd, msg, size)) == -1)
    perror("Read Error");
  close(fd);
  printf("[debug] read_fifo: returning %d\n", nread);
  return nread;
}

int write_fifo(const char* fifo, char* msg, size_t size) {
  int fd, nwrote;
  fd = open(fifo, O_WRONLY);
  if((nwrote = write(fd, msg, size)) == -1)
    perror("Write Error");
  close(fd);
  printf("[debug] write_fifo: returning %d\n", nwrote);
  return nwrote;
}

int begin_server(const char* name, int nclient) {
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
      for(i=0;i<(2*NMAX);i+=2) {
        memset(msg, 0, sizeof(msg));
        printf("[debug] server: reading from '%s'\n", in_fifos[i]);
        // if the message received is greater than 0 bytes...
        if ((nread = read_fifo(in_fifos[i], msg, sizeof(msg))) > 0) {
          printf("[debug] server: read '%s' (%d bytes) from '%s'\n", msg, nread, in_fifos[i]);
            // fifo doesn't belong to another client
            if(in_fifos[i+1] == NULL) {
              in_fifos[i+1] = strdup(msg);
              char resp[BUFMAX];
              // write response
              sprintf(resp, "[server] User '%s' has been successfully connected to FIFO %d\n", in_fifos[i+1], i+1);
              memset(msg, 0, sizeof(msg));
              nwrote = write_fifo(out_fifos[i], resp, sizeof(resp));
              printf("[debug] server: wrote '%s' (%d bytes) to outFIFO[%d]\n", resp, nwrote, i);
            }
            // fifo belongs to a client
            else {
              // NOTE: we are just echoing the msg back to the client for now
              // TODO: echo msg to recipients of the session
              memset(msg, 0, strlen(msg));
              nwrote = write_fifo(out_fifos[i], msg, sizeof(msg));
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

int client_poll(int fifo_n) {

  // TODO: pass in username?

  static char msg[BUFMAX];
  int nread, nwrote, status;
  pid_t pid = fork();
  if(pid == 0) { // outFIFO poll
    while((strcmp(msg, "done") != 0) || (strcmp(msg, "exit") != 0) ) {
      memset(msg, 0, sizeof(msg));
      printf("a2chat_client: ");
      scanf("%s", msg);
      if(strcmp(msg, "done") == 0) { //terminate chat session
        break;
      }
      else if(strcmp(msg, "exit") == 0) { // terminate chat session and client process
        // TODO: figure out how to terminate client from here
        break;
      }
      else {
        nwrote = write_fifo(in_fifos[fifo_n], msg, sizeof(msg));
        printf("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, fifo_n);
      }
    }
    _Exit(EXIT_SUCCESS);
  }
  else { // inFIFO writing
    while(1) {
      memset(msg, 0, sizeof(msg));
      printf("[debug] client: reading from outFIFO[%d]\n", fifo_n);
      nread = read_fifo(out_fifos[fifo_n], msg, sizeof(msg));
      printf("[debug] client: read '%s' (%d bytes) from outFIFO[%d]\n", msg, nread, fifo_n);
      if(waitpid(pid, &status, WNOHANG) == pid) break;
    }
  // TODO: delete username from in_fifos?
  return 0;
  }
}

int begin_client(const char* name) {
  static char msg[BUFMAX];
  char username[BUFMAX] = {'\0'};
  int i, fd, nread, nwrote;

  printf("Chat client begins\n");

  // client loop
  while(1) {
    memset(msg, 0, sizeof(msg));

    printf("a2chat_client: ");
    scanf("%s", msg);

    if(strcmp(msg,"open") == 0) {
      if(username[0] != '\0') {
        printf("This client already has the username '%s'\n", username);
        break;
      }
      scanf("%s", username);
      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        fd = open(in_fifos[i], O_WRONLY);
        if(lockf(fd, F_TEST, 0) == 0) { //unlocked
          // lock the inFIFO
          lockf(fd, F_LOCK, BUFMAX);
          printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifos[i], getpid());
          // write into the inFIFO to notify the server
          memset(msg, 0, sizeof(msg));
          strcat(msg, username);
          printf("client: [debug] writing '%s' to inFIFO[%d] to notify server\n", msg, i);
          nwrote = write_fifo(in_fifos[i], msg, sizeof(msg));
          printf("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, i);
          // await server response
          memset(msg, 0, strlen(msg));
          nread = read_fifo(out_fifos[i], msg, sizeof(msg)); // this read is blocked
          printf("client: [debug] read '%s' (%d bytes) from outFIFO[%d]\n", msg, nread, i);
          // begin chat session
          client_poll(i);
          // only exits if chat session is to be closed
          lockf(fd, F_ULOCK, BUFMAX);
          printf("client: [debug] inFIFO[%s] has been unlocked\n", in_fifos[i]);
          close(fd);
          break;
        }
        if(i == NMAX-1)
          printf("All FIFOs are currently in use. Please try again later.\n");
      }
    }

    if(strcmp(msg,"exit") == 0) {
      // terminate
      printf("client: [debug] exiting...\n");
      _exit(EXIT_SUCCESS);
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
  if(argc < 2 || argc > 4) // at least 2 args, max 4 args
    return usage();
  // get baseName and create inFIFOs and outFIFOs
  const char* baseName = argv[2];
  create_fifos(baseName, 5);
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
