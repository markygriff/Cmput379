/* Mark Griffith - 1422270 */

#include <statistics.h>

#include <options.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// initialize global stats
stats_t* stats;

void print_statistics() {
  printf("[a4vmsim] %d references processed using ’%s’ in _____\n",
        stats->references);
  printf("[a4vmsim] page faults = %d, write count = %d, flushes = %d\n",
        stats->faults, stats->writes, stats->flushes);
  printf("[a4vmsim] accumulator = \n", stats->accumulator);
}
