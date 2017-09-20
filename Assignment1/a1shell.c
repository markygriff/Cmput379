#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>

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
        float one, fifteen, five;
        char procs[5];

        loadavg = fopen("/proc/loadavg", "r");
        fscanf(loadavg, "%f %f %f %s", &one, &five, &fifteen, procs);
        fclose(loadavg);

        printf("-a1monitor: >>>>>>>>>>>\n");
        system("date");
        printf("Load average:  %.2f %.2f %.2f\nProcesses:  %s\n", one, five, fifteen, procs);
        printf("<<<<<<<<<<\n\n");
        sleep(interval);

        // check if parent has terminated
        pid_t done = waitpid(ppid, &status, 0);
        if(done == ppid) {
          printf("-a1monitor: parent terminated?\n");
          return 0;
        }
        else {
          printf("-a1monitor: waitpid error\n");
        }
      }
      return 0;
    }
    else { // a1shell process
      // allow time for child print to stdout
      sleep(1);
      char cmd[1024];
      times();
      while(1) {
        printf("a1shell%% ");
        scanf("%s", cmd);

        if(strcmp(cmd, "cd") == 0) {
          char path[1024];
          scanf("%s", path);
          if (chdir(path) != 0) {
            printf("-a1shell: cd: %s: No such directory\n", path);
          }
        }
        else if(strcmp(cmd, "pwd") == 0) {
          char pwd[1024];
          if(getcwd(pwd, sizeof(pwd)) != NULL) {
            printf("%s\n", pwd);
          }
          else {
            printf("-a1shell: getcwd failed\n");
          }
        }
        else if(strcmp(cmd, "unmask") == 0) {
          printf("UNMASK\n");
        }
        else if(strcmp(cmd, "done") == 0) {
          printf("exiting a1shell...\n");
          break;
        }
        else {
          printf("-a1shell: Operation '%s' not permitted\n", cmd);
        }
      } //while

      printf("-a1shell: preparing to wait for child");
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
