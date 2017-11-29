/* Mark Griffith - 1422270 */

#ifndef A4VMSIM_H
#define A4VMSIM_H

#include <sys/types.h>

typedef enum ref_operation {
  REF_NULL = 0,
  REF_READ = 1,
  REF_WRITE = 2
} ref_op_t;

typedef struct opts {
  int pagesize;
  int memsize;
  int num_pages;
  fault_handler_stuff_t* fault_handler;
} opts_t;

#define REF_TYPE_NUM 3

extern opts_t opts; // global options
extern uint vfn_bits;
const static uint addr_bits = 32;

static inline uint getbits(uint x, int p, int n) {
  return (x >> (p+1-n)) & ~(~0 << n);
}

static inline uint vaddr_to_vfn(vaddr_t vaddr) {
  return getbits(vaddr, addr_space_bits-1, vfn_bits);
}

uint log_2(uint x);
uint pow_2(uint pow);

#endif // A4VMSIM_H
