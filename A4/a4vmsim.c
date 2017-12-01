/* Mark Griffith - 1422270 */

#include <pfault.h>
#include <statistics.h>
#include <ptable.h>
#include <memory.h>
#include <options.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

int references = 0;
double elapsed = 0;

void init();
void simulate();
void print_stuff();

int usage() {
  printf("usage: a4vmsim pagesize memsize strategy [none, mrand, lru, sec]\n");
  _Exit(1);
}

uint log_2(uint x) {
  int i, log = -1;

  for (i=0; i<(8*sizeof(x)); i++) {
    if (x & (1 << i)) {
      // only accept powers of 2
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
	if ((c & (1<<7)) && !(c & (1<<6))) {// 10XXXXXX
    inc_writes();
    return REF_WRITE;
  }

	if ((c & (1<<7)) && (c & (1<<6))) // 11XXXXXX
    return REF_READ;

  if (!((c & (1<<7))) && !(c & (1<<6))) // 00XXXXXX
    inc_accum(c & 0x3F);

  else if (!(c & (1<<7)) && (c & (1<<6))) // 01XXXXXX
    dec_accum(c & 0x3F);

	return REF_NULL;
}

void init() {
  // create page tables
  init_ptable();
  // create physical memory array
  init_mem();
  init_stats();
}

void simulate() {
  uint cnt = 0;
  uint virtual_addr;
  unsigned char ref_str[addr_bits/8];
  ref_op_t operation;
  pte_t* pte;
  fault_handler_t handler;
  clock_t start;

  char* temp=malloc(10);

  stats->accumulator = 0;
  vfn_bits = addr_bits - log_2(opts.pagesize);
  handler = opts.fault_handler->handler;

  printf("[a4vmsim] [page = %d, mem = %d, %s]\n",
        opts.pagesize, opts.memsize, opts.fault_handler->strategy);

  printf("virtual_addr has %d bits, consisting of higher %d bits for vfn (Virtual Frame Number), and lower %d bits for offset within each page (log_2(pagesize=%d))\n",
	addr_bits, vfn_bits, log_2(opts.pagesize), opts.pagesize);

  // start the clock
  start = clock();

  while(read(0, ref_str, addr_bits/8) != 0) {
  // while(scanf("%x", ref_str) != 0) {

    operation = get_op_type(ref_str[0]);

    sprintf(temp, "0x%x%x%x%x", ref_str[3],ref_str[2],ref_str[1],ref_str[0]);

    sscanf(temp, "%x", &virtual_addr);

    inc_references();
    cnt++;

    // print dots because it's cool
    // if ((cnt % 50000) == 0) {
    //   printf(".");
    //   fflush(stdout);
    //   if ((cnt % (64 * 50000)) == 0) {
    //     printf("\n");
    //     fflush(stdout);
    //   }
    // }

    if(operation != REF_NULL) {
      // get the page table entry for the virtual address
      pte = lookup_vaddr(vaddr_to_vfn(virtual_addr), operation);

      // check for page fault
      if(!pte->valid) {
        inc_faults();
        // handle the page fault
        handler(pte, operation);
      }

      // update necessary page table entry parameters
      if(pte->valid) {
        pte->sec_chance = 1; // for sec

        if(pte->modified)
          inc_flushes();
      }

      pte->reference = 1; // set refence bit
      pte->counter = references++;

      if(operation == REF_WRITE)
        pte->modified = 1;
    }
  }
  elapsed = (clock() - (double)start)/(double)CLOCKS_PER_SEC;
}

void print_stuff() {
  print_statistics(elapsed);
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
    (opts.pagesize = atoi(argv[1])) < 256 ||
     opts.pagesize > 8192) {
    printf("Error: invalid pagesize. please choose from 2^8 to 2^13.\n");
    usage();
  }
  // if((opts.pagesize & (opts.pagesize-1)) != 0)
  //   usage();

  if((isdigit(atoi(argv[2]))) == -1 || (opts.memsize = atoi(argv[2])) < opts.pagesize) {
    printf("Error: invalid memsize. must be greater than pagesize.\n");
    usage();
  }

  // determine which fault handler to use
  fault_handler_stuff_t* that;
  for(that = handler_map; that->strategy != NULL; that++) {
    if(strcmp(that->strategy, argv[3]) == 0) {
      break;
    }
  }
  if(that->strategy == NULL) {
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
