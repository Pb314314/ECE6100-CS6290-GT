#include <iostream>
#include <cmath>
#include <memory>
#include "cachesim.hpp"
#include <string>
using namespace std;


struct Cache{
    // Use smart pointer.
    // unique_ptr<Type> is the type name of unique pointer 
    // Use make_unique<Type>() for creating unique resource
    // shared_ptr<Type> is the type name of shared pointer 
    // Use make_shared<Type>() for creating shared resource
    struct Block
    {
        uint64_t tag;
        bool valid_bit;
        bool dirty_bit;
        uint64_t time_stamp;
        
        Block() 
        : tag(0), valid_bit(false), dirty_bit(false), time_stamp(0) {}

        Block(uint64_t tag, uint64_t time_stamp) : tag(tag), valid_bit(true), dirty_bit(false), time_stamp(time_stamp) {}

    };

    struct Set
    {
        uint64_t block_num_per_set;
        uint64_t valid_block_num;
        shared_ptr<Block> set[2048];
        Set(uint64_t s){
            if(s > 10){
                cout << "S too big for a Set" <<endl;
                return;
            }
            block_num_per_set = pow(2.0, static_cast<double>(s));
            valid_block_num = 0;
            // need to create 2**S Blocks
            for(uint64_t i=0;i<block_num_per_set;i++){
                set[i] = make_shared<Block>();
            }
        }
        /* 
        Find tag inside a set, return true when find
        return false when not find
        */
        shared_ptr<Block> Find(uint64_t tag){
            //if(valid_block_num == 0) return nullptr;
            for(uint64_t i = 0;i<block_num_per_set;i++){
                if(set[i]->valid_bit && set[i]->tag == tag){
                    return set[i];
                }
            }
            return nullptr;
        }
        /*
        Insert a block into set. Maybe victim a block.
        return null if no need to write back
        return the dirty victim block if vimtim block is dirty or return false
        */
        shared_ptr<Block> Insert(shared_ptr<Block> block_ptr){
            shared_ptr<Block> result = nullptr;
            shared_ptr<Block> temp_ptr = Find(block_ptr->tag);
            if(temp_ptr){
                cout << "Wrong! Block already inside set!"<< endl;
                return result;
            }
            if(valid_block_num < block_num_per_set){
                for(uint64_t i=0;i<block_num_per_set;i++){
                    if(set[i]->valid_bit == 0){
                        set[i] = block_ptr;
                        valid_block_num++;
                        break;
                    }
                }
            }
            else{
                cout << "Need to Victim!" <<endl;
                uint64_t victim_index = Victim_block();
                if(set[victim_index]->dirty_bit){
                    // need to write back; ?????????????
                    cout<< "Dirty bit!!! Need write back"<<endl;
                    result = set[victim_index];
                }
                set[victim_index] = block_ptr;
            }
            // Finish the insertion of the first block
            return result;
        }
        
        /*
        This is the LRU Replacement policy
        */
        uint64_t Victim_block(){
            uint64_t victim_index;
            uint64_t ts = static_cast<uint64_t>(1) << 63;
            for(uint64_t i=0;i<block_num_per_set;i++){
                if((set[i]->time_stamp) < ts){
                    ts = set[i]->time_stamp;
                    victim_index = i;
                }
            }
            return victim_index;
        }
    };
    bool disabled;
    bool prefetcher_disabled;
    bool strided_prefetch_disabled;
    uint64_t c, b, s;
    replace_policy_t replace_policy;
    insert_policy_t prefetch_insert_policy;
    write_strat_t write_strat;

    uint64_t set_num_per_cache;
    
    uint64_t tag_mask;
    uint64_t index_mask;
    uint64_t offset_mask;
    uint64_t time_stamp;
    Cache*   next_level;
    string cache_name;
    unique_ptr<Set> cache[2048];

    Cache(){
        cout<< "Create a cache!" <<endl;
    }

    Cache(const cache_config_t& cache_config, string cache_name = "Cache",Cache* next_level = nullptr)
        :disabled(cache_config.disabled), 
        prefetcher_disabled(cache_config.prefetcher_disabled),
        strided_prefetch_disabled(cache_config.strided_prefetch_disabled),
        c(cache_config.c), 
        b(cache_config.b), 
        s(cache_config.s), 
        replace_policy(cache_config.replace_policy),    // Decide replacement (victim): LRU or LFU
        prefetch_insert_policy(cache_config.prefetch_insert_policy),    // Decide prefetch insert: MIP or LIP
        write_strat(cache_config.write_strat),
        next_level(next_level), 
        cache_name(cache_name)
    {
        cout<< "Create a cache!" <<endl;
        if((c-b-s) > 10){
            cout << "C too big for a Cache" <<endl;
            return;
        }
        
        offset_mask = (1 << b)-1;
        index_mask = ((1 << (c-b-s))-1) << b;
        tag_mask = ((1 << (64-c+s))-1) << (c-s);
        
        time_stamp = pow(2.0, static_cast<double>(c-b+1));

        // need to create 2**(C-S-B) sets
        set_num_per_cache = pow(2, static_cast<double>(c-s-b));
        for(uint64_t i=0;i < set_num_per_cache;i++){
            cache[i] = make_unique<Set>(s);
        }
    }

    void CacheRW(char rw, uint64_t addr, sim_stats_t* stats){
        if(rw == 'R') Read(addr, stats);
        else if(rw == 'W') Write(addr, stats);
        else cout << "Wrong instrcuction!" <<endl;
    } 
    /*
    Get the block specified by addr. Insert the block if miss.
    Return the block specified by addr.
    */
    shared_ptr<Block> Read(uint64_t addr, sim_stats_t* stats){
        //!!! IMPORTANT !!! updata the stats ！！！！！
        uint64_t tag = (addr & tag_mask) >> (c-s);
        uint64_t index = (addr & index_mask) >> b;
        // uint64_t offset = addr & offset_mask;
        //cout<< "Number of set in cache:"<< set_num_per_cache<< "index"<< index <<endl;
        //printf("L1 tag mask: 0x%lx, index mask: 0x%lx, offset mask: 0x%lx\n", tag_mask, index_mask, offset_mask);
        printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), addr, tag, index);
        Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
        
        // Check whether tag inside the current_set;
        shared_ptr<Block> block_ptr = current_set->Find(tag);
        if(block_ptr){ // Hit
            cout << cache_name <<" hit"<<endl;
            //!!! IMPORTANT !!! updata the stats ！！！！！
            block_ptr->time_stamp = time_stamp;
            time_stamp++;
            return block_ptr;
        }
        else{// Miss
            cout << "Can't Find tag in that set"<<endl;
            shared_ptr<Block> inserted_block = Handle_Miss(addr, stats);
            if(!prefetcher_disabled){
                //!!!! Only implement +1 prefetch now !!!!!
                Prefetch(addr, stats);
            }
            return inserted_block;
        }
    }
    /*
    Completely the same as Read, only set the dirty bit of the accessed block;
    */
    shared_ptr<Block> Write(uint64_t addr, sim_stats_t* stats){
        //!!! IMPORTANT !!! updata the stats ！！！！！
        uint64_t tag = (addr & tag_mask) >> (c-s);
        uint64_t index = (addr & index_mask) >> b;
        // uint64_t offset = addr & offset_mask;
        //cout<< "Number of set in cache:"<< set_num_per_cache<< "index"<< index <<endl;
        //printf("L1 tag mask: 0x%lx, index mask: 0x%lx, offset mask: 0x%lx\n", tag_mask, index_mask, offset_mask);
        printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), addr, tag, index);
        Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
        
        // Check whether tag inside the current_set;
        shared_ptr<Block> block_ptr = current_set->Find(tag);
        if(block_ptr){ // Hit
            if(write_strat == WRITE_STRAT_WBWA){
                cout << cache_name <<" hit"<<endl;
                //!!! IMPORTANT !!! updata the stats ！！！！！
                block_ptr->dirty_bit = true;                    // set dirty bit of accessed block
                block_ptr->time_stamp = time_stamp;
                time_stamp++;
                return block_ptr;
            }
            //????????????
            else{// write_strat == WRITE_STRAT_WTWNA
                block_ptr->time_stamp = time_stamp;
                time_stamp++;
                return block_ptr;
            }
        }
        else{// Miss
            if(write_strat == WRITE_STRAT_WBWA){
                cout << "Can't Find tag in that set"<<endl;
                shared_ptr<Block> inserted_block = Handle_Miss(addr, stats);
                inserted_block->dirty_bit = true;               // set dirty bit of accessed block
                if(!prefetcher_disabled){
                    //!!!! Only implement +1 prefetch now !!!!!
                    Prefetch(addr, stats);
                }
                return inserted_block;
            }
            else{// write_strat == WRITE_STRAT_WTWNA
                return nullptr;
            }
        }
    }
    
    /*
    Handle_Miss. Insert the block according to the addr.
    L1 get block from L2, L2 get block from Mem.(All lower mem strcture insert the block into set)
    Return the address of the inserted block.
    */
    shared_ptr<Block> Handle_Miss(uint64_t addr, sim_stats_t* stats ){
        uint64_t tag = (addr & tag_mask) >> (c-s);
        uint64_t index = (addr & index_mask) >> b;
        Set* current_set = cache[index].get();
        shared_ptr<Block> new_block;
        if(next_level){// L1 get block from L2 
            shared_ptr<Block> block_ptr = next_level->Read(addr, stats);
            new_block = make_shared<Block>(tag, time_stamp++);  // use the MIP way to insert
            }
        else{// L2 get block from memory
            new_block = make_shared<Block>(tag, time_stamp++);  // use the MIP way to insert
        }
        shared_ptr<Block> victim_ptr = current_set->Insert(new_block);   // Because there are no data, just create a block in L1, don't use the Block returned by L2
        uint64_t victim_addr =  get_addr(victim_ptr->tag, index);
        if(victim_ptr){
            cout<< "Handle dirty victim!"<<endl;
            // Handle victim here
        }
        return new_block;
    }
    /*
    Prefetch the block to the cache.
    If hit, do nothing.
    If miss, install the new block.
    */
    void Prefetch(uint64_t addr, sim_stats_t* stats){
        cout<< "Prefetch block!"<<endl;
        uint64_t pre_tag = addr & tag_mask;
        uint64_t pre_index = addr & index_mask;
        uint64_t prefetch_addr = (pre_tag | pre_index) +(static_cast<uint64_t>(1) << b);
        //shared_ptr<Block> inserted_block = Insert(prefetch_addr, stats);
        uint64_t tag = (prefetch_addr & tag_mask) >> (c-s);
        uint64_t index = (prefetch_addr & index_mask) >> b;
        printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), prefetch_addr, tag, index);
        Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
        
        shared_ptr<Block> block_ptr = current_set->Find(tag);

        if(block_ptr == nullptr){ // Hit
            shared_ptr<Block> inserted_block = Handle_Miss(prefetch_addr, stats);
        }
    }
    uint64_t get_addr(uint64_t tag, uint64_t index){
            uint64_t addr_tag = tag << (c-s);
            uint64_t addr_index = index << b;
            return addr_tag | addr_index;
    }
    /* 
    Need to write back the dirty block from L1 to L2;
    */
    void Victim(uint64_t victim_addr, sim_stats_t* stats){
        if(next_level){
            // L1 write back to L2
            next_level->Write(victim_addr, stats);
        }
        //else{
        // L2 write back to Mem
        // do nothing
        //}
        return;
    }

};

