/**
 * @file procsim.hpp
 * @brief Project 2: Tomasulo & Branch Predictor Simulator
 * 
 * @author Bo Pang
 * @date 2024-04-06
 */
#ifndef PROCSIM_H
#define PROCSIM_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>
#include <iostream>
#include <vector>
#include <queue>
#define L1_HIT_CYCLES (2)
#define L2_HIT_CYCLES (10)
#define L2_MISS_CYCLES (100)

#define ALU_STAGES 1
#define MUL_STAGES 3
#define LSU_STAGES 1
#define REG_NUM 32
#define MAT_REG_NUM 64

typedef enum {
    CACHE_LATENCY_L1_HIT = 0,
    CACHE_LATENCY_L2_HIT,
    CACHE_LATENCY_L2_MISS
} cache_lentency_t;

typedef enum {
    OPCODE_ADD = 2,
    OPCODE_MUL,
    OPCODE_LOAD,
    OPCODE_STORE,
    OPCODE_BRANCH, 
} opcode_t;

typedef enum {
    DRIVER_READ_OK = 0,
    DRIVER_READ_MISPRED,
    DRIVER_READ_ICACHE_MISS,
    DRIVER_READ_END_OF_TRACE,
} driver_read_status_t;

typedef struct {
    uint64_t pc;        // pc address
    opcode_t opcode;    // opcode
    int8_t dest;        // dest reg
    int8_t src1;        // src1 reg
    int8_t src2;        // src2 reg
    uint64_t load_store_addr;   // load address

    bool br_taken;              // branch taken
    uint64_t br_target;         // br target address
    cache_lentency_t icache_hit;    // 
    cache_lentency_t dcache_hit;    //

    // the driver fill these fields for you
    bool mispredict;                // mispredict
    uint64_t dyn_instruction_count;
} inst_t;

// This config struct is populated by the driver for you
typedef struct {
    size_t num_rob_entries;
    size_t num_schedq_entries_per_fu;
    size_t num_alu_fus;
    size_t num_mul_fus;
    size_t num_lsu_fus;
    size_t fetch_width;
    size_t dispatch_width;
    size_t retire_width;
    bool misses_enabled;
} procsim_conf_t;

typedef struct {
    uint64_t cycles;
    uint64_t instructions_fetched;
    uint64_t instructions_retired;

    uint64_t no_fire_cycles;            // (Schedule)nothing in the scheduling queue could be fire
    uint64_t rob_no_dispatch_cycles;    // (Dispatch)Dispatch stop putting instructions into the scheduling queue due to ROB constraint
    uint64_t dispq_max_usage;           // maximum number of instructions in the dispatch queue
    uint64_t schedq_max_usage;          // maximum number of instructions in the schedule queue
    uint64_t rob_max_usage;             // maximum number of instructions in the rob
    
    uint64_t dispq_total_usage;
    uint64_t schedq_total_usage;
    uint64_t rob_total_usage;

    double dispq_avg_usage;
    double schedq_avg_usage;
    double rob_avg_usage;
    double ipc;
    
    // The driver populates the stats below for you
    uint64_t icache_misses;
    uint64_t dcache_misses;
    uint64_t num_branch_instructions;
    uint64_t branch_mispredictions; // you need to set *retired_mispredict_out correctly in stage_state_update to get this matched
    uint64_t instructions_in_trace;
} procsim_stats_t;


extern const inst_t *procsim_driver_read_inst(driver_read_status_t *driver_read_status_output);
extern void procsim_driver_update_predictor(uint64_t ip,
                                            bool is_taken,
                                            uint64_t dyncount);

// There is more information on these functions in procsim.cpp
extern void procsim_init(const procsim_conf_t *sim_conf,
                         procsim_stats_t *stats);
extern uint64_t procsim_do_cycle(procsim_stats_t *stats,
                                 bool *retired_mispredict_out);
extern void procsim_finish(procsim_stats_t *stats);

extern void print_instruction(const inst_t *inst);

struct dispatch_queue{
    size_t disp_q_entries;
    std::queue<const inst_t*> dispatch_q;       
    dispatch_queue():disp_q_entries(0){};
    dispatch_queue(size_t disp_q_entries): disp_q_entries(disp_q_entries){}
    // insert inst into dispatch queue
    void insert(const inst_t* inst){
        dispatch_q.push(inst);
    }
};

struct schedule_queue_entry{
    const inst_t*   inst;
    // information for source registers
    bool     src1_ready;
    uint64_t src1_tag;
    uint64_t src1_value;
    bool     src2_ready;
    uint64_t src2_tag;
    uint64_t src2_value;
    // information for mat registers
    uint8_t ms;
    bool ms_ready;
    uint64_t ms_tag;
    uint8_t md;
    bool md_ready;
    uint64_t md_tag;
    // entry information
    uint64_t tag1;
    uint64_t tag2;
    uint64_t fu_count;      // for execute
    bool scheduled;         // for schedule queue skip the scheduled instructions
    bool finished;          // for ROB retire
    schedule_queue_entry(){};
    schedule_queue_entry(const inst_t* inst): 
        inst(inst), src1_ready(0), src1_tag(0),src1_value(0),src2_ready(0), src2_tag(0),src2_value(0), 
        ms(0), ms_ready(0), ms_tag(0), md(0), md_ready(0), md_tag(0),
        tag1(0), tag2(0), fu_count(0), scheduled(0),finished(0){
        if(inst->opcode == OPCODE_LOAD || inst->opcode == OPCODE_STORE){     // load or store
            uint8_t mat_reg = (inst->load_store_addr >> 6) & 0b111111;
            ms = mat_reg;
            md = mat_reg;
        }
    }
    // whether an entry is_load_store
    bool is_load_store() const{
        return (inst->opcode == OPCODE_LOAD || inst->opcode == OPCODE_STORE);
    }
};

struct schedule_queue{
    size_t sche_q_entries;
    std::vector<schedule_queue_entry> schedule_q;
    schedule_queue(): sche_q_entries(0){}
    schedule_queue(size_t sche_q_entries): sche_q_entries(sche_q_entries){}
    void push(schedule_queue_entry entry){
        if(schedule_q.size() < sche_q_entries)schedule_q.push_back(entry);
    }
    // Whether schedule_q is full
    bool full() const{
        return schedule_q.size() >= sche_q_entries;
    }
    // check schedule queue, whether index-th inst can be fired
    bool inst_ready(uint64_t index) const{
        bool is_ready = true;
        schedule_queue_entry entry = schedule_q[index];
        is_ready &= (entry.inst->src1 < 0) || entry.src1_ready;
        is_ready &= (entry.inst->src2 < 0) || entry.src2_ready;
        if(entry.is_load_store()){// for load and store 
            is_ready &= entry.ms_ready;
        }
        return is_ready;
    }
    // remove entry from schedule queue
    void remove(schedule_queue_entry& entry){
        for(uint64_t i=0;i< schedule_q.size();i++){
            if(schedule_q[i].inst == entry.inst){
                #ifdef DEBUG
                printf("Removing instruction with tag %ld from the scheduling queue\n",schedule_q[i].tag2);// use the bigger tag
                #endif
                schedule_q.erase(schedule_q.begin() + i);      
                break;
            }
        }
    }
    // update scheduling queue using completed instructions
    void update(schedule_queue_entry& entry){
        uint64_t tag1 = entry.tag1;
        uint64_t tag2 = entry.tag2;
        for(auto & sq_entry : schedule_q){
            // set register source    
            if(tag2 == sq_entry.src1_tag) {
                sq_entry.src1_ready = true;         // check source reg1
                #ifdef DEBUG
                printf("Setting rs1_ready = 1 for rd_tag %ld from the scheduling queue\n",tag2);
                #endif
            }
            if(tag2 == sq_entry.src2_tag){
                sq_entry.src2_ready = true;         // check source reg2 
                #ifdef DEBUG
                printf("Setting rs2_ready = 1 for rd_tag %ld from the scheduling queue\n",tag2);
                #endif
            }
            if(sq_entry.is_load_store()){
                if(tag1 == sq_entry.ms_tag) {
                    sq_entry.ms_ready = true;       // check load/store register
                    #ifdef DEBUG
                    printf("Setting ms_ready = 1 for md_tag %ld from the scheduling queue\n",tag1);
                    #endif
                }
            }
        }
    }
};

struct function_unit
{
    size_t alu_num;
    size_t alu_current;
    std::vector<schedule_queue_entry> alu_vec;

    size_t mul_num;
    uint64_t mul_current;   // use the first 0 bits： when mul_num = 3, 001 use the second mul fu. 000 use the first mul fu. 111 no fu can be used
    std::vector<std::deque<schedule_queue_entry>> mul_vec;
    uint64_t mul_mask;      // 0b11111 all 1 for number of mul fus

    size_t lsu_num;
    size_t lsu_current;
    std::vector<schedule_queue_entry> lsu_vec;
    function_unit(){}
    function_unit(size_t alu_num, size_t mul_num, size_t lsu_num): alu_num(alu_num),alu_current(0), mul_num(mul_num), lsu_num(lsu_num),lsu_current(0){
        schedule_queue_entry entry = schedule_queue_entry();
        entry.finished = true;
        mul_mask = (1 << mul_num) - 1;  // mul fu mask, to check whether mul fu has room
        mul_current = 0;
        alu_vec.resize(alu_num, entry); // initialize alu_vec using entry
        mul_vec.resize(mul_num);        // initialize mul_vec using {}
        lsu_vec.resize(lsu_num, entry); // initialize lsu_vec using entry
    }
    // whether entry can be push into fu
    bool fu_has_room(schedule_queue_entry entry){
        switch (entry.inst->opcode)
        {
        case 2:     // ALU: for Add
        case 6:     // for Branch
            return (alu_current < alu_num);
            break;
        case 3:     // MUL: for Mul, Divide, Float
            return mul_current < mul_mask;
        case 4:     // LSU: for load
        case 5:     // for store
            return (lsu_current < lsu_num);
            break;
        default:    // error opcode
            break;
        }
        return 0;
    }
    // push entry into corresponding fu
    void push(schedule_queue_entry entry){
        // need to change the number
        switch (entry.inst->opcode)
        {
        case 2:     // ALU: for Add
        case 6:     // for Branch
            for(uint64_t i=0;i<alu_vec.size();i++){
                if(alu_vec[i].finished){
                    #ifdef DEBUG
                    printf("\t\tFired to ALU #%ld\n", i);
                    #endif
                    alu_vec[i] = entry;
                    alu_current++;
                break;
                }
            }
            break;
        case 3:{     // MUL: for Mul, Divide, Float
            uint64_t mul_index = 0;
            uint64_t mul_loop = mul_current;
            while(mul_loop){
                mul_loop >>=1;
                mul_index++;
            }
            if(mul_index < mul_num){
                #ifdef DEBUG
                printf("\t\tFired to MUL #%ld\n", mul_index);  
                #endif
                mul_vec[mul_index].push_back(entry);
                mul_current |= 1 << (mul_index);
            }
            break;
        }
        case 4:     // LSU: for load
        case 5:     // for store
            for(uint64_t i=0;i<lsu_vec.size();i++){
                if(lsu_vec[i].finished){
                    #ifdef DEBUG
                    printf("\t\tFired to LSU #%ld\n", i);
                    #endif
                    lsu_vec[i] = entry;
                    lsu_current++;
                break;
                }
            }
            break;
        default:    // error opcode
            break;
        }
    }
    // check fu for completed instructions
    void fu_check(uint64_t fu_type, std::vector<schedule_queue_entry>& complete_vec){
        switch (fu_type)
        {
        case 1:     // ALU: 1 stage
            for(uint64_t i=0;i<alu_vec.size();i++){
                if(alu_vec[i].finished) continue;   // ignore the finished entry
                if(++alu_vec[i].fu_count == 1){
                    #ifdef DEBUG
                    printf("\tCompleting ALU: %ld, for dyncount=%ld\n", i, alu_vec[i].inst->dyn_instruction_count);
                    #endif
                    complete_vec.push_back(alu_vec[i]);
                    alu_vec[i].finished = true;
                    alu_current--;
                }
            }
            break;
        case 2:     // MUL: 3 stage
            for(uint64_t i=0;i<mul_vec.size();i++){
                for(uint64_t j=0; j<mul_vec[i].size();){
                    auto& current_entry = mul_vec[i][j];
                    current_entry.fu_count++;
                    if(j == 0 && current_entry.fu_count == 3){  // only the first enty has chance to finish, no need to compare other entrys
                        #ifdef DEBUG
                        printf("\tCompleting MUL: %ld, for dyncount=%ld\n", i, current_entry.inst->dyn_instruction_count);
                        #endif
                        complete_vec.push_back(current_entry);
                        mul_vec[i].pop_front();
                    }
                    else j++;
                }
            }
            break;
        case 3:     // LSU: 1 stage
            for(uint64_t i=0;i<lsu_vec.size();i++){
                if(lsu_vec[i].finished) continue;
                lsu_vec[i].fu_count++;
                if(lsu_vec[i].inst->opcode == OPCODE_STORE){// for store instruction
                    if(lsu_vec[i].fu_count == 1){
                        #ifdef DEBUG
                        printf("\tCompleting LSU: %ld, for dyncount=%ld\n", i, lsu_vec[i].inst->dyn_instruction_count);
                        #endif
                        complete_vec.push_back(lsu_vec[i]);
                        lsu_vec[i].finished = true;
                        lsu_current--;
                    }
                    else{
                        #ifdef DEBUG
                        printf("\tLSU: %ld, needs to wait %ld cycles to complete,%ld\n",i, 1-lsu_vec[i].fu_count, lsu_vec[i].fu_count);
                        #endif
                    }
                }
                else{// for load instruction
                    uint64_t cycles_needed = 2;
                    switch (lsu_vec[i].inst->dcache_hit)
                    {
                    case CACHE_LATENCY_L1_HIT:
                        cycles_needed = 2;
                        break;
                    case CACHE_LATENCY_L2_HIT:
                        cycles_needed = 10;
                        break;
                    case CACHE_LATENCY_L2_MISS:
                        cycles_needed = 100;
                        break;
                    default:
                        break;
                    }
                    if(lsu_vec[i].fu_count == cycles_needed){
                        #ifdef DEBUG
                        printf("\tCompleting LSU: %ld, for dyncount=%ld\n", i, lsu_vec[i].inst->dyn_instruction_count);
                        #endif
                        complete_vec.push_back(lsu_vec[i]);
                        lsu_vec[i].finished = true;
                        lsu_current--;
                    }
                    else{
                        #ifdef DEBUG
                        printf("\tLSU: %ld, needs to wait %ld cycles to complete\n",i, cycles_needed-lsu_vec[i].fu_count);
                        #endif
                    }
                }
            }
            break;
        default:
            break;
        }
        
    }
};

struct register_file_entry{
    bool ready;
    uint64_t tag;
    //uint64_t value;   no need value for simulation
    register_file_entry(): ready(true), tag(0){}
    register_file_entry(bool r, uint64_t t) : ready(r), tag(t) {}
};

struct register_file{
    size_t rf_entries;
    std::vector<register_file_entry> rf;
    register_file(): rf_entries(0){}
    register_file(size_t reg_num): rf_entries(reg_num){
        rf.resize(reg_num);                 // resize the register file
        for(size_t i=0; i<reg_num ;i++){
            rf[i] = register_file_entry(1,i); // ready 1, tag = i, value = 0
        }
    }

    // lookup register file to assign value to schedule queue entry;
    void lookup(schedule_queue_entry & entry){
        if(entry.inst->src1 >= 0){     // skip the -1 src register
            entry.src1_ready = rf[entry.inst->src1].ready;
            entry.src1_tag = rf[entry.inst->src1].tag;
            #ifdef DEBUG
            printf("\t\tInstr has src1, setting src1_tag=%ld, src1_ready=%d\n",entry.src1_tag, entry.src1_ready);
            #endif
        }
        else{
            #ifdef DEBUG
            printf("\t\tInstr does not have src1, setting src1_tag=0, src1_ready=1\n");
            #endif
        }
        if(entry.inst->src2 >= 0){     // skip the -1 src register
            entry.src2_ready = rf[entry.inst->src2].ready;
            entry.src2_tag = rf[entry.inst->src2].tag;
            #ifdef DEBUG
            printf("\t\tInstr has src2, setting src2_tag=%ld, src2_ready=%d\n",entry.src2_tag, entry.src2_ready);
            #endif
        }
        else{
            #ifdef DEBUG
            printf("\t\tInstr does not have src2, setting src2_tag=0, src2_ready=1\n");
            #endif
        }
    }
    // update register file use completed instructions
    void update(const schedule_queue_entry& entry){
        int8_t dest_reg = entry.inst->dest;
        if(dest_reg >= 0 && rf[dest_reg].tag == entry.tag2){
            rf[dest_reg].ready = 1;
            #ifdef DEBUG
            printf("Instr has dest, marking ready=1 for R%d (tag = %ld) in messy RF\n", dest_reg, entry.tag2);
            #endif
        }
    }
};

struct rob{
    size_t rob_entries;
    std::deque<schedule_queue_entry> rob_;
    rob(): rob_entries(0){}
    rob(size_t num_rob_entries): rob_entries(num_rob_entries){}      // set the entry number

    // push schedule_queue_entry into rob
    void push(schedule_queue_entry entry){
        rob_.push_back(entry);
    }

    // whether rob has room to push
    bool full(){
        return rob_.size() >= rob_entries;
    }
    // update rob use completed instructions: set finish of completed instruction
    void update(const schedule_queue_entry& entry){
        for(auto & rob_entry: rob_){
            if(entry.inst == rob_entry.inst){
                #ifdef DEBUG
                printf("Setting instruction with rd_tag %ld as done in the ROB\n",entry.tag2);
                #endif
                rob_entry.finished = 1;   
                break;
            }
        }
    }
    // retire the completed instructions in rob
    uint64_t retire(procsim_stats_t *stats, bool *retired_mispredict_out, uint64_t retire_width){
        #ifdef DEBUG
        printf("Checking ROB: \n");
        #endif
        uint64_t retire_count = 0;
        // retire up tp retire_width instructions
        for(uint64_t i=0;i<rob_.size() && retire_count< retire_width;){
            auto& current_entry = rob_[i];
            if(current_entry.finished){
                #ifdef DEBUG
                printf("\tRetiring: ");print_instruction(current_entry.inst);printf("\n");
                #endif
                if(current_entry.inst->opcode == OPCODE_BRANCH){// retire a mispredict branch, update branch predictor
                    stats->num_branch_instructions++;// add total branch 
                    procsim_driver_update_predictor( current_entry.inst->pc, current_entry.inst->br_taken, current_entry.inst->dyn_instruction_count);
                    retire_count++;
                    if(current_entry.inst->mispredict){
                        *retired_mispredict_out = true;
                        rob_.pop_front();
                        break;      // stop retire when retire a branch misprediction
                    }
                    rob_.pop_front();
                }
                else{
                    retire_count++;
                    rob_.pop_front();       // retire this instruction
                }
            }
            else break;
        }
        stats->instructions_retired += retire_count;
        #ifdef DEBUG
        if(rob_.size() && retire_count< retire_width){
            printf("\tROB entry still in flight: dyncount=%ld\n",rob_[0].inst->dyn_instruction_count);
        }
        #endif
        #ifdef DEBUG
        printf("\t%ld instructions retired\n", retire_count);
        #endif
        return retire_count;
    }
};

struct memory_alias_table{  // write this later
    size_t mat_entries;
    std::vector<register_file_entry> mat;
    memory_alias_table(){}
    memory_alias_table(uint64_t mat_entries): mat_entries(mat_entries){
        mat.resize(mat_entries);
        for(uint64_t i=0;i<mat.size();i++){
            mat[i] = register_file_entry(1,32+i);   // ready, tag = 32+i
        }
    }
    // look up the schedule queue to set entry information
    void lookup(schedule_queue_entry & entry){
        if(!entry.is_load_store()) return;
        // load or store instruction
        entry.ms_ready = mat[entry.ms].ready;
        entry.ms_tag = mat[entry.ms].tag;
        #ifdef DEBUG
        printf("\t\tInstr has load_store_addr=0x%lx, addr_class=0x%x, setting ms_tag=%ld, ms_ready=%d, md_tag=%ld\n",
           entry.inst->load_store_addr, entry.md, entry.ms_tag, entry.ms_ready, entry.tag1);
        #endif
    }
    // update the mat register use completed instructions
    void update(const schedule_queue_entry& entry){
        if(entry.is_load_store()){
            int8_t dest_reg = entry.md;
            if(mat[dest_reg].tag == entry.tag1){
                mat[dest_reg].ready = 1;              // set destination register to ready
                #ifdef DEBUG
                printf("Instr is load/store, marking ready=1 for M%d (tag = %ld) in MAT\n", dest_reg, mat[dest_reg].tag);
                #endif
            }
        }   
    }
};

struct procsim{
    const procsim_conf_t* config;     // save the config
    uint64_t tag;               // start tag
    size_t disp_q_entries;      
    dispatch_queue disp_q;
    
    size_t sche_q_entries;
    schedule_queue sche_q;
    
    size_t fetch_width;
    size_t dispatch_width;
    size_t retire_width;

    size_t alu_num;
    size_t mul_num;
    size_t lsu_num;
    
    function_unit fu;

    register_file rf;   // entry number is REG_NUM
    
    size_t rob_entries;
    rob rob_file;
    memory_alias_table mat;
    driver_read_status_t inst_read_status;
    procsim(){}
    procsim(const procsim_conf_t *config): 
        config(config), 
        tag(96),    // initialize to 96 
        fetch_width(config->fetch_width),
        dispatch_width(config->dispatch_width),
        retire_width(config->retire_width),
        alu_num(config->num_alu_fus), 
        mul_num(config->num_mul_fus), 
        lsu_num(config->num_alu_fus),
        rob_entries(config->num_rob_entries){
        // initialize dispatch queue: queue
        disp_q_entries = config->fetch_width * REG_NUM;     
        disp_q = dispatch_queue(disp_q_entries);
        // initialize schedule queue: vector
        sche_q_entries = config->num_schedq_entries_per_fu * (config->num_alu_fus + config->num_mul_fus + config->num_lsu_fus);
        sche_q = schedule_queue(sche_q_entries);
        // initialize function unit
        fu = function_unit(config->num_alu_fus,config->num_mul_fus, config->num_lsu_fus);
        // initialize register file: vector 
        rf = register_file(REG_NUM);
        // initialize rob: vector
        rob_file = rob(rob_entries);
        // initialize mat
        mat = memory_alias_table(MAT_REG_NUM);
        inst_read_status = DRIVER_READ_OK;
    }
    // fetch instruction insert into dispatch queue
    void fetch(procsim_stats_t *stats){   
        size_t fetch_count = 0;
        while(fetch_count < fetch_width && disp_q.dispatch_q.size() < disp_q_entries){   // stop fetch when one of condition match
            #ifdef DEBUG
            driver_read_status_t pre_read_status = inst_read_status;
            #endif
            const inst_t* inst = procsim_driver_read_inst(&inst_read_status);
            fetch_count++;
            if(inst){
                #ifdef DEBUG
                printf("\tFetched Instruction: ");print_instruction(inst);printf("\n");
                if(pre_read_status == DRIVER_READ_ICACHE_MISS){
                    printf("\t\tI-Cache miss repaired by driver\n");
                }
                if(inst->opcode == OPCODE_BRANCH && inst->mispredict) {
                    printf("\t\tBranch Misprediction will be handled by driver\n");
                }
                #endif
                // insert instruction into dispatch queue
                disp_q.insert(inst);
                //cout << "fetch normal inst"<<endl;
                stats->instructions_fetched++;
            }
            else{// NOP inst
                switch (inst_read_status)
                {
                case DRIVER_READ_MISPRED:
                    #ifdef DEBUG
                    printf("\tFetched NOP because of branch misprediction\n");
                    #endif
                    break;
                case DRIVER_READ_ICACHE_MISS:
                    #ifdef DEBUG
                    printf("\tFetched NOP because of icache miss\n");
                    printf("\tPushed an NOP into the dispatch queue on icache miss\n");
                    #endif
                    disp_q.insert(inst);
                    // This icache nop don't count as stats instruction fetched. don't add
                    break;
                case DRIVER_READ_END_OF_TRACE:
                    // not instruction to fetch
                    #ifdef DEBUG
                    printf("\tFetched NOP because of end of trace is reached\n");
                    #endif
                    break;
                default:
                    break;
                }
            }
        }
        if(fetch_count < fetch_width && disp_q.dispatch_q.size() == disp_q_entries){
            #ifdef DEBUG
            printf("\tNot fetching because the dispatch queue is full\n");
            #endif
        }
    }

    // trace format:        <Address> <Opcode> <Dest Reg #> <Src1 Reg #> <Src2 Reg #> <LD/ST Addr> <Branch Taken> <Branch Target> <I-Cache Miss> <D-Cache Miss> <Dynamic Instruction Count>
    // add    instruction : 0x22918     2           10          23            -1            0x0         0               0x0             0               0                   11
    // mul    instruction : 0x13758     3           15          15            14            0x0         0               0x0             0               0                   170
    // load   instruction : 0x228f4     4           1           2             -1            0x3fffff60e8 0              0x0             0               0                   2
    // store  instruction : 0x1a01c     5           -1          2             17            0x3fffff6118 0              0x0             0               0                   34
    // branch instruction : 0x228cc     6           -1          15            0             0x0         1               0x228f4         0               0                   1
    // look throught register file and return the entry for inst
    // modify the register file to modify the tag of dest register
    schedule_queue_entry create_entry(const inst_t* inst){
        schedule_queue_entry entry(inst);
        
        entry.tag1 = tag++;
        mat.lookup(entry);  // set source register ready, tag for load or store
        // modify the MAT
        if(entry.is_load_store()){
            mat.mat[entry.md].ready = 0;
            mat.mat[entry.md].tag = entry.tag1;
            #ifdef DEBUG
            printf("\t\tIncrease tag counter by 1\n");
            printf("\t\tInstr is load/store, marking ready=0 and assigning tag=%ld for M%d in MAT\n",entry.tag1, entry.md);
            #endif
        }else{
            #ifdef DEBUG
            printf("\t\tInstr is not load/store, setting ms_tag=0, ms_ready=1, md_tag=%ld\n",entry.tag1);
            printf("\t\tIncrease tag counter by 1\n");
            #endif
        }

        entry.tag2 = tag++;
        rf.lookup(entry);   // get the ready, tag, value of source registers
        #ifdef DEBUG
        printf("\t\tsetting rd_tag=%ld\n", entry.tag2);
        #endif
        #ifdef DEBUG
        printf("\t\tIncrease tag counter by 1\n");
        #endif
        // modify the dest reg
        if(entry.inst->dest >=0){
            #ifdef DEBUG
            printf("\tInstr has dest, marking ready=0 and assigning tag=%ld for R%d in messy RF\n",entry.tag2, entry.inst->dest);
            #endif
            rf.rf[entry.inst->dest].ready = 0;
            rf.rf[entry.inst->dest].tag = entry.tag2;
        }
        
        return entry;
    }
    // look through dispatch queue. create schedule entry. insert into schedule queue, insert into rob
    // stop condition: dispatch_queue empty; sched_queue full; rob full; reach dispath_width
    void dispatch(procsim_stats_t *stats){
        size_t normal_dispatch = 0;
        size_t nop_dispatch = 0;
        //    normal inst not full;               nop dispatch not full ;           dispatch queue has inst;     schedule queue has room                     ;rob has room
        while(normal_dispatch < dispatch_width && nop_dispatch < dispatch_width && !disp_q.dispatch_q.empty() && !sche_q.full() && !rob_file.full()){
            const inst_t* inst = disp_q.dispatch_q.front();     // front inst
            if(inst){       // dispatch normal instruction
                #ifdef DEBUG
                printf("\tAttempting Dispatch for: ");print_instruction(inst);printf("\n");
                #endif
                schedule_queue_entry entry = create_entry(inst);
                sche_q.push(entry);         // push into schedule queue
                rob_file.push(entry);        // push into rob
                disp_q.dispatch_q.pop();
                normal_dispatch++;
                #ifdef DEBUG
                printf("\tDispatching and adding the instruction to the scheduling queue\n");
                #endif
            }
            else{// ignore NOP inst
                disp_q.dispatch_q.pop();
                nop_dispatch++; // just count and do nothing
            }
        }
        // add cycles which stop only due to rob full.
        if(normal_dispatch < dispatch_width && nop_dispatch < dispatch_width && !disp_q.dispatch_q.empty() && !sche_q.full() ) {
            stats->rob_no_dispatch_cycles++;
            #ifdef DEBUG
            printf("\tDispatch stalled due to insufficient ROB space\n");
            #endif
        }
        #ifdef DEBUG
        printf("\t%ld instructions dispatched and added to the scheduling queue\n", normal_dispatch);
        #endif
    }
    // look through the schedule queue and find the inst to fire
    // inst can fire: source registers ready, FU ready
    void schedule(procsim_stats_t *stats){
        uint64_t fire_count = 0;
        fu.mul_current = 0;
        for(uint64_t i=0;i<sche_q.schedule_q.size();i++){
            schedule_queue_entry entry = sche_q.schedule_q[i];
            //print_instruction(entry.inst);cout <<" "<< entry.src1_ready<<" "<< entry.ms_ready <<" " << sche_q.inst_ready(i) <<endl;
            if(!sche_q.schedule_q[i].scheduled && sche_q.inst_ready(i)){   // ready to fire
                #ifdef DEBUG
                printf("\tAttempting to fire instruction: ");print_instruction(entry.inst);printf(" to \n");
                #endif
                if(fu.fu_has_room(entry.inst)){
                    fu.push(entry);     // fire to fu
                    sche_q.schedule_q[i].scheduled = true;
                    //sche_q.remove(i);   // delete from schedule queue
                    fire_count++;
                }
                else{
                    #ifdef DEBUG
                    printf("\t\tCannot fire instruction\n");
                    #endif
                }
            }
        }
        // add stats when no fire inst a cycle
        if(!fire_count) {   // no fire
            stats->no_fire_cycles++;
            #ifdef DEBUG
            printf("\tCould not find scheduling queue entry to fire this cycle\n");
            #endif
        }
        
        #ifdef DEBUG
        printf("\t%ld instructions fired\n", fire_count);
        #endif 
    }

    // Find completed instruction in the schedule queue, push into complete vec, remove from schedule queue
    void execute(int64_t fu_type, std::vector<schedule_queue_entry>& complete_vec){
        fu.fu_check(fu_type, complete_vec);
    }
    // update the schedule queue and register file use completed instructions
    void result_bus(std::vector<schedule_queue_entry>& complete_vec){
        for(schedule_queue_entry& entry : complete_vec){
            #ifdef DEBUG
            printf("\tProcessing Result Bus for: ");print_instruction(entry.inst);printf("\n");
            #endif
            mat.update(entry);
            // update register file
            rf.update(entry);
            // remove entry from schedule queue
            sche_q.remove(entry);
            // update schedule queue
            sche_q.update(entry);
            // update rob
            rob_file.update(entry);
        }
    }

    // retire completed instructionf from rob
    uint64_t retire(procsim_stats_t *stats, bool *retired_mispredict_out){
        return rob_file.retire(stats, retired_mispredict_out, retire_width);
    }
};


#endif
