#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>

// Mark Griffith 1422270

// run from command line using "a1shell interval"
// where interval is the number of seconds

int main(int argc, char* argv[]) {

  int interval = atoi(argv[1]);
  printf("interval: %d\n",interval);
  // 1. use setrlimit to set a limit on CPU time (e.g. 10 minutes)

  // 2. Fork a child process (a1monitor)
  int ppid;
  ppid = fork();

  if (ppid == 0) { // a1monitor
    printf("child: I am the a1monitor process\n");

    // run in a loop until the parent terminates
    while(1) {
      // display on the stdout
      // a) The current date and time
      // b) Average system loads
      // c) Number of running processes and total number of processes
      FILE* file;
      char* line = NULL;
      size_t read;
      size_t len = 0;
      char** info;
      int i = 0;

      info = malloc(1 * sizeof(char*));
      file = fopen("/proc/loadavg", "r");
      while ((read = getline(&line, &len, file)) != -1) {
        info[i] = strdup(line);
        i++;
      }
      info[i] = 0;
      fclose(file);

/*       i = 0; */
/*       while (info[i]) */
/*       { */
/*         printf("%s", info[i]); */
/*         free (info[i]); */
/*         i++; */
/*       } */

      printf("Load average: ");
      for (int j=0; j<=14; j++) {
        printf("%s",info[0][j]);
      }
      printf("balh\n");

      printf("Processes:");
      for (i=16; i<=20; i++) {
        printf("%s",info[i]);
      }
      printf("\n");

      free (info);
      printf("return");
      return 0;

      /* printf("a1monitor: %s\n        Load average: %s\n        Processes: %s\n, something, something, something); */
      printf("sleep...");
      sleep(interval);
    }
  }

  // run parent process (a1shell)
  else { // a1shell process
    printf("parent: I am the a1shell process\n");
    sleep(10);
    return 0;
  }

  return 0;
}
