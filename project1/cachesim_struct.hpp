#include <iostream>
#include <cmath>
#include <memory>
#include "cachesim.hpp"
#include <string>
using namespace std;

enum Stats_mode{
    //Reads,
    //Writes,

    L1_access,
    L1_hit,
    L1_miss,

    L2_read,
    L2_write,
    L2_read_hit,
    L2_read_miss,
    L2_prefetch
};

struct Block{
    uint64_t tag;
    bool valid_bit;
    bool dirty_bit;
    uint64_t time_stamp;
    bool MRU;
    uint64_t counter;
    Block() : tag(0), valid_bit(false), dirty_bit(false), time_stamp(0), MRU(false), counter(0){}

    Block(uint64_t tag, uint64_t time_stamp) : tag(tag), valid_bit(true), dirty_bit(false), time_stamp(time_stamp), MRU(false), counter(0){}
};

struct Set{
    uint64_t block_num_per_set;
    uint64_t valid_block_num;
    shared_ptr<Block> set[2048];
    Set(uint64_t s);
    /* 
    Find tag inside a set, return true when find
    return false when not find
    */
    shared_ptr<Block> Find(uint64_t tag);
    /*
    Insert a block into set. Maybe victim a block.
    Return null if no need to write back; Return the dirty victim block if vimtim block is dirty or return false
    */
    shared_ptr<Block> Insert(shared_ptr<Block> block_ptr, replace_policy rep_policy);
    /*
    This is the LRU Replacement policy
    */
    uint64_t LRU_Victim_block();
    uint64_t LFU_Victim_block();

    void Clear_MRU();
};

struct Cache{
    bool disabled;
    bool prefetcher_disabled;
    bool strided_prefetch_disabled;
    uint64_t c, b, s;
    uint64_t last_miss_addr;
    replace_policy_t replace_policy;
    insert_policy_t prefetch_insert_policy;
    write_strat_t write_strat;

    uint64_t set_num_per_cache;
    uint64_t Time = 0;
    
    uint64_t tag_mask;
    uint64_t index_mask;
    uint64_t offset_mask;
    uint64_t time_stamp;
    
    Cache*   next_level;
    string cache_name;
    unique_ptr<Set> cache[2048];

    Cache(){}

    Cache(const cache_config_t& cache_config, string cache_name = "Cache",Cache* next_level = nullptr);

    void CacheRW(char rw, uint64_t addr, sim_stats_t* stats);

    void modify_stats(sim_stats_t* stats, Stats_mode mode);
    /*
    Get the block specified by addr. Insert the block if miss.
    Return the block specified by addr.
    */
    shared_ptr<Block> Read(uint64_t addr, sim_stats_t* stats);

    //Completely the same as Read, only set the dirty bit of the accessed block;
    shared_ptr<Block> Write(uint64_t addr, sim_stats_t* stats);
    
    /*
    Handle_Miss. Insert the block according to the addr.
    L1 get block from L2, L2 get block from Mem.(All lower mem strcture insert the block into set)
    Return the address of the inserted block.
    */
    shared_ptr<Block> Handle_Miss(uint64_t addr, sim_stats_t* stats);

    shared_ptr<Block> Prefetch_Miss(uint64_t addr, sim_stats_t* stats);
    /*
    Prefetch the block to the cache.
    If hit, do nothing.
    If miss, install the new block.
    */
    void Prefetch(uint64_t addr, sim_stats_t* stats);


    uint64_t get_addr(uint64_t tag, uint64_t index);

    /* 
    Need to write back the dirty block from L1 to L2; ????????????????????
    */
    void Victim(uint64_t victim_addr, sim_stats_t* stats);

};

