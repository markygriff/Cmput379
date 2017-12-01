/* Mark Griffith - 1422270 */

#ifndef A4VMSIM_H
#define A4VMSIM_H

// #include <pfault.h>

#include <sys/types.h>

typedef enum ref_operation {
  REF_NULL = 0,
  REF_READ = 1,
  REF_WRITE = 2
} ref_op_t;

#define REF_TYPE_NUM 3

extern uint vfn_bits;
extern double elapsed;
const static uint addr_bits = 32;

static inline uint getbits(uint x, int p, int n) {
  return (x >> (p+1-n)) & ~(~0 << n);
}

static inline uint vaddr_to_vfn(uint vaddr) {
  return getbits(vaddr, addr_bits-1, vfn_bits);
}

uint log_2(uint x);
uint pow_2(uint pow);

#endif // A4VMSIM_H
