/* Mark Griffith - 1422270 */
/* memory: - controls and models the physical memory of the simulator */

#ifndef MEMORY_H
#define MEMORY_H

#include <a4vmsim.h>
#include <ptable.h>

/* initializes an empty memory */
void init_mem();

/* get array of pte_t that models the physical memory */
pte_t** mem_array();

/* evict page at pfn from memory and mark pfn as empty */
void mem_evict(uint pfn, ref_op_t operation);

/* lpad page into physical memory space (pfn)
 * assumes pfn is empty */
void mem_load(uint pfn, pte_t* load_page ref_op_t operation);

// gloabl physical memory array
extern pte_t **pmem;

#endif // MEMORY_H
