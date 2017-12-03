/* Mark Griffith - 1422270 */

#include <a4vmsim.h>
#include <pfault.h>
#include <options.h>
#include <memory.h>
#include <statistics.h>

#include <stdlib.h>
#include <stdio.h>

static void fault_none(pte_t* pte, ref_op_t operation);
static void fault_rand(pte_t* pte, ref_op_t operation);
static void fault_lru(pte_t* pte, ref_op_t operation);
static void fault_sec(pte_t* pte, ref_op_t operation);

fault_handler_stuff_t handler_map[5] = {
  { "none", fault_none },
  { "mrand", fault_rand },
  { "lru", fault_lru },
  { "sec", fault_sec },
  { NULL, NULL } // for checking opts
};

void init_fault() {
  // initialize random num generator for fault_rand
  long s = 1234567;
  srandom(s);
}

/* Do nothing? */
void fault_none(pte_t* pte, ref_op_t operation) {
  static int page = 0;

  mem_load(page++, pte, operation);
}

/* Evict and replace page at random */
void fault_rand(pte_t* pte, ref_op_t operation) {
    int page;

    page = random() % opts.num_pages;

    if(pmem[page] != NULL && pmem[page]->modified)
      inc_flushes();

    mem_evict(page, operation);
    mem_load(page, pte, operation);
}

/* Search through physical memory until we find the least used
*  page. Evict that page and load the new entry to it */
void fault_lru(pte_t* pte, ref_op_t operation){
  int i;
  int dat_boy = 0;
  int current_min = 0;
  static int inserted = 1;
  static int curr_frame_num = 0;

  // load memory until it is filled. then we can proceed to evict pages
  if(inserted <= opts.num_pages) {
    mem_load(curr_frame_num++, pte, operation);
    inserted++;
  }

  else {
    current_min = pmem[0]->counter;
    for(i=0;i<opts.num_pages;i++) {
      if(pmem[i]->counter < current_min)
        current_min = pmem[i]->counter;
        dat_boy = i;
        break;
    }

    if(pmem[dat_boy]->modified)
      inc_flushes();

    mem_evict(dat_boy, operation);
    mem_load(dat_boy, pte, operation);
  }
}

/* Search through physical memory until we find a page that is
*  not referenced. Evict that page and load the new entry to it.
*  If we find a page that is referenced, dereference it */
void fault_sec(pte_t* pte, ref_op_t operation) {
  int i;
  int dat_boy = -1;
  static int inserted = 1;
  static int curr_frame_num = 0;

  // load memory until it is filled. then we can proceed to evict pages
  if(inserted <= opts.num_pages) {
    mem_load(curr_frame_num++, pte, operation);
    inserted++;
  }

  else {
    while(dat_boy == -1) {
      for(i=0;i<opts.num_pages;i++) {
        if(pmem[i]->sec_chance == 1)
          pmem[i]->sec_chance = 0;
        else if(pmem[i]->sec_chance == 0) {
          dat_boy = i;
          break;
        }
      }
    }

    if(pmem[dat_boy]->modified)
      inc_flushes();

    mem_evict(dat_boy, operation);
    mem_load(dat_boy, pte, operation);
  }
}
