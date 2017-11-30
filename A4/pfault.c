/* Mark Griffith - 1422270 */

#include <a4vmsim.h>
#include <pfault.h>
#include <options.h>
#include <memory.h>

#include <stdlib.h>
#include <stdio.h>

static void fault_none(pte_t* pte, ref_op_t operation);
static void fault_rand(pte_t* pte, ref_op_t operation);
static void fault_lru(pte_t* pte, ref_op_t operation);
static void fault_sec(pte_t* pte, ref_op_t operation);

fault_handler_stuff_t handler_map[5] = {
  { "none", fault_none },
  { "rand", fault_rand },
  { "lru", fault_lru },
  { "sec", fault_sec },
  { NULL, NULL } // for checking opts
};

void init_fault() {
  // initialize fault_rand random num generator
  long s = 1234567;
  srandom(s);
}

void fault_none(pte_t* pte, ref_op_t operation) {

}

// evict and replace page at random
void fault_rand(pte_t* pte, ref_op_t operation) {
    int page;
    page = random() % opts.num_pages;
    mem_evict(page, operation);
    mem_load(page, pte, operation);
}

void fault_lru(pte_t* pte, ref_op_t operation){
}

void fault_sec(pte_t* pte, ref_op_t operation) {
  static int c = 1;
}
