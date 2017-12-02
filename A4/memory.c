/* Mark Griffith - 1422270 */

#include <memory.h>
#include <ptable.h>
#include <statistics.h>
#include <options.h>

#include <assert.h>
#include <stdlib.h>

// initialize global physical memory array
pte_t** pmem;

/* Allocates space for physical memory array */
void init_mem() {
  pmem = (pte_t**)(calloc(opts.num_pages, sizeof(pte_t*)));
  assert(pmem);
}

/* Dereferences the page table entry in physical memory
*  and resets physical memory cell parameters */
void mem_evict(uint pfn, ref_op_t operation) {
  assert(0 <= pfn && pfn < opts.num_pages);

  // no page. do nothing
  if(pmem[pfn] == NULL || !pmem[pfn]->valid) {
    pmem[pfn] = NULL;
    return;
  }

  pmem[pfn]->modified = 0;
  pmem[pfn]->valid = 0;
  pmem[pfn] = NULL;
}

/* Loads a page table entry into the physical memory array */
void mem_load(uint pfn, pte_t* load_page, ref_op_t operation) {
  assert(0 <= pfn && pfn < opts.num_pages);
  assert(load_page && !load_page->valid);
  assert(pmem[pfn] == NULL);

  pmem[pfn] = load_page;

  pmem[pfn]->pfn = pfn;
  pmem[pfn]->reference = 0;
  pmem[pfn]->modified = 0;
  pmem[pfn]->valid = 1;
}

/* Simple getter for the physical memory array */
pte_t** get_pmem_array() {
  return pmem;
}
