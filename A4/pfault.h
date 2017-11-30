/* Mark Griffith - 1422270 */
/* pfault: - defines page fault handler functions for the supported
*            strategies
*             1. none
*             2. rand
*             3. lru (least recently used)
*             4. sec (second chance)
*/

#ifndef PFAULT_H
#define PFAULT_H

// #include <a4vmsim.h>
#include <ptable.h>
#include <unistd.h>

/* the fault handler function type */
/* pte: new page to be inserted */
typedef void (*fault_handler_t)(pte_t* pte, ref_op_t operation);

typedef struct fault_handler_stuff {
  char* strategy;
  fault_handler_t handler; // page fault function handler
} fault_handler_stuff_t;

extern fault_handler_stuff_t handler_map[];

void init_fault();

#endif // PFAULT_H
