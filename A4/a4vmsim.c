/* Mark Griffith - 1422270 */

#include <pfault.h>
#include <statistics.h>
#include <ptable.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/resource.h>

// initialize global options
opts_t opts;
int references = 0;

void init();
void simulate();

int usage() {
  printf("usage: a4vmsim pagesize memsize strategy [none, mrand, lru, sec]\n");
  Exit(1);
}

uint log_2(uint x) {
  int i, log = -1;

  for (i=0; i<(8*sizeof(x)); i++) {
    if (x & (1 << i)) {
      /* we only want perfect powers of 2 */
      if (log != -1)
	return -1;
      log = i;
    }
  }
  return log;
}

uint pow_2(uint pow) {
  return 1 << pow;
}

ref_op_t get_op_type(char c) {
	if (c[0] & 0b10000000)
    return REF_WRITE;

	if (c[0] & 0b11000000)
    return REF_READ;

	return REF_NULL;
}

void init() {
  init_ptable();
  init_mem();
}

void simulate() {
  // the generated reference string is 32 bits = 4 bytes

  /***********/
  uint cnt = 0;
  uint virtual_addr;
  char ref_str;
  ref_op_t operation;
  pte_t* pte;
  fault_handler_t handler;

  stats->accumulator = 0;
  handler = opts.fault_handler->handler;

  printf("[a4vmsim] [page = %d, mem = %d, %s]\n",
        opts.pagesize, opts.memsize, opts.handler->strategy);

  printf("vaddr (Virtual Address) has %d bits, consisting of higher %d bits for vfn (Virtual Frame Number), and lower %d bits for offset within each page (log_2(pagesize=%d))\n",
	addr_bits, vfn_bits, log_2(opts.pagesize), opts.pagesize);

  while(read(0, &ref_str, 4)) {
    operation = get_op_type(ref_str);
    inc_references();
    cnt++;

    // print dots because it's cool
    if ((cnt % 100) == 0) {
      printf(".");
      fflush(stdout);
      if ((count % (64 * 100)) == 0) {
        printf("\n");
        fflush(stdout);
      }
    }

    pte = ptable_lookup_vaddr(vaddr_to_vfn(vaddr), operation);

    // check for page fault
    if(!pte->valid) {
      inc_faults();
      // handle the page fault
      handler(pte, operation);
    }

    if(pte->valid) {
      pte->chance = 1; // for sec

      if(pte->modified)
        inc_flushes();
    }

    pte->reference = 1; // set refence bit
    pte->counter = references++;

    if(operation == REF_WRITE)
      pte->modified = 1;
  }
  /***********/

}

/// Processes input arguments
int main(int argc, const char* argv[]) {

  // check for at least 2 args, max 4 args
  if(argc != 4)
    usage();

  // set a limit on CPU time (e.g. 10 minutes)
  struct rlimit cpu_lim = {600, 600};
  if(setrlimit(RLIMIT_CPU, &cpu_lim) == -1) {
    perror("setrlimit: failed\nError ");
    return -1;
   }

  // check if num of clients is valid
  if((isdigit(atoi(argv[1])) == -1) ||
    (opts.pagesize) = atoi(argv[1])) < 256 ||
     opts.pagesize > 8192) {
    printf("Error: invalid pagesize. please choose from 2^8 to 2^13.\n");
    usage();
  }
  if((opts.pagesize & (opts.pagesize-1)) != 0)
    usage();

  if((isdigit(argv[2])) == -1 || (opts.memsize = atoi(argv[2])) < opts.pagesize) {
    printf("Error: invalid memsize. must be greater than pagesize.\n");
    usage();
  }

  // determine which fault handler to use
  opts.strategy = argv[3];

  fault_handler_stuff_t* that;
  for(that = handler_map; that->strategy != NULL; func++) {
    if(strcmp(that->strategy, opts.strategy) == 0) {
      break;
    }
  }
  if(that.strategy == NULL) {
    printf("Error: invalid strategy.\n");
    usage();
  }

  opts.fault_handler = that;
  opts.num_pages = ceil(opts.memsize/opts.pagesize);

  init();
  simulate();
  print_stuff();

  return 0;
}
