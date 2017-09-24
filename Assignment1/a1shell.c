#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

// Mark Griffith 1422270

// run from command line using "a1shell interval"
// where interval is the number of seconds

void usage() {
  printf("usage: a1shell interval\n");
}

int main(int argc, char* argv[]) {
  // user must provide interval. No default.
  if (argc != 2) {
    usage();
    return 1;
  }
  else {
    int interval = atoi(argv[1]);
    struct rlimit cpu_lim = {600, 600}; // cpu time limit in seconds

    // set a limit on CPU time (e.g. 10 minutes)
    if(setrlimit(RLIMIT_CPU, &cpu_lim) == -1) {
      perror("setrlimit: failed");
      return 1;
     }

    // fork a child process (a1monitor)
    int status;
    pid_t pid;
    pid = fork();

    if (pid == 0) { // a1monitor
      FILE* loadavg;
      static float one, fifteen, five;
      static char procs[5];
      pid_t ppid = getppid(); // a1shell pid
      pid_t ppid_curr = ppid;

      while(ppid_curr == ppid) {
        // displays the time and date, load average, and number of running
        // processes on stdout
        loadavg = fopen("/proc/loadavg", "r");
        fscanf(loadavg, "%f %f %f %s", &one, &five, &fifteen, procs);
        fclose(loadavg);
        printf("\n-a1monitor: >>>>>>>>>>>>>>>>\n");
        system("date");
        printf("Load average:  %.2f %.2f %.2f\nProcesses:  %s\n", one, five, fifteen, procs);
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        
        // sleep for interval specified by user input
        sleep(interval);
        ppid_curr = getppid(); // 1 if parent is terminated
      }
      _Exit(EXIT_SUCCESS);
    } // a1monitor
    else { // a1shell process
      static char cmd[1024];
      cmd[0] = '\0';

      // allow time for a1monitor to print to stdout
      sleep(1);

      while(1) {
        // shell prompt for the user
        printf("a1shell%% ");
        scanf("%s", cmd);
        
        /// Change directory functionality
        if(strcmp(cmd, "cd") == 0) {
          static char path[1024];
          path[0] = '\0';

          scanf("%s", path);
          // check if the path begins with an environment var
          if(path[0] == '$') {
            char* complete_path = NULL;
            static char env_var[1024];
            env_var[0] = '\0';
            int i = 0;

            // remove '$' from the path string
            memmove(path, path+1, strlen(path));
            // check for end of $VAR in path string
            while(i < strlen(path)) {
              if(path[i] == '/')
                break;
              i++;
            }
            // copy $VAR into buffer
            strncpy(env_var, path, i);
            env_var[i] = '\0'; // strncpy does not \0 terminate
            // remove $VAR from path string
            memmove(path, path+i, strlen(path));
            // expand environment var and check if it exists
            complete_path = getenv(env_var);
            if(complete_path == NULL) {
              printf("-a1shell: cd: $%s: no such directory\n");
              continue;
            }
            // get the full path with the expanded environment var
            else {
              strcat(complete_path, path);
              if (chdir(complete_path) != 0)
                printf("-a1shell: cd: %s: No such directory\n", path);
            }
            complete_path = NULL;
            env_var[0] = '\0';
          }
          else if (chdir(path) != 0)
            printf("-a1shell: cd: %s: No such directory\n", path);
        }
        /// Print Working Directory functionality
        else if(strcmp(cmd, "pwd") == 0) {
          char pwd[1024];

          if(getcwd(pwd, sizeof(pwd)) != NULL) {
            printf("%s\n", pwd);
          }
          else {
            perror("-a1shell: getcwd: failed\n");
          }
        }
        else if(strcmp(cmd, "umask") == 0) {
          mode_t mask = umask(0);
          printf("-a1shell: current mask: %04o\n", mask);
          printf("-a1shell: rwx user (S_IRWXU): %04o\n", S_IRWXU);
          printf("-a1shell: rwx group (S_IRWXG): %04o\n", S_IRWXG);
          printf("-a1shell: rwx general (S_IRWXO): %04o\n", S_IRWXO);
        }
        /// Exit a1shell functionality
        else if(strcmp(cmd, "done") == 0) {
          /* wait(&status); */

          /* printf("-a1shell: exiting...\n"); */
          /* pid = waitpid(pid, &status, WNOHANG); */
          /* while(pid == 0) { */
          /*   pid = waitpid(pid, &status, WNOHANG); */
          /*   if(pid == -1) { */
          /*     perror("-a1shell: waitpid on a1monitor failed"); */
          /*     _exit(EXIT_FAILURE); */
          /*   } */
          /*   else if(pid == 0) // a1monitor is still running */
          /*   { */
          /*     printf("-a1shell: still waiting for a1monitor to terminate...\n"); */
          /*     sleep(1); */
          /*   } */
          /* } */
          /* printf("-a1shell: a1monitor process terminated?"); */
          _exit(EXIT_SUCCESS);
        }
        /// Bash Command Execution functionality
        else { 
          char args[1024];
          char cmd_with_args[1024];
          static struct tms st_buf;
          static struct tms end_buf;
          static clock_t start_time;
          static clock_t end_time;
          pid_t pid2;

          fgets(args, sizeof(args), stdin);
          strcpy(cmd_with_args, cmd);
          strcat(cmd_with_args, args);

          // start clock
          start_time = times(&st_buf);

          // begin new process to exec cmd arg1 arg2 ...
          pid2 = fork();
          if(pid2 == 0) { // execl process
            execl("/bin/bash", "bash", "-c", cmd_with_args, (char*) 0); {
              // execl only returns on failure
              perror("-execl: failed");
              _exit(EXIT_FAILURE);
            }
          }
          else { // a1shell process
            // wait for execl to terminate
            pid2 = waitpid(pid2, &status, WNOHANG);
            while(pid2 == 0) {
              pid2 = waitpid(pid2, &status, WNOHANG);
              if(pid2 == -1)
                perror("-a1shell: waitpid on execl failed");
              else if(pid2 == 0) // child is still running
                sleep(1);
            }

            // record user and CPU times for the current process
            // NOTE: we are assuming the time taken for the process is
            //       within the range of an int type
            end_time = times(&end_buf);

            printf("\n-a1shell: >>>>>>>>>>>>>>>>\n");
            printf("total real time: %jd\n",\
                end_time - start_time);
            printf("total user time: %jd\n",\
                end_buf.tms_utime - st_buf.tms_utime);
            printf("total cpu time: %jd\n",\
                end_buf.tms_stime - st_buf.tms_stime);
            printf("execl: total user time: %jd\n",\
                end_buf.tms_cutime - st_buf.tms_cutime);
            printf("execl: total cpu  time: %jd\n",\
                end_buf.tms_cstime - st_buf.tms_cstime);
            printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
          }
        }
      } //while
    } // a1shell process
  }
}
