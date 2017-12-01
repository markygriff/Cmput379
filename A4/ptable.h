/* Mark Griffith - 1422270 */

#ifndef PTABLE_H
#define PTABLE_H

#include <a4vmsim.h>

typedef struct pte {
  uint pfn; // physical frame number
  uint vfn; // virtual frame number
  int modified;
  int valid; // true if in physmem
  int counter; // for lru
  int sec_chance; // for sec
  int reference;
} pte_t;

void init_ptable();
pte_t* lookup_vaddr(uint vfn, ref_op_t operation);

#endif // PTABLE_H
