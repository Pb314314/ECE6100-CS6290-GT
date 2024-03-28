#include "cachesim_struct.hpp"

Set::Set(uint64_t s){
    if(s > 10){
        #ifdef DEBUG
        cout << "S too big for a Set" <<endl;
        #endif
        return;
    }
    block_num_per_set = pow(2.0, static_cast<double>(s));
    valid_block_num = 0;
    // need to create 2**S Blocks
    for(uint64_t i=0;i<block_num_per_set;i++){
        set[i] = make_shared<Block>();
    }
}

shared_ptr<Block> Set::Find(uint64_t tag){
    if(valid_block_num == 0) return nullptr;
    for(uint64_t i = 0;i<block_num_per_set;i++){
        if(set[i]->valid_bit && set[i]->tag == tag){
            return set[i];
        }
    }
    return nullptr;
}
/*
Insert the input block into set.
Return pointer point to the victim block
replacement policy influence victim
*/
shared_ptr<Block> Set::Insert(shared_ptr<Block> block_ptr, replace_policy rep_policy){
    shared_ptr<Block> result = nullptr;
    if(block_ptr == nullptr){
        
        //cerr << "Wrong! Insert a null block!"<<endl;
        return result;
    }
    shared_ptr<Block> temp_ptr = Find(block_ptr->tag);
    if(temp_ptr){
        //cerr << "Wrong! Block already inside set!"<< endl;
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
        uint64_t victim_index = (rep_policy == REPLACE_POLICY_LRU) ? LRU_Victim_block() : LFU_Victim_block();
        result = set[victim_index];
        set[victim_index] = block_ptr;
    }
    // Finish the insertion of the first block
    return result;
}

uint64_t Set::LRU_Victim_block(){
    uint64_t victim_index = -1;
    uint64_t ts = static_cast<uint64_t>(1) << 63;
    for(uint64_t i=0;i<block_num_per_set;i++){
        if((set[i]->time_stamp) < ts){
            ts = set[i]->time_stamp;
            victim_index = i;
        }
    }
    if(victim_index < 0){
        //cerr << "No block can victim! Wrong!"<<endl;
    }
    return victim_index;
}

uint64_t Set::LFU_Victim_block(){
    uint64_t victim_index = -1;
    uint64_t min_count = static_cast<uint64_t>(1) << 63;
    uint64_t tag_num = static_cast<uint64_t>(1) << 63;
    for(uint64_t i=0;i<block_num_per_set;i++){
        if(set[i]->MRU) continue;
        if((set[i]->counter) < min_count || ((set[i]->counter == min_count) && set[i]->tag < tag_num)){
            min_count = set[i]->counter;
            tag_num = set[i]->tag;
            victim_index = i;
        }
    }
    if(victim_index < 0){
        cerr << "No block can victim! Wrong!"<<endl;
    }
    return victim_index;
}

void Set::Clear_MRU(){
    for(uint64_t i = 0 ; i< block_num_per_set; i++){
        set[i]->MRU = false;
    }
}
