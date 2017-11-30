/* Mark Griffith - 1422270 */

#include <a4vmsim.h>
#include <pfault.h>

typedef struct opts {
  int pagesize;
  int memsize;
  int num_pages;
  fault_handler_stuff_t* fault_handler;
} opts_t;

extern opts_t opts; // global options
