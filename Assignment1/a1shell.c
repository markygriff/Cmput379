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

int usage() {
  printf("usage: a1shell interval\n");
  return -1;
}


/// Print Working Directory functionality
int print_dir(char* pwd, long size) {
  if(getcwd(pwd, size) != NULL)
    printf("%s\n", pwd);
  else
    return -1;
  return 0;
}

/// Umask functionality
void print_mask() {
  mode_t mask = umask(0); // always suceeds
  printf("-a1shell: current mask: %04o\n", mask);
  printf("-a1shell: rwx user (S_IRWXU): %04o\n", S_IRWXU);
  printf("-a1shell: rwx group (S_IRWXG): %04o\n", S_IRWXG);
  printf("-a1shell: rwx general (S_IRWXO): %04o\n", S_IRWXO);
}

/// Bash Command Execution functionality
int execute_bash(char* cmd) {
  char args[1024];
  char cmd_with_args[1024];
  static struct tms st_buf;
  static struct tms end_buf;
  static clock_t start_time;
  static clock_t end_time;
  int status;
  pid_t pid2;
  fgets(args, sizeof(args), stdin);
  strcpy(cmd_with_args, cmd);
  strcat(cmd_with_args, args);
  // start clock
  start_time = times(&st_buf);
  // begin new process to exec cmd arg1 arg2 ...
  pid2 = fork();
  if(pid2 == 0) { // execl process
    execl("/bin/bash", "bash", "-c", cmd_with_args, (char*) 0);
    // execl only returns on failure
    perror("-execl: failed");
    _exit(EXIT_FAILURE);
  }
  else { // a1shell process
    // wait for execl (child process) to terminate
    pid2 = waitpid(pid2, &status, WNOHANG);
    while(pid2 == 0) {
      pid2 = waitpid(pid2, &status, WNOHANG);
      if(pid2 == -1)
        perror("-a1shell: waitpid on execl failed");
    }
    // record and print user and CPU times for the current process
    // NOTE: we are assuming the time taken for the process is
    //       within the range of an int type
    end_time = times(&end_buf);
    printf("\n-a1shell: >>>>>>>>>>>>>>>>>>\n");
    printf("total real time: %jd\n",\
        end_time - start_time);
    // print parent (a1shell) times
    printf("total user time: %jd\n",\
        end_buf.tms_utime - st_buf.tms_utime);
    printf("total cpu time: %jd\n",\
        end_buf.tms_stime - st_buf.tms_stime);
    // print child (execl) times
    printf("execl: total user time: %jd\n",\
        end_buf.tms_cutime - st_buf.tms_cutime);
    printf("execl: total cpu time: %jd\n",\
        end_buf.tms_cstime - st_buf.tms_cstime);
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
  }
  return 0;
}

/// Change directory functionality
int change_dir(char* path) {
  scanf("%s", path);
  // check if the path begins with an environment var
  if(path[0] == '$') {
    char* env_var_p; // pointer to environment_variable
    static char env_var[1024];
    static char expanded_path[1024];
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
    // NOTE: getenv returns a pointer to a list of defined environment vars
    //       so DO NOT MODIFY env_var_p
    env_var_p = getenv(env_var);
    if(env_var_p == NULL)
      return -1;
    else {
      // expand the full path
      strcpy(expanded_path, env_var_p);
      strcat(expanded_path, path);
      if (chdir(expanded_path) != 0)
        return -1;
    }
    env_var[0] = '\0';
  }
  // if path does not start with environment var, change into it
  else if (chdir(path) != 0)
    return -1;
  return 0;
}

/// Command Parser
void parse(char* cmd) {
  if(strcmp(cmd, "cd") == 0) {
    static char path[1024];
    path[0] = '\0';
    if(change_dir(path) == -1)
      printf("-a1shell: cd: %s: no such directory\n", path);
  }
  else if(strcmp(cmd, "pwd") == 0) {
    static char pwd[1024];
    if(print_dir(pwd, sizeof(pwd)) == -1)
      perror("-a1shell: getcwd: failed\nError ");
  }
  else if(strcmp(cmd, "umask") == 0)
    print_mask();
  else if(strcmp(cmd, "done") == 0)
    _exit(EXIT_SUCCESS);
  else
    execute_bash(cmd);
}

int main(int argc, char* argv[]) {
  // user must provide interval. No default.
  if (argc != 2)
    return usage();
  else {
    int interval = atoi(argv[1]);
    struct rlimit cpu_lim = {600, 600}; // cpu time limit in seconds
    // set a limit on CPU time (e.g. 10 minutes)
    if(setrlimit(RLIMIT_CPU, &cpu_lim) == -1) {
      perror("setrlimit: failed");
      return -1;
     }
    // fork a child process (a1monitor)
    pid_t pid;
    pid = fork();
    if (pid == 0) { // a1monitor
      FILE* loadavg;
      static float one, fifteen, five;
      static char procs[5];
      pid_t ppid = getppid(); // a1shell pid
      pid_t ppid_curr = ppid;
      // NOTE: the condition on this 'while' is that the a1monitor's ppid changes.
      // This is because the a1monitor process is to terminate after it's parent
      // process (a1shell) has terminated, therefore making it an orphan with ppid=1
      while(ppid_curr == ppid) {
        // better to check if parent is terminated before sleeping to avoid
        // hanging the process
        ppid_curr = getppid(); // 1 if parent is terminated
        if(ppid_curr != ppid) _Exit(EXIT_SUCCESS);
        // displays the time and date, load average, and number of running
        // processes on stdout
        loadavg = fopen("/proc/loadavg", "r");
        fscanf(loadavg, "%f %f %f %s", &one, &five, &fifteen, procs);
        fclose(loadavg);
        printf("\n-a1monitor: >>>>>>>>>>>>>>>>\n");
        system("date");
        printf("Load average:  %.2f %.2f %.2f\nProcesses:  %s\n", one, five, fifteen, procs);
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n");
        // sleep for interval specified by user input and chek ppid
        sleep(interval);
      }
      // exit a1monitor process successfully
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
        // formate user input
        scanf("%s", cmd);
        // parse user command
        parse(cmd);
      } // while
    } // a1shell process
  }
}
