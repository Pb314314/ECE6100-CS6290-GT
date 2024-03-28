#include "cachesim_struct.hpp"

Cache::Cache(const cache_config_t& cache_config, string cache_name, Cache* next_level )
    :disabled(cache_config.disabled), 
    prefetcher_disabled(cache_config.prefetcher_disabled),
    strided_prefetch_disabled(cache_config.strided_prefetch_disabled),
    c(cache_config.c), 
    b(cache_config.b), 
    s(cache_config.s), 
    last_miss_addr(0),
    replace_policy(cache_config.replace_policy),    // Decide replacement (victim): LRU or LFU
    prefetch_insert_policy(cache_config.prefetch_insert_policy),    // Decide prefetch insert: MIP or LIP
    write_strat(cache_config.write_strat),
    next_level(next_level), 
    cache_name(cache_name)
{
    if((c-b-s) > 10){
        //cerr << "C too big for a Cache" <<endl;
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

void Cache::CacheRW(char rw, uint64_t addr, sim_stats_t* stats){
    #ifdef DEBUG
    printf("Time: %" PRIu64 ". Address: 0x%" PRIx64 ". Read/Write: %c\n", this->Time, addr,rw);
    #endif
    if(rw == 'R') {
        stats->reads++;
        Read(addr, stats);
    }
    else if(rw == 'W') {
        stats->writes++;
        Write(addr, stats);
    }
    else cerr << "Wrong instruction!" <<endl;
    this->Time++;
    #ifdef DEBUG
    printf("\n");
    #endif
} 

void Cache::modify_stats(sim_stats_t* stats, Stats_mode mode){
    switch(mode){
        /*case Reads:
            stats->reads++;
            break;
        case Writes:
            stats->writes++;
            break;*/
        case L1_access:
            stats->accesses_l1++;
            break;
        case L1_hit:
            stats->hits_l1++;
            break;
        case L1_miss:
            stats->misses_l1++;
            break;
        case L2_read:
            stats->reads_l2++;
            break;
        case L2_write:
            stats->writes_l2++;
            break;
        case L2_read_hit:
            stats->read_hits_l2++;
            break;
        case L2_read_miss:
            stats->read_misses_l2++;
            break;
        case L2_prefetch:
            stats->prefetches_l2++;
            break;
        default:
            cerr << "Unknown mode"<<endl;
    }
}

/*
Get the block specified by addr. Insert the block if miss.
Return the block specified by addr.
*/
shared_ptr<Block> Cache::Read(uint64_t addr, sim_stats_t* stats){
    //!!! IMPORTANT !!! updata the stats ！！！！！
    if(this->cache_name == "L1"){
        modify_stats(stats, L1_access);
    }
    else{
        modify_stats(stats, L2_read);
    }
    uint64_t tag = (addr & tag_mask) >> (c-s);
    uint64_t index = (addr & index_mask) >> b;
    #ifdef DEBUG
    printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), addr, tag, index);
    #endif
    if(this->cache_name == "L2" && this->disabled){     // If l2 disabled, modify stats and return
        modify_stats(stats, L2_read_miss);
        #ifdef DEBUG
        cout<< "L2 is disabled, treating this as an L2 read miss" <<endl;
        #endif
        return nullptr;
    }

    Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
    shared_ptr<Block> block_ptr = current_set->Find(tag);// Check whether tag inside the current_set;
    if(block_ptr){ // Hit
        if(this->cache_name == "L1"){   //updata the stats 
            #ifdef DEBUG
            cout << cache_name <<" hit"<<endl;
            #endif
            modify_stats(stats, L1_hit);
        }
        else{ // L2 hit
            #ifdef DEBUG
            cout << cache_name <<" read hit"<<endl;
            #endif
            modify_stats(stats, L2_read_hit);
        }
        if(replace_policy == REPLACE_POLICY_LRU){
            block_ptr->time_stamp = time_stamp;
            time_stamp++;
        }
        else{ //replace_policy == REPLACE_POLICY_LFU
            current_set->Clear_MRU();
            block_ptr->MRU = true;
            block_ptr->counter++;
        }
        #ifdef DEBUG
        printf("In %s, moving Tag: 0x%lx and Index: 0x%lx to MRU position\n",
        this->cache_name.c_str(),
        block_ptr->tag,
        index
        );
        #endif

        return block_ptr;
    }
    else{// Read Access Miss
        if(this->cache_name == "L1"){
            #ifdef DEBUG
            cout << cache_name <<" miss"<<endl;
            #endif
            modify_stats(stats, L1_miss);
        }
        else{
            #ifdef DEBUG
            cout << cache_name <<" read miss"<<endl;
            #endif
            modify_stats(stats, L2_read_miss);
        }
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
shared_ptr<Block> Cache::Write(uint64_t addr, sim_stats_t* stats){
    //!!! IMPORTANT !!! updata the stats ！！！！！
    if(this->cache_name == "L1"){
        modify_stats(stats, L1_access);
    }
    else{
        modify_stats(stats, L2_write);
    }
    uint64_t tag = (addr & tag_mask) >> (c-s);
    uint64_t index = (addr & index_mask) >> b;
    #ifdef DEBUG
    printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), addr, tag, index);
    #endif
    if(this->cache_name == "L2" && this->disabled){     // If l2 disabled, modify stats and return
        #ifdef DEBUG
        cout<< "L2 is disabled, writing through to memory" <<endl;
        #endif
        return nullptr;
    }
    Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
    
    // Check whether tag inside the current_set;
    shared_ptr<Block> block_ptr = current_set->Find(tag);
    if(block_ptr){ // Hit
        if(write_strat == WRITE_STRAT_WBWA){    //  L1
            #ifdef DEBUG
            cout << cache_name <<" hit"<<endl;
            #endif
            if(this->cache_name == "L1"){
                modify_stats(stats, L1_hit);
            }
            block_ptr->dirty_bit = true;                    // set dirty bit of accessed block
            if(replace_policy == REPLACE_POLICY_LRU){
            block_ptr->time_stamp = time_stamp;
            time_stamp++;
            }
            else{ //replace_policy == REPLACE_POLICY_LFU
                current_set->Clear_MRU();
                block_ptr->MRU = true;
                block_ptr->counter++;
            }
            #ifdef DEBUG
            printf("In %s, moving Tag: 0x%lx and Index: 0x%lx to MRU position and setting dirty bit\n",
            
            this->cache_name.c_str(),
            block_ptr->tag,
            index
            );
            #endif
            return block_ptr;
        }
        // only do here when L2.write. when L1 victim
        // L2 write Hit, updata MRU
        #ifdef DEBUG
        printf("L2 found block in cache on write\n");
        #endif
        if(replace_policy == REPLACE_POLICY_LRU){
            block_ptr->time_stamp = time_stamp;
            time_stamp++;
        }
        else{
            current_set->Clear_MRU();
            block_ptr->MRU = true;
            block_ptr->counter++;
        }
        block_ptr->dirty_bit = false;
        #ifdef DEBUG
        printf("In %s, moving Tag: 0x%lx and Index: 0x%lx to MRU position\n", this->cache_name.c_str(),block_ptr->tag, index);
        #endif
        return block_ptr;
    }
    else{// Write Access Miss
        if(write_strat == WRITE_STRAT_WBWA){
            #ifdef DEBUG
            cout << cache_name <<" miss"<<endl;
            #endif
            if(this->cache_name == "L1"){
                modify_stats(stats, L1_miss);
            }
            shared_ptr<Block> inserted_block = Handle_Miss(addr, stats);
            inserted_block->dirty_bit = true;               // set dirty bit of accessed block
            // No need to prefetch for L1
            return inserted_block;
        }
        // only do here when L2.write. when L1 victim
        // L2 write Miss, do nothing
        #ifdef DEBUG
        printf("L2 did not find block in cache on write, writing through to memory anyway\n");
        #endif
        return nullptr;
    }
}

/*
Handle_Miss. Insert the block according to the addr.
L1 get block from L2, L2 get block from Mem.(All lower mem strcture insert the block into set)
Return the address of the inserted block.
*/
shared_ptr<Block> Cache::Handle_Miss(uint64_t addr, sim_stats_t* stats){
    uint64_t tag = (addr & tag_mask) >> (c-s);
    uint64_t index = (addr & index_mask) >> b;
    Set* current_set = cache[index].get();
    shared_ptr<Block> new_block;
    if(next_level ){// L1 get block from L2
        shared_ptr<Block> block_ptr = next_level->Read(addr, stats);
        new_block = make_shared<Block>(tag, time_stamp++);  // Because there are no data, just create a block in L1, no need to use the Block returned by L2
        }
    else{// L2 get block from memory
        new_block = make_shared<Block>(tag, time_stamp++);  // This is the MIP insert
    }
    // Always use MIP insert
    shared_ptr<Block> victim_ptr = current_set->Insert(new_block, this->replace_policy);    // Insert the new_block into set.
    current_set->Clear_MRU();   // Clear the MRU after insertion
    new_block->MRU = true;      // Set MRU of the current bit
    new_block->counter = 1;

    if(victim_ptr ){ // Only L1 need to write back to L2
        #ifdef DEBUG
        if(write_strat == WRITE_STRAT_WBWA){
            printf("Evict from %s: block with valid=%d, dirty=%d, tag 0x%lx and index=0x%lx\n", 
            cache_name.c_str(), 
            victim_ptr->valid_bit, 
            victim_ptr->dirty_bit, 
            victim_ptr->tag, 
            index);
        }
        else{
            printf("Evict from %s: block with valid=%d, tag 0x%lx and index=0x%lx\n", 
            cache_name.c_str(), 
            victim_ptr->valid_bit, 
            victim_ptr->tag, 
            index);// Only print Evict info when not prefetch
        }
        #endif
        uint64_t victim_addr =  get_addr(victim_ptr->tag, index);
        // Handle victim here
        if(victim_ptr->dirty_bit){
            #ifdef DEBUG
            printf("Writing back dirty block with address 0x%lx to L2\n", victim_addr);
            #endif
            Victim(victim_addr, stats);
        }
    }
    else{
        #ifdef DEBUG
        printf("Evict from %s: block with valid=0 and index=0x%lx\n", 
        cache_name.c_str(),
        index);
        #endif
    }
    return new_block;
}


shared_ptr<Block> Cache::Prefetch_Miss(uint64_t addr, sim_stats_t* stats){
    uint64_t tag = (addr & tag_mask) >> (c-s);
    uint64_t index = (addr & index_mask) >> b;
    Set* current_set = cache[index].get();
    shared_ptr<Block> new_block;
    if(prefetch_insert_policy == INSERT_POLICY_MIP){   
        new_block = make_shared<Block>(tag, time_stamp++);  // This is the MIP insert
    }
    else{//prefetch_insert_policy == INSERT_POLICY_LIP
        if(!current_set->valid_block_num){// no valid block in set use time_stamp
            new_block = make_shared<Block>(tag, time_stamp);
        }
        else{// use min_time_stamp-1
            uint64_t min_time = static_cast<uint64_t>(1)<<63;
            for(uint64_t i=0;i<current_set->valid_block_num;i++){
                min_time = std::min(min_time, current_set->set[i]->time_stamp);
            }
            new_block = make_shared<Block>(tag, min_time-1);
        }
    }
    // Build the block base on insertion policy
    shared_ptr<Block> victim_ptr = current_set->Insert(new_block, this->replace_policy);    // Insert the new_block into set.
    if(prefetch_insert_policy == INSERT_POLICY_MIP){
        current_set->Clear_MRU();   //Clear all other MRU
        new_block->MRU = true;      // Set MRU, counter = 0;
        new_block->counter = 0;
    }
    else{
        new_block->MRU = false;
        new_block->counter = 0;
    }
    return new_block;
}
/*
Prefetch the block to the cache.
If hit, do nothing.
If miss, install the new block.
*/
void Cache::Prefetch(uint64_t addr, sim_stats_t* stats){
    uint64_t pre_tag = addr & tag_mask;
    uint64_t pre_index = addr & index_mask;
    uint64_t miss_addr = (pre_tag | pre_index); //+(static_cast<uint64_t>(1) << b);
    uint64_t prefetch_addr = miss_addr;
    if(this->strided_prefetch_disabled ){// +1 prefetch
        prefetch_addr += (static_cast<uint64_t>(1) << b);
    }
    else{ // strided prefetch
        prefetch_addr += miss_addr -last_miss_addr;
        last_miss_addr = miss_addr;
    }
    //shared_ptr<Block> inserted_block = Insert(prefetch_addr, stats);
    uint64_t tag = (prefetch_addr & tag_mask) >> (c-s);
    uint64_t index = (prefetch_addr & index_mask) >> b;
    //printf("%s decomposed address 0x%lx -> Tag: 0x%lx and Index: 0x%lx\n", cache_name.c_str(), prefetch_addr, tag, index);
    Set* current_set = cache[index].get(); // Use raw pointer to modify the set, without Ownership Transfer
    
    shared_ptr<Block> block_ptr = current_set->Find(tag);

    if(block_ptr == nullptr){ // Miss
        #ifdef DEBUG
        printf("Prefetch block with address 0x%lx from memrory to L2\n", prefetch_addr);
        #endif
        modify_stats(stats, L2_prefetch);   // Only count prefetch when L2 miss
        shared_ptr<Block> inserted_block = Prefetch_Miss(prefetch_addr, stats);// true to indicate it is prefetch call, dont need evict print
    }
}

uint64_t Cache::get_addr(uint64_t tag, uint64_t index){
    uint64_t addr_tag = tag << (c-s);
    uint64_t addr_index = index << b;
    return addr_tag | addr_index;
}

/* 
Need to write back the dirty block from L1 to L2; ????????????????????
*/
void Cache::Victim(uint64_t victim_addr, sim_stats_t* stats){
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