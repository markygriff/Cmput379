/* Mark Griffith - 1422270 */

#include <statistics.h>
#include <options.h>

// #include <options.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// initialize global stats
stats_t* stats;

void init_stats() {
  stats = (stats_t*)calloc(1, sizeof(stats_t));
  assert(stats);
}

void print_statistics() {
  printf("[a4vmsim] %d references processed using ’%s’ in _____\n",
        stats->references, opts.fault_handler->strategy);
  printf("[a4vmsim] page faults = %d, write count = %d, flushes = %d\n",
        stats->faults, stats->writes, stats->flushes);
  printf("[a4vmsim] accumulator = %d\n", stats->accumulator);
}
