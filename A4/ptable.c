/* Mark Griffith - 1422270 */
/* ptable.c: - handles page table creation and modelling
*            - defines page table entry lookup  functionality
*/

#include <a4vmsim.h>
#include <statistics.h>
#include <ptable.h>
#include <options.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct ptable_level {
  uint size;
  uint log_size;
  uint is_leaf;
} ptable_level_t;

// default levels for the ptable
ptable_level_t levels[3] = {
  { 4096, 12, 0 },
  { 4096, 12, 0 },
  { 256, 8, 1 }
};

// vfn_bits is number of bits in the virtual frame number
// vfn_bits = sum of log_size of all levels
uint vfn_bits;

// Structure representing multi-level pagetable
typedef struct ptable {
  void **table;
  int level;
} ptable_t;

// the table root will hold the current ptable
static ptable_t *table_root;

ptable_t* new_ptable(int level);
pte_t* new_pte(uint vfn);
pte_t* ptable_lookup_helper(uint vfn, uint bits, uint masked_vfn,
			                       ptable_t* pages, ref_op_t opearation);
inline uint getbits(uint x, int p, int n);

/* Allocates spcae for a new ptable_t */
ptable_t* new_ptable(int level) {
  ptable_t* table;
  ptable_level_t* config;
  config = &levels[level];
  assert(config);

  table = malloc(sizeof(struct ptable));
  assert(table);

  table->table = calloc(config->size, sizeof(void*));
  assert(table->table);

  table->level = level;

  return table;
}

/* Allocates space for a new page table entry
*  and sets default parameters */
pte_t* new_pte(uint vfn) {
  pte_t* pte;
  pte = (pte_t*)(malloc(sizeof(pte_t)));
  assert(pte);

  pte->vfn = vfn;
  pte->pfn = -1;
  pte->valid = 0;
  pte->modified = 0;
  pte->reference = 0;

  return pte;
}

pte_t* lookup_vaddr(uint vfn, ref_op_t operation) {
  return ptable_lookup_helper(vfn, 0, vfn, table_root, operation);
}

pte_t* ptable_lookup_helper(uint vfn, uint bits, uint masked_vfn,
			                   ptable_t* pages, ref_op_t operation) {
  uint index;
  int log_size;

  log_size = levels[pages->level].log_size;

  index = getbits(masked_vfn, vfn_bits - (1+bits), log_size);

  bits += log_size;
  masked_vfn = getbits(masked_vfn, vfn_bits - (1+bits), vfn_bits - bits);

  if (levels[pages->level].is_leaf) {
    if (pages->table[index] == NULL) {
      pages->table[index] = (void*)new_pte(vfn);
    }
    return (pte_t*)(pages->table[index]);
  }
  else {
    if (pages->table[index] == NULL) {
      pages->table[index] = new_ptable(pages->level+1);
    }
    return ptable_lookup_helper(vfn, bits, masked_vfn, pages->table[index],
				                       operation);
  }
}

/* */
void init_ptable() {
  int level;
  uint pbits, bits;
  pbits = log_2(opts.pagesize);
  if (pbits == -1) {
    printf("Error: pagesize must be a power of 2\n");
    abort();
  }

  // virtual frame number bits = 32 - page bits
  vfn_bits = addr_bits - pbits;

  bits = 0;
  level = 0;
  while(1) {
    bits += levels[level].log_size;
    if (bits >= vfn_bits)
      break;
    level++;
  }

  levels[level].log_size = levels[level].log_size - (bits - vfn_bits);
  levels[level].size = pow_2(levels[level].log_size);
  levels[level].is_leaf = 1;

  table_root = new_ptable(0);
}
