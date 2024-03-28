#include "cachesim.hpp"
#include "cachesim_struct.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include <algorithm>
using namespace std;
/**
 * Subroutine for initializing the cache simulator. You many add and initialize any global or heap
 * variables as needed.
 * TODO: You're responsible for completing this routine
 */

Cache L1;
Cache L2;

//std::streambuf* coutbuf = cout.rdbuf();

//std::ofstream out("./pb_out/out.txt");
//std::streambuf* coutbuf = std::cout.rdbuf(out.rdbuf());

void sim_setup(sim_config_t *config) {
    // Build L1 and L2 cache.
    L2 = Cache(config->l2_config, "L2");
    L1 = Cache(config->l1_config, "L1", &L2);
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * TODO: You're responsible for completing this routine
 */
void sim_access(char rw, uint64_t addr, sim_stats_t* stats) {
    L1.CacheRW(rw, addr, stats);
}
/**
 * Calculate the HitTime for L1.
 */
double calculateL1HitTime(uint64_t c, uint64_t b, uint64_t s) {
    const double k0 = 1.0; 
    const double k1 = 0.15; 
    const double k2 = 0.15; 
    
    return k0 + (k1 * (c - b - s)) + k2 * (std::max(3.0, static_cast<double>(s)) - 3);
}

/**
 * Calculate the HitTime for L2.
 */
double calculateL2HitTime(uint64_t c, uint64_t b, uint64_t s) {
    const double k3 = 4.0; 
    const double k4 = 0.3; 
    const double k5 = 0.3; 
    
    return k3 + (k4 * (c - b - s)) + k5 * (std::max(3.0, static_cast<double>(s)) - 3);
}
/**
 * Calculate the AAT for use ht, mr, mp.
 */
double calculate_AAT(double ht, double mr, double mp){
    double result = ht + mr*mp;
    return result;
}
/**
 * Calculate the stats.
 */
void calculate_stats(sim_stats_t *stats){
    stats->hit_ratio_l1 = static_cast<double>(stats->hits_l1) / stats->accesses_l1;
    stats->miss_ratio_l1 = 1.0 - stats->hit_ratio_l1;
    double l1_ht = calculateL1HitTime(L1.c, L1.b, L1.s);
    
    stats->read_hit_ratio_l2 = static_cast<double>(stats->read_hits_l2)/stats->reads_l2;
    stats->read_miss_ratio_l2 = 1.0 - stats->read_hit_ratio_l2;
    double l2_ht = L2.disabled ? 0.0 : calculateL2HitTime(L2.c, L2.b, L2.s);
    double l2_mp = 100.0;
    stats->avg_access_time_l2 = calculate_AAT(l2_ht, stats->read_miss_ratio_l2, l2_mp);
    stats->avg_access_time_l1 = calculate_AAT(l1_ht, stats->miss_ratio_l1, stats->avg_access_time_l2);
}
/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * TODO: You're responsible for completing this routine
 */
void sim_finish(sim_stats_t *stats) {
    calculate_stats(stats);
}
