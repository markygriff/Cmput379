/* Mark Griffith - 1422270 */

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
#include <sys/resource.h>
#include <fcntl.h>
#include <time.h>


/* #define DEBUG */

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

typedef int bool;
#define false 0
#define true 1

#define NMAX 5
#define BUFMAX 1024


// stores information regarding a
// client's current chat session
typedef struct client_map {
  bool connected;
  int chatters;
  char user[BUFMAX];
  char* return_fifos[NMAX];
} client_map_t, *client_map_p;


int usage() {
  printf("usage: a2chat -s [baseName nclient]\n");
  printf("       a2chat -c [baseName]\n");
  return -1;
}

/// Helper function to create named pipes for chat
void create_fifos(const char* name, int num) {
  int i, ret;
  char new_name[strlen(name) + 6];
  for(i=1;i<=num;i++) {
    memset(new_name, 0, strlen(new_name));
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
  }
}

/// Helper function to remove substring
void remove_substring(char *str, const char *toRemove) {
  while( (str = strstr(str,toRemove)) ) {
    int len = strlen(toRemove);
    memmove(str, str + len, 1 + strlen(str + len));
  }
}

/// Function to handle server reading and writing
int begin_server(const char* name, int nclients) {
  printf("Chat server begins [nclient = %d]\n", nclients);
  pid_t pid;
  pid = fork();

  if(pid == 0) {
    char cmd[BUFMAX];
    while(1) {
      // poll stdin
      scanf("%s", cmd);
      // terminate server
      if(strcmp(cmd, "exit") == 0) {
        printf("exiting server...\n");
        _Exit(EXIT_SUCCESS);
      }
    }
  }
  else {
    int i, j, select_ret, nread, nwrote, fd;
    char* cmd;
    char* args;
    char in_msg[BUFMAX];
    char out_msg[BUFMAX];
    char serv_msg[BUFMAX];
    char in_fifo[strlen(name) + 6];

    fd_set readfds;
    struct timeval tv;

    // initialize clients
    client_map_t clients[nclients];
    for(i=0;i<nclients;i++) {
      strcpy(clients[i].user,"");
      clients[i].connected = false;
      clients[i].chatters = 0;
      for(j=0;j<NMAX;j++)
        clients[i].return_fifos[j] = (char *) malloc(BUFMAX);;
    }

    // create enough fifos for number of clients
    create_fifos(name, nclients);

    while(1) {

      // check if child process is terminated
      for(i=0;i<nclients;i++) {
        if(waitpid(pid, NULL, WNOHANG) == pid)
          return 0;

        memset(in_msg, 0, sizeof(in_msg));

        // get infifo to read from
        sprintf(in_fifo, "%s-%d.in", name, i+1);

        /* DEBUG_PRINT(("[debug] server: reading from %d '%s'\n", i, in_fifo)); */

        // open from the infifo to read from
        fd = open(in_fifo, O_RDONLY | O_NONBLOCK);

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        tv.tv_sec = 0; // seconds
        tv.tv_usec = 1; // microseconds

        // check if the infifo has data to read
        if((select_ret = select(fd+1, &readfds, NULL, NULL, &tv)) == -1) {
          close(fd);
          perror("select error");
          continue;
        }
        // nothing to read
        else if(select_ret == 0) {
          close(fd);
          continue;
        }

        // read from the infifo
        if ((nread = read(fd, in_msg, sizeof(in_msg))) < 0) {
          DEBUG_PRINT(("[debug] server: read error\n"));
          // a read error here should not cause the server to fail
          close(fd);
          continue;
        }
        else if(nread == 0) {
          close(fd);
          continue;
        }

        // read some data
        else if(nread > 0) {
          close(fd);

          DEBUG_PRINT(("[debug] server: read '%s' (%d bytes) from '%s'\n", in_msg, nread, in_fifo));

          // NOTE: client message format: [return fifo, command, arguments]

          char* tok;
          tok = strtok(in_msg, ", \n");

          // this is strictly a sanity check.
          // we know that in_msg is not null because of the checks above
          if(NULL != tok)
            strcpy(clients[i].return_fifos[0], tok); // NOT INITIALIZED
          else
            break;

          DEBUG_PRINT(("[debug] server: return_fifo is '%s'\n", clients[i].return_fifos[0]));

          // command
          cmd = strtok(NULL, ", \n");
          DEBUG_PRINT(("[debug] server: cmd: '%s'\n", cmd));

          // arguments
          if((args = strtok(NULL, "\n")) == NULL)
            args = "";

          // first time connecting
          // add user to list of users
          if(strcmp(cmd, "open") == 0 && clients[i].connected == false) {
            int x;
            int exists = false;

            // check if username is taken by another client
            for(x=0;x<nclients;x++) {
              if(strcmp(args, clients[x].user) == 0) {
                DEBUG_PRINT(("[debug] server: user '%s' already exists!\n", clients[x].user));
                exists = true;
                sprintf(serv_msg, "[server] error: user '%s' already exists", args);
              }
            }

            if(exists == false) {
              DEBUG_PRINT(("[debug] server: adding user '%s' to clients\n", args));
              // connect user to clients[i]
              clients[i].connected = true;
              memset(clients[i].user, 0, sizeof(clients[i].user));
              strcpy(clients[i].user, args);

              // create message for client
              sprintf(serv_msg, "[server] User '%s' connected on FIFO %d", args, i+1);
            }
          }

          // copy message and send to chat
          else if(strcmp(cmd, "<") == 0) {

            sprintf(out_msg, "\n[%s] %s", clients[i].user, args);

            // write to each recipient's out-fifo that is part of the chat session
            for(j=0;j<=clients[i].chatters;j++) {
              if(clients[j].connected == false)
                continue;

              fd = open(clients[i].return_fifos[j], O_WRONLY | O_NONBLOCK);

              DEBUG_PRINT(("[debug] server: writing out message to user %s -> '%s' to %s\n", clients[j].user, out_msg, clients[j].return_fifos[0]));

              if((nwrote = write(fd, out_msg, strlen(out_msg))) != strlen(out_msg))
                sprintf(serv_msg, "[server] error: failed to send message to a user.");

              close(fd);
            }
            if(strncmp(serv_msg, "[server] error:", 15) != 0)
              sprintf(serv_msg, "[server] done");
          }

          // add users to senders
          else if(strcmp(cmd, "to") == 0) {

            // assert user is not trying to add itself as a recipient
            if(strcmp(args, clients[i].user) == 0)
              sprintf(serv_msg, "[server] error: you cannot add yourself as a recipient.");

            // attempt to find users to add to client's recipients
            else {
              char connected_users[BUFMAX] = "";

              // assert that the recipients are connected users
              for(j=0;j<nclients;j++) {

                DEBUG_PRINT(("[debug] server: checking '%s' for '%s'...\n", args, clients[j].user));

                // if client is connected and recipeint list includes client username...
                // add the client's return_fifos[0] to current sender's return_fifos
                if(clients[j].connected == true && strstr(args, clients[j].user) != NULL) {
                  DEBUG_PRINT(("[debug] server: adding user %s's return-fifo\n", clients[j].user));

                  // increment the number of users connected to the chat session
                  clients[i].chatters++;
                  clients[i].return_fifos[clients[i].chatters] = clients[j].return_fifos[0];

                  DEBUG_PRINT(("[debug] server: removing '%s' from args\n", clients[j].user));

                  // add user to list of connected users
                  strcat(connected_users, clients[j].user);
                  strcat(connected_users, " ");

                  // remove user from args. empty args should break the loop
                  remove_substring(args, clients[j].user);

                  DEBUG_PRINT(("[debug] server: args -> '%s'\n", args));
                  DEBUG_PRINT(("[debug] server: connected_users -> '%s'\n", connected_users));
                  DEBUG_PRINT(("[debug] server: %s's return_fifos[%d] is now %s\n", clients[i].user, clients[i].chatters, clients[j].return_fifos[0]));

                  if(strcmp(args, "") == 0)
                    break;
                }
              }

              DEBUG_PRINT(("[debug] server: chatter is now '%d'\n", clients[i].chatters));

              sprintf(serv_msg, "[server] recipients added: %s", connected_users);
            }
          }

          // print who is connected to client[i]'s session
          else if(strcmp(cmd, "who") == 0) {
            sprintf(serv_msg, "[server] Current users:");

            int x;
            char user_str[BUFMAX];
            // find the users connected to clients[i]'s session
            for(j=0;j<=clients[i].chatters;j++) {
              for(x=0;x<nclients;x++) { // for each client
                // check if clients[x] is in clients[i]'s chat session
                if(clients[i].return_fifos[j] == clients[x].return_fifos[0]) {
                  DEBUG_PRINT(("[debug] server: catting user %s to server message\n", clients[x].user));

                  // add clients[x] to server message
                  sprintf(user_str, " [%d] %s", j+1, clients[x].user);
                  strcat(serv_msg, user_str);
                  break;
                }
              }
            }
          }

          // close the user's chat session (don't have to notify other users)
          else if((strcmp(cmd, "close") == 0) || (strcmp(cmd, "exit") == 0))  {
            DEBUG_PRINT(("[debug] server: removing user '%s' from clients\n", clients[i].user));

            // update clients who have the removed user as a recipient
            int x;
            for(j=0;j<nclients;j++) { // clients
              for(x=0;x<=clients[j].chatters;x++) { // return fifos
              if(clients[j].return_fifos[x] == clients[i].return_fifos[0]) {
                memset(clients[j].return_fifos[x], 0, strlen(clients[j].return_fifos[x]));
                clients[j].chatters--;
                }
              }
            }

            // destroy client user
            clients[i].connected = false;


            for(j=0;j<clients[i].chatters;j++)
              memset(clients[i].return_fifos[j], 0, strlen(clients[i].return_fifos[j]));

            clients[i].chatters = 0;
            memset(clients[i].user, 0, strlen(clients[i].user));

            sprintf(serv_msg, "[server] done");
          }

          // else, unsupported command. Do nothing
          else
            continue;

          DEBUG_PRINT(("[debug] server: writing server message to user %s -> '%s' to %s\n", clients[i].user, serv_msg, clients[i].return_fifos[0]));

          // write server response to the current client's out-fifo
          fd = open(clients[i].return_fifos[0], O_WRONLY | O_NONBLOCK);
          if((nwrote = write(fd, serv_msg, strlen(serv_msg))) != strlen(serv_msg)) {
            // handle error
          }
          close(fd);
        }
      }
    }
  }
  return 0;
}

/// Initializes chat session for a client process
/// Spawns two processes: read and write
/// Returns: -1 on err, 1 on exit, 0 on close
int client_chat(char* username, char* in_fifo, char* out_fifo) {

  // fork a child process for writing to the server
  // the parent process will read from the server
  pid_t pid = fork();

  if(pid < 0) {
    printf("fork error. exiting chat session...\n\n");
    return -1;
  }

  if(pid == 0) { // write process
    int fd, nwrote;
    char cmd_buf[BUFMAX];
    char write_buf[BUFMAX];
    char* cmd;
    char* msg;

    // lock the fifo to the client process
    fd = open(in_fifo, O_WRONLY | O_NONBLOCK);
    lockf(fd, F_LOCK, BUFMAX);

    printf("FIFO [%s] has been successfully locked by PID %d\n", in_fifo, getpid());

    printf("\n***chat session opened***\n");
    printf("     welcome %s\n", username);

    while(1) {
      // sleep to allow time for server response
      usleep(100000);

      memset(cmd_buf, 0, sizeof(cmd_buf));
      memset(write_buf, 0, sizeof(write_buf));

      // user prompt
      printf("a2chat_client: ");
      fflush(stdout);

      if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL ||  cmd_buf[0] == '\n')
        continue;

      // msg format: [outfifo, command, [message]]
      cmd = strtok(cmd_buf, ", \n");
      strcpy(write_buf, out_fifo);
      strcat(write_buf, " ");
      strcat(write_buf, cmd); // [outfifo] [cmd]

      // write message to infifo
      if(strcmp(cmd, "<") == 0 || strcmp(cmd, "to") == 0 || strcmp(cmd, "who") == 0) {

        // determine if there is a message to write
        if((msg = strtok(NULL, "\n")) != NULL) {
          strcat(write_buf, " ");
          strcat(write_buf, msg);
        }

        DEBUG_PRINT(("[debug] chat sesh: writing '%s' to %s\n", write_buf, in_fifo));

        if((nwrote = write(fd, write_buf, strlen(write_buf))) != strlen(write_buf)) {
          // notify user that message did not go through to server
          printf("\n*[oops! an error occured when sending your message. please try again...]*\n\n");
          continue;
        }
      }

      //terminate chat session
      // exit 0
      else if(strcmp(cmd, "close") == 0) {
        if((nwrote = write(fd, write_buf, strlen(write_buf))) != strlen(write_buf)) {
          perror("write error");
        }

        // breifly sleep so server can remove user
        usleep(100000);

        DEBUG_PRINT(("[debug] chat sesh: exiting write process...\n"));

        // unlock fifo and close fd
        lockf(fd, F_ULOCK, BUFMAX);
        close(fd);
        _Exit(0);
      }

      // terminate chat session and client process
      // exit 1
      else if(strcmp(cmd, "exit") == 0) {

        if((nwrote = write(fd, write_buf, strlen(write_buf))) != strlen(write_buf)) {
          perror("write error");
        }

        // breifly sleep so server can remove user
        usleep(100000);

        DEBUG_PRINT(("[debug] chat sesh: exiting write process...\n"));

        // unlock fifo and close fd
        lockf(fd, F_ULOCK, BUFMAX);
        close(fd);
        _Exit(1);
      }
    }
  }
  else { // simple read process
    int fd, nread, status;
    char read_buf[BUFMAX];

    // we want to read from our out-fifo for incoming
    // server messages
    fd = open(out_fifo, O_RDONLY | O_NONBLOCK);

    while(1) {

      if(waitpid(pid, &status, WNOHANG) == pid) {
        DEBUG_PRINT(("[debug] chat sesh: exiting read process...\n"));
        DEBUG_PRINT(("[debug]exit status was %d\n", WEXITSTATUS(status)));

        // return 1 if we are to terminate the client
        close(fd);
        if(WEXITSTATUS(status) == 1)
          return 1;

        else
        return 0;
      }

      memset(read_buf, 0, sizeof(read_buf));
      if((nread = read(fd, read_buf, sizeof(read_buf))) == -1) {
        continue;
      }

      // nothing to write
      else if(nread == 0) {
        continue;
      }

      // echo incoming message to stdout
      else {
        printf("%s\n", read_buf);

        char buf[BUFMAX] = "\n[";
        strcat(buf, username);
        // if the message is not from this user or the server
        // re-display the user prompt
        if((strncmp(read_buf, "[server]", 8) != 0) && (strncmp(read_buf, buf, strlen(buf)-1) != 0)) {
          printf("a2chat_client: ");
          fflush(stdout);
        }
      }
    }
  }
}

/// Client process handling outside of a chat session
int begin_client(const char* name) {
  int i, fd, nread, nwrote;
  char cmd_buf[BUFMAX];
  char msg[BUFMAX];
  char in_fifo[strlen(name) + 6];
  char out_fifo[strlen(name) + 6];
  char* username;
  char* cmd;

  printf("Chat client begins\n");

  // client loop
  while(1) {
    memset(cmd_buf, 0, sizeof(cmd_buf));

    // user prompt
    printf("a2chat_client: ");
    fflush(stdout);

    // get user input
    if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL || cmd_buf[0] == '\n')
      continue;

    // extract user command
    cmd = strtok(cmd_buf, ", \n");

    // attempt to open a chat session for this client
    if(strcmp(cmd,"open") == 0) {

      // check for an unlocked inFIFO
      for(i=0;i<NMAX;i++) {
        sprintf(in_fifo, "%s-%d.in", name, i+1);
        fd = open(in_fifo, O_RDWR|O_NONBLOCK);

        // test if fifo is locked
        // 0 if unlocked or locked by this process
        if(lockf(fd, F_TEST, 0) != 0) {
          DEBUG_PRINT(("[debug] client: attempted lock %s ... already locked\n", in_fifo));
          close(fd);
          if(i == NMAX-1)
            printf("All FIFOs are currently in use. Please try again later.\n");
        }

        else {
          // if username not specified, give the user a default username
          if((username = strtok(NULL, "")) == NULL) {
            char buf[10];
            sprintf(buf, "Default User %d", i+1);
            username = buf;
          }

          // write into the fifo.in to notify the server
          // msg format: [outfifo, command, message]
          sprintf(out_fifo, "%s-%d.out", name, i+1);
          strcpy(msg, out_fifo);
          strcat(msg, " ");
          strcat(msg, cmd);
          strcat(msg, " ");
          strcat(msg, username);

          DEBUG_PRINT(("[debug] client: writing '%s' to inFIFO[%d] to notify server\n", msg, i));

          // write message to infifo
          if((nwrote = write(fd, msg, strlen(msg))) != strlen(msg)) {
            printf("write error. could not connect to server. please try again...\n");
            break;
          }

          // NOTE: Don't close the in-fifo that we just wrote to immediately!
          //       The server might not have read the message yet
          //       so if the fd is closed, it will never read it.

          DEBUG_PRINT(("[debug] client: wrote '%s' (%d bytes) to inFIFO[%d]\n", msg, nwrote, i));

          memset(msg, 0, sizeof(msg));

          // we want to block this read open to make
          // sure the server has it open for write
          fd = open(out_fifo, O_RDONLY);

          // read server message
          // due to server timing not always being perfectly in sync
          // with the client, read errors may occur
          if((nread = read(fd, msg, sizeof(msg))) < 0) {
            printf("unexpected read error. please try again...\n");
            break;
          }
          if(nread == 0) {
            printf("read error. server not responding. please wait...\n");
            // allow time for server to read message
            sleep(1);
            if((nread = read(fd, msg, sizeof(msg))) <= 0) {
              printf("error connecting with server...\n");
              break;
            }
          }

          // dump server response
          printf("%s\n", msg);
          close(fd);

          // check server response for warnings/errors
          if(strncmp(msg, "[server] error:", 15) == 0) {
            DEBUG_PRINT(("[debug] client: server error. breaking\n"));
            break;
          }

          // 0 = close
          // 1 = exit
          // -1 = err
          int ret;
          ret = client_chat(username, in_fifo, out_fifo);

          printf("\n    goodbye %s", username);
          printf("***chat session closed***\n\n");
          printf("FIFO [%s] unlocked by PID %d\n\n", in_fifo, getpid());

          if(ret == 0)
            break;

          else if(ret == 1) {
            printf("exiting...\n");
            return 0;
          }

          else
            return -1;
        }
      }
    }

    if((strcmp(cmd, "<") == 0) || (strcmp(cmd, "to") == 0) || (strcmp(cmd, "who") == 0)) {
      printf("You are not connected to a chat session!\n");
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

/// Processes input arguments
int main(int argc, const char* argv[]) {
  // check for at least 2 args, max 4 args
  if(argc < 2 || argc > 4)
    return usage();

  // set a limit on CPU time (e.g. 10 minutes)
  struct rlimit cpu_lim = {600, 600};
  if(setrlimit(RLIMIT_CPU, &cpu_lim) == -1) {
    perror("setrlimit: failed\nError ");
    return -1;
   }

  const char* baseName = argv[2];
  if((strcmp(argv[1], "-s") == 0) && argc == 4) { // server
    int nclient;

    // check if num of clients is valid
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
