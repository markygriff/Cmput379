#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>

typedef int bool;
#define false 0
#define true 1

#define NMAX 5
#define BUFMAX 1024
#define ARGSMAX 8
#define INFIFO "inFIFO"
#define OUTFIFO "outFIFO"

//temp
/* const char* in_fifos[] = {"fifo-1.in","fifo-2.in","fifo-3.in","fifo-4.in","fifo-5.in"}; */
/* const char* in_fifos[] = {"fifo-1.in",NULL,"fifo-2.in",NULL,"fifo-3.in",NULL,"fifo-4.in",NULL,"fifo-5.in"}; */
/* const char* in_fifos[] = {"fifo-1.in","fifo-2.in","fifo-3.in","fifo-4.in","fifo-5.in"}; */
/* const char* out_fifos[] = {"fifo-1.out","fifo-2.out","fifo-3.out","fifo-4.out","fifo-5.out"}; */


const char* in_fifos[] = {"fifo-1.in","fifo-2.in","fifo-3.in","fifo-4.in","fifo-5.in"};
const char* out_fifos[] = {"fifo-1.out","fifo-2.out","fifo-3.out","fifo-4.out","fifo-5.out"};

int usage() {
  printf("usage: a2chat [-s baseName nclient]\n");
  printf("       a2chat [-c baseName]\n");
  return -1;
}

// Helper function that optimizes string concatenation
/* char* mystrcat( char* dest, char* src ) { */
/*      while(*dest) dest++; */
/*      while(*dest++ = *src++); */
/*      return --dest; */
/* } */

void create_fifos(const char* name, int num) {
  int i, ret;
  char new_name[strlen(name) + 6];

  // TODO: create FIFO name array during creation of FIFOs

  for(i=1;i<=num;i++) {
    sprintf(new_name, "%s-%d.in", name, i);
    ret = mkfifo(new_name, S_IRWXU);
    if ((ret == -1) && (errno != EEXIST)) {
	    perror("Error creating the named cpipe (in)");
	    exit (1);
    }

    memset(new_name, 0, strlen(new_name));
    sprintf(new_name, "%s-%d.out", name, i);
    ret = mkfifo(new_name, S_IRWXU);
    if ((ret == -1) && (errno != EEXIST)) {
	    perror("Error creating the named pipe (out)");
	    exit (1);
    }
    memset(new_name, 0, strlen(new_name));
  }
}

int read_fifo(const char* fifo, char* msg, size_t size) {
  int fd, select_ret, nread;
  fd_set readfds;
  struct timeval tv;

  fd = open(fifo, O_RDONLY | O_NONBLOCK);

  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);

  tv.tv_sec = 1;
  tv.tv_usec = 10000;

  if((select_ret = select(fd+1, &readfds, NULL, NULL, &tv)) == -1) {
    perror("select error");
    return -1;
  }

  else if(select_ret == 0) {
    /* printf("[debug] read_fifo: nothing to read...\n"); */
    return 0;
  }

  if((nread = read(fd, msg, size)) == -1)
    perror("read error");

  close(fd);
  printf("[debug] read_fifo: read %d bytes\n", nread);
  return nread;
}

int write_fifo(const char* fifo, char* msg, size_t size) {
  int fd, nwrote;
  fd = open(fifo, O_WRONLY | O_NONBLOCK);
  if((nwrote = write(fd, msg, size)) != size)
    perror("Write Error");
  close(fd);
  /* printf("[debug] write_fifo: wrote %d bytes\n", nwrote); */
  return nwrote;
}

int begin_server(const char* name, int nclients) {
  printf("Chat server begins [nclient = %d]\n", nclients);
  pid_t pid;
  pid = fork();
  if(pid == 0) { // stdin poll
    char cmd[BUFMAX];
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
    int i, nread, nwrote;
    bool connected[nclients];
    char* return_fifo[nclients];
    char* cmd;
    char* args;
    char in_msg[BUFMAX];
    char out_msg[BUFMAX];

    for(i=0;i<=nclients;i++)
      connected[i] = 0;

    // create enough fifos for number of clients
    create_fifos(name, nclients);

    while(1) {

      for(i=0;i<nclients;i++) {
        if(waitpid(pid, NULL, WNOHANG) == pid)
          return 0;

        memset(in_msg, 0, sizeof(in_msg));

        printf("[debug] server: reading from '%s'\n", in_fifos[i]);

        if ((nread = read_fifo(in_fifos[i], in_msg, sizeof(in_msg))) < 0) {
          // handle error
        }

        else if(nread == 0) // nothing to read
          continue;

        else if(nread > 0) {
          printf("[debug] server: read '%s' (%d bytes) from '%s'\n", in_msg, nread, in_fifos[i]);

          return_fifo[i] = strtok(in_msg, ", \n"); // fifo-i.out
          printf("[debug] server: return_fifo is '%s'\n", return_fifo[i]);

          cmd = strtok(NULL, ", \n"); // open, <, to, who
          printf("[debug] server: cmd: '%s'\n", cmd);

          args = strtok(NULL, "\n"); // arguments

          // first time connecting
          if(connected[i] == false) { // connect user to session
            connected[i] = true;
            sprintf(out_msg, "[server] User '%s' connected on FIFO %d", args, 0); // TODO: update to user i
          }
          else if(strcmp(cmd, "<") == 0) { // copy message and echo back
            strcpy(out_msg, args);
          }
          else if(strcmp(cmd, "close") == 0) { // close the user's chat session (don't have to notify other users)
            connected[i] = false;
            sprintf(out_msg, "[server] done");
          }
          // NOTE: server doesn't have to reply to 'exit' command from client because client is terminated
          else if(strcmp(cmd, "exit") == 0) { // close the user's chat session (don't have to notify other users)
            connected[i] = false;
          }
          // else echo whatever was sent
          else {
            strcat(cmd, " ");
            strcat(cmd, args);
            strcpy(out_msg, cmd);
          }

          printf("[debug] server: writing '%s' to %s\n", out_msg, return_fifo[i]);

          // write message to return fifo
          if((nwrote = write_fifo(return_fifo[i], out_msg, strlen(out_msg))) != strlen(out_msg)) {
            // handle error
            break; // or return 0
          }
        }
      }
    }
  }
  return 0;
}

int client_poll(int fifo_n) {
  // TODO: pass in username?

  pid_t pid = fork();
  if(pid == 0) { // writing
    int nwrote;
    char cmd_buf[BUFMAX];
    char write_buf[BUFMAX];
    char* cmd;
    char* msg;

    while(1) {
      memset(cmd_buf, 0, sizeof(cmd_buf));

      printf("a2chat_client: ");
      fflush(stdout);

      if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL ||  cmd_buf[0] == '\n')
        continue;

      // msg format: [outfifo name, command, [message]]
      cmd = strtok(cmd_buf, ", \n");
      strcpy(write_buf, out_fifos[fifo_n]); // DO I HAVE TO MEMSET WRITE_BUF?
      strcat(write_buf, " ");
      strcat(write_buf, cmd);

      if(strcmp(cmd, "<") == 0) {
        msg = strtok(NULL, "\n");

        // append message
        strcat(write_buf, " ");
        strcat(write_buf, msg);

        printf("[debug] chat sesh: writing '%s' to %s\n", write_buf, in_fifos[fifo_n]);

        if((nwrote = write_fifo(in_fifos[fifo_n], write_buf, strlen(write_buf))) != strlen(write_buf)) {
          // hendle error
          continue;
        }
      }

      if(strcmp(cmd, "close") == 0) { //terminate chat session
        if((nwrote = write_fifo(in_fifos[fifo_n], write_buf, strlen(write_buf))) != strlen(write_buf)) {
          // hendle error
        }
        printf("[debug] chat sesh: exiting write process...\n");
        _Exit(EXIT_SUCCESS);
      }

      else if(strcmp(cmd, "exit") == 0) { // terminate chat session and client process
        if((nwrote = write_fifo(in_fifos[fifo_n], write_buf, strlen(write_buf))) != strlen(write_buf)) {
          // hendle error
        }
        printf("[debug] chat sesh: exiting write process...\n");
        _Exit(EXIT_FAILURE);
      }
    }
  }
  else { // read process
    int nread, status;
    char read_buf[BUFMAX];

    while(1) {

      // TODO: GET STATUS AND RETURN -1 IF FAILURE (EXIT)
      if(waitpid(pid, &status, WNOHANG) == pid) {
        printf("[debug] chat sesh: exiting read process...\n");
        return 0;
      }

      memset(read_buf, 0, sizeof(read_buf));
      /* printf("[debug] chat sesh: reading from %s\n", out_fifos[fifo_n]); */
      if((nread = read_fifo(out_fifos[fifo_n], read_buf, sizeof(read_buf))) == -1) {
        // handle error
      }

      // nothing to write
      else if(nread == 0) {
        continue;
      }

      // echo to stdout
      else
        printf("%s\n", read_buf);
    }
  return 0;
  }
}


// ISSUES : if you start client first, start the client, and open a chat session, we lose stdout...

int begin_client(const char* name) {
  int i, fd, nread, nwrote;
  char cmd_buf[BUFMAX];
  char msg[BUFMAX];
  char* username;
  char* cmd;

  printf("Chat client begins\n");

  // client loop
  while(1) {
    /* memset(cmd, 0, sizeof(msg)); */
    memset(cmd_buf, 0, sizeof(cmd_buf));

    printf("a2chat_client: ");
    fflush(stdout);

    if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL || cmd_buf[0] == '\n')
      continue;

    cmd = strtok(cmd_buf, ", \n");

    if(strcmp(cmd,"open") == 0) {
      username = strtok(NULL, ", \n");

      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        fd = open(in_fifos[i], O_RDWR|O_NONBLOCK);

        // test if locked
        // 0 if unlocked or locked by this process
        if(lockf(fd, F_TEST, 0) != 0) {
          close(fd);
          if(i == NMAX-1)
            printf("All FIFOs are currently in use. Please try again later.\n");
        }

        else {
          lockf(fd, F_LOCK, BUFMAX);
          printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifos[i], getpid());

          /* if(lockf(fd, F_TEST, 0) == 0) { */
          /*   printf("UNLOCKED\n"); */
          /* } */

          /* sleep(3); */

          // write into the inFIFO to notify the server
          // msg: [outfifo name, command, message]
          strcpy(msg, out_fifos[i]);
          strcat(msg, " ");
          strcat(msg, cmd);
          strcat(msg, " ");
          strcat(msg, username);

          printf("[debug] client: writing '%s' to inFIFO[%d] to notify server\n", msg, i);

          /* printf("3\n"); */
          /* sleep(1); */
          /* printf("2\n"); */
          /* sleep(1); */
          /* printf("1\n"); */
          /* sleep(1); */

          if((nwrote = write_fifo(in_fifos[i], msg, strlen(msg))) != strlen(msg)) {
            // handle error
            lockf(fd, F_ULOCK, BUFMAX);
            break;
          }
          printf("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, i);

          /* sleep(3); */

          memset(msg, 0, strlen(msg));
          if((nread = read_fifo(out_fifos[i], msg, sizeof(msg))) < 0) {
            printf("unexpected server error. could not read. quitting chat session...\n");
            lockf(fd, F_ULOCK, BUFMAX);
            break;
          }
          if(nread == 0) {
            printf("server not responding. quitting chat session...\n");
            lockf(fd, F_ULOCK, BUFMAX);
            break;
          }
          printf("%s\n", msg);

          /* sleep(3); */

          // 0 = close
          // -1 = exit or err
          int ret;
          ret = client_poll(i);
          // unlock fifo
          lockf(fd, F_ULOCK, BUFMAX);
          printf("Chat session closed. FIFO [%s] unlocked by PID %d\n", in_fifos[i], getpid());

          if(ret == 0) break;
          else return -1;
        }
      }
    }

    if(strcmp(cmd, "<") == 0) {
      printf("This user is not connected to a chat session!\n");
    }

    else if(strcmp(cmd, "close") == 0) {
      printf("No session to close.\n");
    }

    else if(strcmp(cmd,"exit") == 0) {
      // terminate
      printf("exiting...\n");
      return 0;
    }
  }
  return 0;
}

int main(int argc, const char* argv[]) {
  if(argc < 2 || argc > 4) // at least 2 args, max 4 args
    return usage();
  // get baseName and create inFIFOs and outFIFOs
  const char* baseName = argv[2];
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
