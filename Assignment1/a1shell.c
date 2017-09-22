#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/* #include <sys/wait.h> */
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

/* char get_pwd(char pwd) { */
/*   if(getcwd(pwd, sizeof(pwd)) != NULL) { */
/*     printf("%s\n", pwd); */
/*     return pwd; */
/*   } */
/*   else { */
/*     printf("-a1shell: getcwd failed\n"); */
/*     return NULL; */
/*   } */
/* } */

int main(int argc, char* argv[]) {
  if (argc == 1) {
    usage();
    return 1;
  }
  else if(argc > 1) {
    int interval = atoi(argv[1]);
    printf("interval: %d\n",interval);

    // 1. use setrlimit to set a limit on CPU time (e.g. 10 minutes)
    // ???

    // 2. Fork a child process (a1monitor)
    int status;
    pid_t pid;
    pid = fork();
    if (pid == 0) { // a1monitor
      pid_t ppid = getpid();
      printf("child: parent pid is %d\n", ppid);
      /* chdir("/proc"); */
      // run in a loop until the parent terminates
      while(1) {
        // display on the stdout
        // a) The current date and time
        // b) Average system loads
        // c) Number of running processes and total number of processes
        FILE* loadavg;
        static float one, fifteen, five;
        static char procs[5];

        loadavg = fopen("/proc/loadavg", "r");
        fscanf(loadavg, "%f %f %f %s", &one, &five, &fifteen, procs);
        fclose(loadavg);

        printf("\n-a1monitor: >>>>>>>>>>>>>>>>\n");
        system("date");
        printf("Load average:  %.2f %.2f %.2f\nProcesses:  %s\n", one, five, fifteen, procs);
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        sleep(interval);

        // check if parent has terminated
        pid_t done = waitpid(ppid, &status, 0);
        if(done == ppid) {
          printf("\n-a1monitor: parent(a1shell) terminated?\n");
          break;
        }
        else {
          printf("\n-a1monitor: waitpid error\n");
        }
      }
      return 0;
    }
    else { // a1shell process
      // allow time for a1monitor to print to stdout
      sleep(1);
      static char cmd[1024];
      cmd[0] = '\0';

      while(1) {
        // shell prompt for the user
        printf("a1shell%% ");
        scanf("%s", cmd);

        if(strcmp(cmd, "cd") == 0) {
          static char path[1024];
          path[0] = '\0';

          scanf("%s", path);
          if(path[0] == '$') {
            char* complete_path = NULL;
            static char env_var[1024];
            env_var[0] = '\0';
            int i = 0;

            // remove '$' from the path string
            memmove(path, path+1, strlen(path));

            while(i < strlen(path)) {
              // check for end of $VAR in path string
              if(path[i] == '/')
                break;
              i++;
            }

            /* printf("-a1shell: about to copy path = '%s' from 0 to %d into\ */
              /* env_var = '%s'\n", path, i, env_var); */

            // copy $VAR into buffer
            strncpy(env_var, path, i);
            env_var[i] = '\0';
            /* printf("-a1shell: copied buffer: %s\n", env_var); */
            // remove $VAR from path string
            memmove(path, path+i, strlen(path));
            /* printf("-a1shell: new path: %s\n", path); */
            // expand env var
            complete_path = getenv(env_var);
            /* printf("-a1shell: $VAR: %s\n", complete_path); */

            if(complete_path == NULL) {
              printf("-a1shell: cd: $%s: no such directory\n", env_var);
              continue;
            }
            else {
              // get the full path with expanded env var
              strcat(complete_path, path);
              /* printf("a1shell: complete_path: %s\n", complete_path); */
              if (chdir(complete_path) != 0)
                printf("-a1shell: cd: %s: No such directory\n", path);
            }
            complete_path = NULL;
            env_var[0] = '\0';
          }
          // attempt to change directory
          else if (chdir(path) != 0)
            printf("-a1shell: cd: %s: No such directory\n", path);
        }

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
          printf("-a1shell: mask: %o\n", mask);
          printf("-a1shell: S_IRWXU: ");
          printf("-a1shell: S_IRWXG: ");
          printf("-a1shell: S_IRWXO: ");
        }
        else if(strcmp(cmd, "done") == 0) {
          printf("exiting a1shell...\n");
          break;
        }
        else { // attempt to execute command using /bin/bash
          char args[1024];
          char cmd_with_args[1024];

          fgets(args, sizeof(args), stdin);
          strcpy(cmd_with_args, cmd);
          strcat(cmd_with_args, args);
          /* printf("-a1shell: args: %s\n", args); */
          /* printf("-a1shell: cmd with args: %s\n", cmd_with_args); */

          pid_t pid2;

          // record user and CPU times for the current process
          // NOTE: we are assuming the time taken for the process is
          //       within the range of an int type
          static struct tms st_buf;
          static struct tms end_buf;
          static clock_t start_time;
          static clock_t end_time;

          // start clock
          start_time = times(&st_buf);
          /* printf("-a1shell: start time: %jd\n", start_time); */
          /* printf("-a1shell: user time: %jd\n", st_buf.tms_utime); */
          /* printf("-a1shell: cpu time: %jd\n", st_buf.tms_stime); */
          /* printf("-a1shell: child user time: %jd\n", st_buf.tms_cutime); */
          /* printf("-a1shell: child cpu time time: %jd\n", st_buf.tms_cstime); */

          // begin new process to exec cmd arg1 arg2 ...
          pid2 = fork();
          if(pid2 == 0) { // execl process
            execl("/bin/bash", "bash", "-c", cmd_with_args, (char*) 0);
            // execl only returns on failure
            perror("-execl: failed");
          }
          else { // a1shell process
            waitpid(pid2);

            // end clock
            end_time = times(&end_buf);
            printf("-a1shell: execl process terminated?\n");

            /* printf("-a1shell: end time: %jd\n", end_time); */
            /* printf("-a1shell: user time: %jd\n", end_buf.tms_utime); */
            /* printf("-a1shell: cpu time: %jd\n", end_buf.tms_stime); */
            /* printf("-a1shell: child user time: %jd\n", end_buf.tms_cutime); */
            /* printf("-a1shell: child cpu time time: %jd\n", end_buf.tms_cstime); */

            printf("-a1shell: total user time: %jd\n",\
                end_buf.tms_utime - st_buf.tms_utime);
            printf("-a1shell: total cpu time: %jd\n",\
                end_buf.tms_stime - st_buf.tms_stime);
            printf("-a1shell: total user time: %jd\n",\
                end_buf.tms_cutime - st_buf.tms_cutime);
            printf("-a1shell: total child cpu  time: %jd\n",\
                end_buf.tms_cstime - st_buf.tms_cstime);
            printf("-a1shell: total total real time: %jd\n",\
                end_time - start_time);

            //temp
            continue;
          }
        }
      } //while

      printf("-a1shell: waiting for child to terminate...");
      // wait for child to terminate
      if(pid == wait(&status)) {
        printf("-a1shell: child terminated?\n");
        return 0;
      }
      else {
        printf("-a1shell: wait error\n");
        return 0;
      }
    }
  }
  return 0;
}
