/* Mark Griffith - 1422270 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>


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
#define KAL_char 0x6 // ACK
#define KAL_length 5
#define KAL_interval 5// seconds
#define KAL_count 5 // number of tries before termination

#define h_addr h_addr_list[0]

volatile sig_atomic_t print_flag = false;

// stores information regarding a
// client's current chat session
typedef struct client_map {
  int chatters;
  bool connected;
  int kal_misses;
  int descriptors[NMAX];
  char user[BUFMAX];
  time_t timestamp;
} client_map_t, *client_map_p;


int usage() {
  printf("usage: a2chat -s [portnumber nclient]\n");
  printf("       a2chat -c [portnumber serverAddress]\n");
  return -1;
}

/// Alarm signal handler
void handle_alarm( int sig ) {
    print_flag = true;
}

/// Helper function to remove substring
void remove_substring(char *str, const char *toRemove) {
  while( (str = strstr(str,toRemove)) ) {
    int len = strlen(toRemove);
    memmove(str, str + len, 1 + strlen(str + len));
  }
}

// create the keep alive message
void get_kal_msg(char* kal_msg, int size) {
  int i;
  int len = KAL_length;
  char kal_char[1] = {KAL_char};

  memset(kal_msg, 0, size);

  for(i=0;i<len;i++) {
    strcat(kal_msg, kal_char);
  }
}

/// Function to handle server reading and writing
int begin_server(const char* portnumber, int nclients) {
  printf("Chat server begins [port = %s] [nclient = %d]\n", portnumber, nclients);
  pid_t pid;
  pid = fork();

  if(pid == 0) {
    char cmd[BUFMAX];
    pid_t ppid_current;
    pid_t ppid_original = getppid();

    while(1) {
      // poll stdin
      scanf("%s", cmd);

      // terminate server
      if(strcmp(cmd, "exit") == 0) {
        printf("exiting server...\n");
        _Exit(EXIT_SUCCESS);
      }

      ppid_current = getppid();

      if(ppid_current != ppid_original) {
        DEBUG_PRINT(("[debug] parent process terminated. exiting...\n"));
        _Exit(EXIT_FAILURE);
      }
    }
  }
  else {
    client_map_t clients[nclients];

    int i, j;
    int pr, nread, nwrote, on = 1;
    int listen_sd, comm_sd = -1;
    int curr_nlisteners, nlisteners = 1;
    bool server_up = true, shrink_polls = false;
    bool close_conn, destroy_client;
    clock_t current, start;
    double elapsed;
    char* cmd;
    char* args;
    char serv_msg[BUFMAX];
    char in_msg[BUFMAX];
    char out_msg[BUFMAX];
    char kal_msg[KAL_length];
    struct sockaddr_in serv;
    struct pollfd fds[nclients+1];

    // create the keep alive message
    get_kal_msg(kal_msg, sizeof(kal_msg));

    // install alarm handler
    signal(SIGALRM, handle_alarm);
    alarm(5);

    // initialize clients
    for(i=0;i<nclients;i++) {
      clients[i].kal_misses = 0;
      strcpy(clients[i].user,"");
      clients[i].chatters = 0;
      clients[i].connected = 0;
      for(j=0;j<nclients;j++)
        clients[i].descriptors[j] = 0;
    }

    // TODO protocol ?
    // create AF_INET socket stream for incoming messages
    listen_sd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_sd < 0) {
      perror("[error] socket");
      return -1;
    }

    // make socket descriptor reusable
    if(setsockopt(listen_sd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
      perror("[error] setsockopt");
      close(listen_sd);
      return -1;
    }

    // make socket (and inherited sockets) nonblocking
    if(ioctl(listen_sd, FIONBIO, (char *)&on) < 0) {
      perror("[error] ioctl");
      close(listen_sd);
      return -1;
    }

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    // allow any IP to connect
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    // listen on port portnumber
    serv.sin_port = htons(atoi(portnumber));

    // get ready to listen for connection on port
    if(bind(listen_sd, (struct sockaddr*) &serv, sizeof(serv)) != 0)
      perror("[error] bind");

    // begin listening for connections
    // listen to at most 'nclients' connections
    if(listen(listen_sd, nclients+1) < 0) {
      perror("[error] listen");
      close(listen_sd);
      return -1;
    }

    // initialize poll and set first
    // fd to listening fd
    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_sd;
    fds[0].events = POLLIN;

    // start the clock
    start = clock();

    do {

      // check if child process is terminated
      if(waitpid(pid, NULL, WNOHANG) == pid)
        server_up = false;

      current = clock();
      elapsed = ((double)current - (double)start)/(double)CLOCKS_PER_SEC;

      // if the KAL_interval has been reached, we can
      // update the kal counter for each client
      if((nlisteners > 1) && (fmod((double)elapsed, (double)KAL_interval) == 0)) {
        for(i=1;i<nlisteners;i++) {
          DEBUG_PRINT(("[debug] checking if max KAL misses have been reached for client[%d] = %s\n", i-1, clients[i-1].user));
          DEBUG_PRINT(("current KAL misses = %d\n", clients[i-1].kal_misses));

          // we can destroy the client and close the connection if
          // the server hasn't read a KAL message from the client
          // KAL_count consecutive times
          if(++clients[i-1].kal_misses == KAL_count) {
            DEBUG_PRINT(("[debug] maximum KAL misses reached.\n"));

            // update clients who have the removed user as a recipient
            int x;
            for(j=0;j<nclients;j++) {
              for(x=0;x<=clients[j].chatters;x++) {
                if(clients[j].descriptors[x] == clients[i-1].descriptors[0] && clients[j].connected == 1) {
                  clients[j].descriptors[x] = 0;
                  clients[j].chatters--;
                }
              }
            }

            // remove allocated client assets
            memset(&clients[i-1], 0, sizeof(clients[i-1]));

            // clean up connection iff close_conn flag is set
            close(fds[i].fd);
            fds[i].fd = -1;

            // shirink poll struct
            DEBUG_PRINT(("[debug] shrinking polls\n"));
            for(i=0;i<nlisteners;i++) {
              if(fds[i].fd == -1) {
                for(j=i;j<nlisteners;j++) {
                  fds[j].fd = fds[j+1].fd;
                  if(i != 0 ) {
                    if(j == nclients) {
                      int n;
                      for(n=0;n<=clients[j-1].chatters;n++)
                        clients[j-1].descriptors[n] = -1;
                      clients[j-1].chatters = 0;
                      clients[j-1].kal_misses = 0;
                      clients[j-1].connected = 0;
                      strcpy(clients[j-1].user, "");
                    }
                    else {
                      clients[j-1] = clients[j];
                    }
                  }
                }
                nlisteners--;
                DEBUG_PRINT(("[debug] nlisteners is now %d\n", nlisteners));
              }
            }
          }
        }
      }

      // print activity report
      if(print_flag && nlisteners > 1) {

        printf("\nactivity report:\n");
        for(i=1;i<nlisteners;i++) {
          if(clients[i-1].descriptors[0] == 0)
            continue;
          printf("'%s' [sockfd = %d]: %s", clients[i-1].user, clients[i-1].descriptors[0], ctime(&clients[i-1].timestamp));
        }
        printf("\n");
        print_flag = false;
        alarm(5);
      }

      // call poll and wait
      pr = poll(fds, nclients+1, 0);

      if(pr < 0) {
        /* perror("[error] poll"); */
      }

      // poll timeout
      if(pr == 0) {
        continue;
      }

      // if we're here, there are one or more fds
      // that have data to read.
      // now we have to determine which fds have data
      curr_nlisteners = nlisteners;

      for(i=0;i<curr_nlisteners;i++) {

        if(fds[i].revents == 0)
          continue;

        // not POLLIN. unexpected
        if(fds[i].revents != POLLIN) {
          DEBUG_PRINT(("[error] unexpected revents: %d\n", fds[i].revents));
        }

        // the listening socket is readable, meaning that a new
        // connection is available
        if(fds[i].fd == listen_sd) {
          DEBUG_PRINT(("[debug] listening socket is readable\n"));

          do {
            // accept the incomming connections
            // when accept fails with EWOULDBLOCK that indicates
            // that we have accepted every incoming connection
            comm_sd = accept(listen_sd, NULL, NULL);

            if(comm_sd < 0) {
              if(errno != EWOULDBLOCK) {
                perror("[error] accept failure");
                server_up = true;
              }
              DEBUG_PRINT(("[debug] accept() errno = EWOULDBLOCK\n"));
              break;
            }

            // server may only accept a maximum of ncients clients
            if(nlisteners > nclients) {
              DEBUG_PRINT(("[debug] maximum descritors reached. skipping.\n"));
              continue;
            }

            if(comm_sd == 0)
              continue;

            // add new listener to poll struct
            DEBUG_PRINT(("[debug] new connection with descriptor: %d\n", comm_sd));
            fds[nlisteners].fd = comm_sd;
            fds[nlisteners].events = POLLIN;
            DEBUG_PRINT(("[debug] fds[%d] is now %d\n", nlisteners, comm_sd));

            nlisteners++;
            DEBUG_PRINT(("[debug] nlisteners is now %d\n", nlisteners));
          } while(comm_sd != -1);
        }

        // not a listening socket, so it is an existing
        // connection
        else {
          DEBUG_PRINT(("[debug] descriptor %d (existing connection) is now readable\n", fds[i].fd));
          close_conn = false;
          destroy_client = false;

          // read incomming data from socket
          do {
            memset(in_msg, 0, sizeof(in_msg));

            DEBUG_PRINT(("[debug] reading from descriptor %d\n", fds[i].fd));
            nread = recv(fds[i].fd, in_msg, sizeof(in_msg), 0);

            if(nread < 0) {
              if(errno != EWOULDBLOCK) {
                perror("[error] recv failed");
                close_conn = true;
              }
              break;
            }

            // check for closed connection
            if(nread == 0) {
              DEBUG_PRINT(("[debug] connection with client closed\n"));
              close_conn = true;
              break;
            }

            /**** this is where all the command processing happens ****/

            DEBUG_PRINT(("[debug] server: read '%s' (%d bytes) from descriptor %d \n", in_msg, nread, fds[i].fd));
            memset(serv_msg, 0, sizeof(serv_msg));
            memset(out_msg, 0, sizeof(out_msg));

            // get user command
            cmd = strtok(in_msg, ", \n");

            // get user command arguments
            if((args = strtok(NULL, "\n")) == NULL)
              args = "";

            DEBUG_PRINT(("[debug] cmd [args]: %s [%s]\n", cmd, args));

            // if the message sent is a Keep Alive Message, we don't
            // want the server to process it
            if(strcmp(cmd, kal_msg) == 0) {
              DEBUG_PRINT(("[debug] KAL message recieved\n"));
              clients[i-1].kal_misses = 0;
              break;
            }

            // record client timestamp
            time(&clients[i-1].timestamp);

            // first time connecting
            // add user to list of users
            if(strcmp(cmd, "open") == 0 && clients[i-1].connected != 1) {
              int x;
              int exists = 0;

              // check if username is taken by another client
              for(x=0;x<nclients;x++) {
                if(strcmp(args, clients[x].user) == 0) {
                  DEBUG_PRINT(("[debug] user '%s' already exists!\n", clients[x].user));

                  exists = 1;
                  close_conn = true;

                  sprintf(serv_msg, "[server] error: user '%s' already exists", args);
                }
              }

              if(exists == false) {
                DEBUG_PRINT(("[debug] server: adding user '%s' to clients\n", args));

                // connect user to clients[i-1]
                memset(clients[i-1].user, 0, sizeof(clients[i-1].user));
                strcpy(clients[i-1].user, args);

                clients[i-1].connected = 1;
                clients[i-1].descriptors[0] = fds[i].fd;

                // create message for client
                sprintf(serv_msg, "[server] User '%s' logged in", args);
              }
            }

            // copy message and send to client's chat session
            else if(strcmp(cmd, "<") == 0) {

              sprintf(out_msg, "\n[%s] %s", clients[i-1].user, args);

              // write to each recipient's out-fifo that is part of the chat session
              for(j=0;j<=clients[i-1].chatters;j++) {

                if(clients[j].connected == 0)
                  continue;

                DEBUG_PRINT(("[debug] server: writing out message '%s' to user %s (descriptor %d) \n", out_msg, clients[j].user, clients[j].descriptors[0]));

                if((nwrote = send(clients[i-1].descriptors[j], out_msg, strlen(out_msg), 0)) < 0) {
                  if(j==0)
                    sprintf(serv_msg, "[server] error: failed to echo message.");
                  else
                    sprintf(serv_msg, "[server] error: failed to send message to a chat user.");
                }
              if(strncmp(serv_msg, "[server] error:", 15) != 0)
                sprintf(serv_msg, "[server] done");
              }
            }

            // add users to senders
            else if(strcmp(cmd, "to") == 0) {

              // assert user is not trying to add itself as a recipient
              if(strcmp(args, clients[i-1].user) == 0)
                sprintf(serv_msg, "[server] error: you cannot add yourself as a recipient.");

              // attempt to find users to add to client's recipients
              else {
                int skip = 0;
                char connected_users[BUFMAX] = "";

                // assert that the recipients are connected users
                for(j=0;j<nclients;j++) {

                  DEBUG_PRINT(("[debug] server: checking '%s' for '%s'...\n", args, clients[j].user));

                  // if client is connected and recipeint list includes client username...
                  // add the client's return_fifos[0] to current sender's return_fifos
                  if(clients[j].connected == 1 && strstr(args, clients[j].user) != NULL) {
                    DEBUG_PRINT(("[debug] server: adding user %s's descriptor\n", clients[j].user));

                    int x;
                    for(x=0;x<=clients[i-1].chatters;x++) {
                      if(clients[i-1].descriptors[x] == clients[j].descriptors[0]) {
                        DEBUG_PRINT(("[debug] user %s is already part of the chat\n", clients[j].user));
                        sprintf(serv_msg, "[server] error: user %s is already connected to chat\n", clients[j].user);
                        skip = 1;
                        break;
                      }
                    }

                    if(skip == 1)
                      break;

                    // increment the number of users connected to the chat session
                    clients[i-1].chatters++;
                    clients[i-1].descriptors[clients[i-1].chatters] = clients[j].descriptors[0];

                    DEBUG_PRINT(("[debug] server: removing '%s' from args\n", clients[j].user));

                    // add user to list of connected users
                    strcat(connected_users, clients[j].user);
                    strcat(connected_users, " ");

                    // remove user from args. empty args should break the loop
                    remove_substring(args, clients[j].user);

                    DEBUG_PRINT(("[debug] args -> '%s'\n", args));
                    DEBUG_PRINT(("[debug] connected_users -> '%s'\n", connected_users));
                    DEBUG_PRINT(("[debug] %s's descriptors[%d] is now %d\n", clients[i-1].user, clients[i-1].chatters, clients[j].descriptors[0]));

                    if(strcmp(args, "") == 0)
                      break;
                  }
                }

                DEBUG_PRINT(("[debug] chatter is now '%d'\n", clients[i-1].chatters));

                if(skip != 1)
                  sprintf(serv_msg, "[server] recipients added: %s", connected_users);

                skip = 0;
              }
            }

            // print who is connected to client's session
            else if(strcmp(cmd, "who") == 0) {

              sprintf(serv_msg, "[server] Current users:");

              int x;
              char user_str[BUFMAX];
              // find the users connected to clients[i]'s session
              for(j=0;j<=clients[i-1].chatters;j++) {
                for(x=0;x<nclients;x++) { // for each client
                  // check if clients[x] is in clients[i]'s chat session
                  if(clients[i-1].descriptors[j] == clients[x].descriptors[0]) {
                    DEBUG_PRINT(("[debug] catting user %s to server message\n", clients[x].user));

                    // add clients[x] to server message
                    sprintf(user_str, " [%d] %s", j+1, clients[x].user);
                    strcat(serv_msg, user_str);
                    break;
                  }
                }
              }
            }

            // close the client's chat session
            else if((strcmp(cmd, "exit") == 0) || (strcmp(cmd, "close") ==0)) {

              DEBUG_PRINT(("[debug] server: removing user '%s' from clients\n", clients[i-1].user));

              destroy_client = true;
              close_conn = true;

              sprintf(serv_msg, "[server] done");

            }

            // unsupported command. do nothing
            else
              continue;

            // send server message back to client
            DEBUG_PRINT(("[debug] sending message '%s' back to client\n", serv_msg));

            if((nwrote = send(fds[i].fd, serv_msg, strlen(serv_msg), 0)) < 0) {
              perror("[error] send failed");
              break;
            }

            break;

          } while(1);

          // remove allocated client assets
          if(destroy_client) {
            // update clients who have the removed user as a recipient
            int x;
            for(j=0;j<nclients;j++) { // clients
              for(x=0;x<clients[j].chatters;x++) { // descriptors
                if(clients[j].descriptors[x] == clients[i-1].descriptors[0] && clients[j].connected == 1) {
                  clients[j].descriptors[x] = -1;
                  clients[j].chatters--;
                }
              }
            }

            for(x=0;x<clients[i-1].chatters;i++)
              clients[i-1].descriptors[x] = -1;
            clients[i - 1].chatters = 0;
            clients[i - 1].kal_misses = 0;
            clients[i - 1].connected = 0;
            strcpy(clients[i-1].user, "");

            DEBUG_PRINT(("[debug] clients[%d] destroyed.\n", i-1));
          }

          // clean up connection iff close_conn flag is set
          if(close_conn) {
            close(fds[i].fd);
            fds[i].fd = -1;
            shrink_polls = true;
          }

        } // existing cnnection

      } // for i in pollable sockets

      // shirink poll struct
      if(shrink_polls) {
        DEBUG_PRINT(("[debug] shrinking polls\n"));
        shrink_polls = false;
        for(i=0;i<nlisteners;i++) {
          if(fds[i].fd == -1) {
            for(j=i;j<nlisteners;j++) {
              fds[j].fd = fds[j+1].fd;
              if(i != 0 ) {
                if(j == nclients) {
                  int n;
                  for(n=0;n<=clients[j-1].chatters;n++)
                    clients[j-1].descriptors[n] = -1;
                  clients[j-1].chatters = 0;
                  clients[j-1].kal_misses = 0;
                  clients[j-1].connected = 0;
                  strcpy(clients[j-1].user, "");
                }
                else
                  clients[j-1] = clients[j];
              }
            }
            nlisteners--;
            DEBUG_PRINT(("[debug] nlisteners is now %d\n", nlisteners));
          }
        }
      }

    } while(server_up == true);

    // clean up open sockets
    for(i=0;i<nlisteners;i++) {
      if(fds[i].fd >= 0)
        close(fds[i].fd);
    }
    return 0;
  } // parent proc
}

/// Client Chat Session protocol
/// Spawns a child process for reading from sockfd
int client_chat(const char* username, int sockfd) {
  int status;
  bool kal_sent;
  int nread, nwrote;
  char cmd_buf[BUFMAX], read_buf[BUFMAX], write_buf[BUFMAX];
  char kal_msg[BUFMAX];
  char* cmd;
  char* msg;

  // create the keep alive message
  get_kal_msg(kal_msg, sizeof(kal_msg));

  // fork a child process for writing to the server
  // the parent process will read from the server
  pid_t pid = fork();

  if(pid < 0) {
    printf("fork error. exiting...\n\n");
    return -1;
  }

  if(pid == 0) { // write process

    while(1) {
      usleep(100000);

      memset(cmd_buf, 0, sizeof(cmd_buf));

      // user prompt
      printf("a3chat_client: ");
      fflush(stdout);

      // get user input
      if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL) {
        printf("\nconnection with server lost. exiting...\n");
        fflush(stdout);
        usleep(100000);
        _Exit(0);
      }
      else if(cmd_buf[0] == '\n')
        continue;

      // extract user command
      cmd = strtok(cmd_buf, ", \n");

      // attempt to open a chat session for this client
      if(strcmp(cmd,"open") == 0) {
        printf("chat session already opened\n");
        continue;
      }

      if(strcmp(cmd, "<") == 0 || strcmp(cmd, "to") == 0 || strcmp(cmd, "who") == 0) {

        strcpy(write_buf, cmd);

        // determine if there is a message to write
        if((msg = strtok(NULL, "\n")) != NULL) {
          strcat(write_buf, " ");
          strcat(write_buf, msg);
        }

        DEBUG_PRINT(("[debug] chat sesh: writing '%s' to descriptor %d\n", write_buf, sockfd));

        if((nwrote = write(sockfd, write_buf, strlen(write_buf))) != strlen(write_buf)) {
          perror("[error] write failed");
          continue;
        }
      }

      else if(strcmp(cmd, "close") == 0) {
        // inform the server that we're closing
        DEBUG_PRINT(("[debug] writing %s to socket\n", cmd));
        if((nwrote = write(sockfd, cmd, strlen(cmd))) != strlen(cmd)) {
          perror("[error] write failed");
          continue;
        }
        usleep(100000);
        _Exit(0);
      }

      else if(strcmp(cmd,"exit") == 0) {
        // tell server we're exiting and close socket
        DEBUG_PRINT(("[debug] writing %s to socket\n", cmd));
        if((nwrote = write(sockfd, cmd, strlen(cmd))) != strlen(cmd)) {
          perror("[error] write failed");
          continue;
        }
        // terminate
        printf("exiting...\n");
        _Exit(1);
      }

    } // while(1)
  } // child proc

  else {
    clock_t start = clock();

    while(1) {

      if(waitpid(pid, &status, WNOHANG) == pid) {
        DEBUG_PRINT(("[debug] exiting read process...\n"));
        DEBUG_PRINT(("[debug] exit status was %d\n", WEXITSTATUS(status)));

        close(sockfd);

        // return 1 if we are to terminate the client
        return WEXITSTATUS(status);
      }

      clock_t current = clock();
      double elapsed = ((double)current - (double)start)/(double)CLOCKS_PER_SEC;

      // send keep alive message if KAL_interval time has elapsed
      if(fmod((double)elapsed, (double)KAL_interval) == 0) {
        if(kal_sent == false) {
          kal_sent = true;
          send(sockfd, kal_msg, strlen(kal_msg), 0);
        }
      }
      else
        kal_sent = false;

      memset(read_buf, 0, sizeof(read_buf));
      if((nread = read(sockfd, read_buf, sizeof(read_buf))) == -1) {
        continue;
      }

      // nothing to write
      else if(nread == 0) {
        continue;
      }

      // echo incoming message to stdout
      else {
        DEBUG_PRINT(("[debug] client read proc: read from server!\n"));
        printf("%s\n", read_buf);

        // if the message is not from this user or the server
        // re-display the user prompt
        char buf[BUFMAX] = "\n[";
        strcat(buf, username);
        if((strncmp(read_buf, "[server]", 8) != 0) && (strncmp(read_buf, buf, strlen(buf)) != 0)) {
          printf("a3chat_client: ");
          fflush(stdout);
        }
      }
    } // while(1)
  } // parent
  return 0;
}


/// Client process handling
int begin_client(const char* portnumber, const char* server_addr) {
  int on = 1;
  int nread, nwrote, ntries;
  int sockfd, s;
  bool close_conn = false;
  char cmd_buf[BUFMAX], write_buf[BUFMAX], read_buf[BUFMAX];
  char* username;
  char* cmd;

  struct hostent* lh;
  struct in_addr **addr_list;
  struct sockaddr_in serv;

  DEBUG_PRINT(("[debug] server address input: %s\n", server_addr));

  // resolve server_addr to get server IP
  if((lh = gethostbyname(server_addr)) == NULL) {
    printf("Invalid server address.\n");
    return -1;
  } // IP = lh.h_addr

  addr_list = (struct in_addr** )lh->h_addr_list;

  printf("lh->h_addr: %s\n", inet_ntoa(*addr_list[0]));

  memset(&serv, 0, sizeof(serv));

  // initialize server params
  serv.sin_family = AF_INET;
  serv.sin_port = htons(atoi(portnumber));

  // convert server IP to binary representation
  s = inet_pton(AF_INET, inet_ntoa(*addr_list[0]), &(serv.sin_addr));
  if(s <= 0) {
    if(s == 0)
      fprintf(stderr, "server address in incorrect format\n");
    else
      perror("inet_pton");
    return -1;
  }

  DEBUG_PRINT(("[debug] calling socket()...\n"));

  printf("Chat client begins (server '%s' [%s], port %s)\n", lh->h_name, inet_ntoa(*addr_list[0]), portnumber);

  while(1) {
    usleep(100000);

    memset(cmd_buf, 0, sizeof(cmd_buf));

    // user prompt
    printf("a3chat_client: ");
    fflush(stdout);

    // get user input
    if(fgets(cmd_buf, sizeof(cmd_buf), stdin) == NULL || cmd_buf[0] == '\n')
      continue;

    // extract user command
    cmd = strtok(cmd_buf, ", \n");

    // attempt to open a chat session for this client
    if(strcmp(cmd,"open") == 0) {

      // get socket fd and attempt connection
      sockfd = socket(AF_INET, SOCK_STREAM, 0);

      // connect with server's accept
      if(connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) != 0) {
        perror("[error] connect");
        continue;
      }

      if(ioctl(sockfd, FIONBIO, (char *)&on) < 0) {
        perror("[error] ioctl");
        close(sockfd);
        return -1;
      }

      // if username not specified, give the user a default username
      if((username = strtok(NULL, "")) == NULL) {
        printf("please provide a username\n");
        continue;
      }

      // create message to send to server
      memset(write_buf, 0, sizeof(write_buf));
      strtok(username, "\n");
      strcpy(write_buf, cmd);
      strcat(write_buf, " ");
      strcat(write_buf, username);

      DEBUG_PRINT(("[debug] writing '%s' to socket\n", write_buf));

      // write to server
      if((nwrote = write(sockfd, write_buf, strlen(write_buf))) != strlen(write_buf)) {
        printf("write error. could not connect to server. please try opening again...\n");
        continue;
      }

      DEBUG_PRINT(("[debug] reading from socket\n"));

      // read server message
      nread = 0;
      ntries = 0;
      close_conn = false;
      do {
        if((nread = recv(sockfd, read_buf, sizeof(read_buf), 0)) < 0) {
          if(errno != EWOULDBLOCK)  {
            perror("[error] recv()");
            close(sockfd);
            close_conn = true;
            break;
          }
          if(ntries > 10) {
            printf("unable to connect with server. please try again later.\n");
            close(sockfd);
            close_conn = true;
            break;
          }
          ntries++;
          usleep(100000);
        }
      } while(nread <= 0);

      if(close_conn == true)
        continue;

      // dump server response
      printf("%s\n", read_buf);

      // check server response for warnings/errors
      if(strncmp(read_buf, "[server] error:", 15) == 0) {
        DEBUG_PRINT(("[debug] server error. breaking\n"));
        close(sockfd);
        continue;
      }

      printf("\n***chat session opened***\n");
      printf("     welcome %s\n\n", username);

      int ret;
      ret = client_chat(username, sockfd);

      printf("\n       goodbye %s\n", username);
      printf("***chat session closed***\n\n");

      if(ret == 0) {
       continue;

      }
      else if(ret == 1) {
        printf("exiting...\n");
        return 0;
      }
      else
        return -1;
    }

    if(strcmp(cmd, "<") == 0 || strcmp(cmd, "to") == 0 || strcmp(cmd, "who") == 0) {
      printf("You are not connected to a chat session!\n");
    }

    else if(strcmp(cmd, "close") == 0) {
      printf("No session to close\n");
    }

    else if(strcmp(cmd,"exit") == 0) {
      // terminate
      printf("exiting...\n");
      return 0;
    }

  } // while(1)

  return 0;
}

/// Processes input arguments
int main(int argc, const char* argv[]) {
  const char* portnumber = argv[2];

  // check for at least 2 args, max 4 args
  if(argc < 2 || argc > 4)
    return usage();

  // set a limit on CPU time (e.g. 10 minutes)
  struct rlimit cpu_lim = {600, 600};
  if(setrlimit(RLIMIT_CPU, &cpu_lim) == -1) {
    perror("setrlimit: failed\nError ");
    return -1;
   }

  if((strcmp(argv[1], "-s") == 0) && argc == 4) { // server
    int nclient;

    // check if num of clients is valid
    if((isdigit(atoi(argv[3])) == -1) || (nclient = atoi(argv[3])) > NMAX) {
      printf("Error: invalid nclient\n");
      return usage();
    }
    else
      return begin_server(portnumber, nclient);
  }

  else if((strcmp(argv[1], "-c") == 0) && argc == 4) // client
  {
    const char* server_addr = argv[3];
    return begin_client(portnumber, server_addr);
  }

  else
    return usage();

  return 0;
}
