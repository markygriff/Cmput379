/* Mark Griffith - 1422270 */
/* statistics: - handles printing stats about simulator execution
*              - prints references, writes, faults, flushes, and accumulator
*/

#ifndef STATISTICS_H
#define STATISTICS_H

#include <a4vmsim.h>

#include <stdio.h>
#include <sys/types.h>

typedef struct stats {
  uint references;
  uint writes;
  uint faults;
  uint flushes;
  int accumulator;
} stats_t;

// global statistics
extern stats_t* stats;

void init_stats();
void print_statistics();

static inline void inc_references() {
  stats->references++;
}

static inline void inc_writes() {
  stats->writes++;
}

static inline void inc_faults() {
  stats->faults++;
}

static inline void inc_flushes() {
  stats->flushes++;
}

static inline void inc_accum(uint amount) {
  stats->accumulator += amount;
}

static inline void dec_accum(uint amount) {
  stats->accumulator -= amount;
}

#endif // STATISTICS_H
