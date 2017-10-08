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
  printf("usage: a2chat -s baseName [nclient]\n");
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
    //TODO: find a way of exiting properly. Right now, if the server
    //      gets blocked on a read, it will hang indefinetly. So exiting
    //      from here doesn nothing
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
    int i, nread, nwrote;
    char* return_fifo;
    char* cmd;
    char* buf;
    char in_msg[BUFMAX];
    char out_msg[BUFMAX];

    create_fifos(name, nclient);

    while(1) {

      // TODO: create FIFOs for the client

      memset(in_msg, 0, sizeof(in_msg));
      printf("[debug] server: reading from '%s'\n", in_fifos[0]);
      if ((nread = read_fifo(in_fifos[0], in_msg, sizeof(in_msg))) > 0)
        printf("[debug] server: read '%s' (%d bytes) from '%s'\n", in_msg, nread, in_fifos[0]);

      if(nread == 0)
        continue;

      if (nread > 0) {
        return_fifo = strtok(in_msg, ", \n");
        printf("[debug] server: return_fifo: '%s'\n", return_fifo);

        cmd = strtok(NULL, ", \n");
        printf("[debug] server: cmd: '%s'\n", cmd);

        buf = strtok(NULL, ", \n");
        strcpy(out_msg, "[server] ");
        strcat(out_msg, buf);
        printf("[debug] server: msg: '%s'\n", out_msg);

        if((nwrote = write_fifo(return_fifo, out_msg, strlen(out_msg))) != strlen(out_msg)) {
          perror("write error");
          break;
        }
        return 0;

      }


      // poll inFIFOs
      /* for(i=0;i<(2*NMAX);i+=2) { */
      /*   memset(msg, 0, sizeof(msg)); */
      /*   printf("[debug] server: reading from '%s'\n", in_fifos[i]); */
      /*   if ((nread = read_fifo(in_fifos[i], msg, sizeof(msg))) > 0) { */
      /*     printf("[debug] server: read '%s' (%d bytes) from '%s'\n", msg, nread, in_fifos[i]); */
      /*       // fifo doesn't belong to another client */
      /*       if(in_fifos[i+1] == NULL) { */
      /*       in_fifos[i+1] = strdup(msg); */
      /*       char resp[BUFMAX]; */
      /*       // write response */
      /*       sprintf(resp, "[server] User '%s' has been successfully connected to FIFO %d\n", in_fifos[i+1], i+1); */
      /*      memset(msg, 0, sizeof(msg)); */
      /*       nwrote = write_fifo(out_fifos[i], resp, sizeof(resp)); */
      /*       printf("[debug] server: wrote '%s' (%d bytes) to outFIFO[%d]\n", resp, nwrote, i); */
      /*     } */
      /*     // fifo belongs to a client */
      /*     else { */
      /*       // NOTE: we are just echoing the msg back to the client for now */
      /*       // TODO: echo msg to recipients of the session */
      /*       memset(msg, 0, strlen(msg)); */
      /*       nwrote = write_fifo(out_fifos[i], msg, sizeof(msg)); */
      /*       printf("server: [debug] wrote %s (%d bytes) to outFIFO[%d]\n", msg, nwrote, i); */
      /*     } */
      /*   } */
      /* } */
      /* if(waitpid(pid, &status, WNOHANG) == pid) */
      /*   _exit(EXIT_SUCCESS); */
    }
  }
  return 0;
}

/* int client_poll(int fifo_n) { */

/*   // TODO: pass in username? */

/*   static char msg[BUFMAX]; */
/*   int nread, nwrote, status; */
/*   pid_t pid = fork(); */
/*   if(pid == 0) { // outFIFO poll */
/*     while((strcmp(msg, "done") != 0) || (strcmp(msg, "exit") != 0) ) { */
/*       memset(msg, 0, sizeof(msg)); */
/*       printf("a2chat_client: "); */
/*       scanf("%s", msg); */
/*       if(strcmp(msg, "<")) { */
/*         nwrote = write_fifo(in_fifos[fifo_n], msg, sizeof(msg)); */
/*         printf("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, fifo_n); */
/*       } */
/*       if(strcmp(msg, "done") == 0) { //terminate chat session */
/*         break; */
/*       } */
/*       else if(strcmp(msg, "exit") == 0) { // terminate chat session and client process */
/*         // TODO: figure out how to terminate client from here */
/*         break; */
/*       } */
/*       else {} */
/*     } */
/*     _Exit(EXIT_SUCCESS); */
/*   } */
/*   else { // inFIFO writing */
/*     while(1) { */
/*       memset(msg, 0, sizeof(msg)); */
/*       printf("[debug] client: reading from outFIFO[%d]\n", fifo_n); */
/*       nread = read_fifo(out_fifos[fifo_n], msg, sizeof(msg)); */
/*       printf("[debug] client: read '%s' (%d bytes) from outFIFO[%d]\n", msg, nread, fifo_n); */
/*       if(waitpid(pid, &status, WNOHANG) == pid) break; */
/*     } */
/*   // TODO: delete username from in_fifos? */
/*   return 0; */
/*   } */
/* } */

int begin_client(const char* name) {
  int i, fd, nread, nwrote;
  char cmd_buf[BUFMAX];
  char msg[BUFMAX];
  char* username;
  char* cmd;
  char* args;

  printf("Chat client begins\n");

  // client loop
  while(1) {
    /* memset(cmd, 0, sizeof(msg)); */
    memset(cmd_buf, 0, sizeof(cmd_buf));

    printf("a2chat_client: ");
    fflush(stdout);
    /* if((nread = read(STDIN_FILENO, cmd_buf, sizeof(cmd_buf))) == -1) */
    /*   perror("stdin read error"); */
    /* if(nread > 0) { */
    /*   cmd = strtok(cmd_buf, ", \n"); */
    /*   while(args[i] != NULL && i < ARGSMAX) */
    /*     args[++i] = strtok(NULL, ", \n"); */
    /*   int num_args = i; */
    /*   printf("cmd: '%s'\n", cmd); */

    if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL)
      continue;

    cmd = strtok(cmd_buf, ", \n");

    if(strcmp(cmd,"open") == 0) {
      username = strtok(NULL, ", \n");
      /* args = strtok(NULL, ""); */

      /* if(username[0] != '\0') { */
      /*   printf("This client already has the username '%s'\n", username); */
      /*   break; */
      /* } */
      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        fd = open(in_fifos[i], O_WRONLY);
        // lock the inFIFO
        if(lockf(fd, F_TEST, 0) == 0) { //unlocked
          lockf(fd, F_LOCK, BUFMAX);
          printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifos[i], getpid());

          // write into the inFIFO to notify the server
          // msg: [outfifo name, command, message]
          strcpy(msg, out_fifos[i]);
          strcat(msg, " ");
          strcat(msg, cmd);
          strcat(msg, " ");
          strcat(msg, username);

          /* strcat(msg, " "); */
          /* strcat(msg, cmd); */
          /* strcat(msg, " "); */
          /* strcat(msg, username); */

          printf("client: [debug] writing '%s' to inFIFO[%d] to notify server\n", msg, i);
          if((nwrote = write_fifo(in_fifos[i], msg, strlen(msg))) != strlen(msg)) {
            perror("write error");
            // TODO: what to do here?
            break;
          }
          printf("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, i);

          // await server response
          memset(msg, 0, strlen(msg));
          nread = read_fifo(out_fifos[i], msg, sizeof(msg)); // this read is blocked
          /* printf("client: [debug] read '%s' (%d bytes) from outFIFO[%d]\n", msg, nread, i); */
          printf("%s\n", msg);
          return 0;

          // begin chat session
          /* client_poll(i); */

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

    if(strcmp(cmd,"exit") == 0) {
      // terminate
      printf("client: [debug] exiting...\n");
      _exit(EXIT_SUCCESS);
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
