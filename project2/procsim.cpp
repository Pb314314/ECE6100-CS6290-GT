#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <list>
#include "procsim.hpp"

#define NUM_REGS 32
#define NUM_MATS 64

//
// TODO: Define any useful data structures and functions here
//


// The helper functions in this#ifdef are optional and included here for your
// convenience so you can spend more time writing your simulator logic and less
// time trying to match debug trace formatting! (If you choose to use them)
static procsim processor;

#ifdef DEBUG
static void print_operand(int8_t rx) {
    if (rx < 0) {
        printf("(none)"); //  PROVIDED
    } else {
        printf("R%" PRId8, rx); //  PROVIDED
    }
}

// Useful in the fetch and dispatch stages
void print_instruction(const inst_t *inst) {
    if (!inst) {
        return;
    }
    static const char *opcode_names[] = {NULL, NULL, "ADD", "MUL", "LOAD", "STORE", "BRANCH"};

    printf("opcode=%s, dest=", opcode_names[inst->opcode]); // PROVIDED
    print_operand(inst->dest); // PROVIDED
    printf(", src1="); // PROVIDED
    print_operand(inst->src1); // PROVIDED
    printf(", src2="); // PROVIDED
    print_operand(inst->src2); // PROVIDED
    printf(", dyncount=%lu", inst->dyn_instruction_count); //  PROVIDED
}

// This will print the state of the ROB where instructions are identified by their dyn_instruction_count
static void print_rob(void) {
    size_t printed_idx = 0;
    printf("\tAllocated Entries in ROB: %lu\n", processor.rob_file.rob_.size()); // TODO: Fix Me
    for (uint64_t i=0; i < processor.rob_file.rob_.size(); i++) {
        if (printed_idx == 0) {
            printf("    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", processor.rob_file.rob_[i].inst->dyn_instruction_count, processor.rob_file.rob_[i].finished, processor.rob_file.rob_[i].inst->mispredict); // TODO: Fix Me
        } else if (!(printed_idx & 0x3)) {
            printf("\n    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d }", processor.rob_file.rob_[i].inst->dyn_instruction_count, processor.rob_file.rob_[i].finished, processor.rob_file.rob_[i].inst->mispredict); // TODO: Fix Me
        } else {
            printf(", { dyncount=%05" PRIu64 " completed: %d, mispredict: %d }", processor.rob_file.rob_[i].inst->dyn_instruction_count, processor.rob_file.rob_[i].finished, processor.rob_file.rob_[i].inst->mispredict); // TODO: Fix Me
        }
        printed_idx++;
    }
    if (!printed_idx) {
        printf("    (ROB empty)"); //  PROVIDED
    }
    printf("\n"); //  PROVIDED
}

static void print_messy_rf(void) {
    for (size_t i = 0; i < NUM_REGS; i++) {
        if (!(i & 0x3)) {
            printf("\t");
        } else {
            printf(", ");
        }
            printf("R%" PRIu64 "={tag: %" PRIu64 ", ready: %d}", i, processor.rf.rf[i].tag, processor.rf.rf[i].ready); 
        if ((!((i+1) & 0x3)) && ((i+1) != NUM_REGS)) {
            printf("\n"); // PROVIDED
        }
    }
    printf("\n"); // PROVIDED
}

static void print_mat(void) {
    for (size_t i = 0; i < NUM_MATS; i++) {
        if (!(i & 0x3)) {
            printf("\t");
        } else {
            printf(", ");
        }
            printf("M%" PRIu64 "={tag: %" PRIu64 ", ready: %d}", i, processor.mat.mat[i].tag,processor.mat.mat[i].ready); 
        if ((!((i+1) & 0x3)) && ((i+1) != NUM_MATS)) {
            printf("\n");
        }
    }
    printf("\n"); // PROVIDED
}
#endif


// Optional helper function which retires instructions in ROB in program
// order. (In a real system, the destination register value from the ROB would be written to the
// architectural registers, but we have no register values in this
// simulation.) This function returns the number of instructions retired.
// Immediately after retiring a mispredicting branch, this function will set
// *retired_mispredict_out = true and will not retire any more instructions. 
// Note that in this case, the mispredict must be counted as one of the retired instructions.

// Retire instructions in rob in program order; stop when retired a mispredicting branch
// return the number of instructions retired
static uint64_t stage_state_update(procsim_stats_t *stats, bool *retired_mispredict_out) {
    // TODO: fill me in
    #ifdef DEBUG
        printf("Stage State Update: \n"); //  PROVIDED
    #endif
    return processor.retire(stats, retired_mispredict_out);
}

// Optional helper function which is responsible for moving instructions
// through pipelined Function Units and then when instructions complete (that
// is, when instructions are in the final pipeline stage of an FU and aren't
// stalled there), setting the ready bits in the register file. This function 
// should remove an instruction from the scheduling queue when it has completed.

// do execute for each function unit. Add cycles for each executing instruction 
// If instruction complete: broadcast tag and value to reset schedule queue and register file, remove from schedule queue
// 
static void stage_exec(procsim_stats_t *stats) {
    // TODO: fill me in
#ifdef DEBUG
    printf("Stage Exec: \n"); //  PROVIDED
#endif
    std::vector<schedule_queue_entry> complete_vec;
    uint64_t alu_type = 1;
    uint64_t mul_type = 2;
    uint64_t lsu_type = 3;
    
    // Progress ALUs    
#ifdef DEBUG
    printf("\tProgressing ALU units\n");  // PROVIDED
#endif
    processor.execute(alu_type, complete_vec);
    // Progress MULs
#ifdef DEBUG
    printf("\tProgressing MUL units\n");  // PROVIDED
#endif
    processor.execute(mul_type, complete_vec);
    // Progress LSU loads / stores
#ifdef DEBUG
    printf("\tProgressing LSU units\n");  // PROVIDED
#endif
    processor.execute(lsu_type, complete_vec);
    // Apply Result Busses
#ifdef DEBUG
    printf("\tProcessing Result Busses\n"); // PROVIDED
#endif
    processor.result_bus(complete_vec);
#ifdef DEBUG
    printf("\t%ld instructions completed\n", complete_vec.size()); 
#endif
}

// Optional helper function which is responsible for looking through the
// scheduling queue and firing instructions that have their source pregs
// marked as ready. Note that when multiple instructions are ready to fire
// in a given cycle, they must be fired in program order. 
// Also, load and store instructions must be fired according to the 
// memory disambiguation algorithm described in the assignment PDF. Finally,
// instructions stay in their reservation station in the scheduling queue until
// they complete (at which point stage_exec() above should free their RS).
static void stage_schedule(procsim_stats_t *stats) {

    #ifdef DEBUG
        printf("Stage Schedule: \n"); //  PROVIDED
    #endif
    processor.schedule(stats);
}

// Optional helper function which looks through the dispatch queue, decodes
// instructions, and inserts them into the scheduling queue. Dispatch should
// not add an instruction to the scheduling queue unless there is space for it
// in the scheduling queue and the ROB; 
// effectively, dispatch allocates reservation stations and ROB space for 
// each instruction dispatched and stalls if there any are unavailable. 
// Note the scheduling queue has a configurable size and the ROB has F * 32 entries.
// The PDF has details.
// dispatch width is fetch width

// look through dispatch queue, insert instruction into scheduling queue.
// stop condition: dispatch_q empty, sched_q full, rob full, 
static void stage_dispatch(procsim_stats_t *stats) {
    // TODO: fill me in
    #ifdef DEBUG
        printf("Stage Dispatch: \n"); //  PROVIDED
    #endif
    processor.dispatch(stats);
}

// Optional helper function which fetches instructions from the instruction
// cache using the provided procsim_driver_read_inst() function implemented
// in the driver and appends them to the dispatch queue. 
// The size of the dispatch queue is same as number of rob entries.
// Fetch should not add an instruction to the dispatch queue unless there
// is space for it in the dispatch queue 
// If a NULL pointer is fetched from the procsim_driver_read_inst, and driver_read_status is DRIVER_READ_ICACHE_MISS
// insert a NOP to the dispatch queue
// that NOP should be dropped at the dispatch stage

// fetch instruction using API, insert instructions into dispatch queue
static void stage_fetch(procsim_stats_t *stats) {
    // TODO: fill me in
    #ifdef DEBUG
        printf("Stage Fetch: \n"); //  PROVIDED
    #endif
    processor.fetch(stats); 
}

/*
size_t num_rob_entries;
    size_t num_schedq_entries_per_fu;
    size_t num_alu_fus;
    size_t num_mul_fus;
    size_t num_lsu_fus;
    size_t fetch_width;
    size_t dispatch_width;
    size_t retire_width;
    bool misses_enabled;
*/
// Use this function to initialize all your data structures, simulator state, and statistics.
void procsim_init(const procsim_conf_t *sim_conf, procsim_stats_t *stats) {
    using namespace std;
    processor = procsim(sim_conf);
    #ifdef DEBUG
        printf("\nScheduling queue capacity: %lu instructions\n", processor.sche_q_entries); // TODO: Fix Me
        printf("\n"); //  PROVIDED
    #endif
}

// To avoid confusion, we have provided this function for you. Notice that this
// calls the stage functions above in reverse order!   . This function returns the number of instructions retired, and also
// returns if a mispredict was retired by assigning true or false to
// *retired_mispredict_out, an output parameter.
/**
 * Return number of instructions retired, return if a mispredict was retired by assigning to *retired_mispredict_out
 * @param offset 
 * @return the row pointer at the offset
 */
uint64_t procsim_do_cycle(procsim_stats_t *stats, bool *retired_mispredict_out) {   // call this function each cycle.
    #ifdef DEBUG
        printf("================================ Begin cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
    #endif

    // stage_state_update() should set *retired_mispredict_out for us
    uint64_t retired_this_cycle = stage_state_update(stats, retired_mispredict_out);

    if (*retired_mispredict_out) {
        #ifdef DEBUG
                printf("%" PRIu64 " instructions retired. Retired mispredict, so notifying driver to fetch correctly!\n", retired_this_cycle); //  PROVIDED
        #endif

        // After we retire a misprediction, the other stages don't need to run
        stats->branch_mispredictions++;
    } 
    else {
        #ifdef DEBUG
                printf("%" PRIu64 " instructions retired. Did not retire mispredict, so proceeding with other pipeline stages.\n", retired_this_cycle); //  PROVIDED
        #endif

        // If we didn't retire an interupt, then continue simulating the other pipeline stages
        stage_exec(stats);
        stage_schedule(stats);
        stage_dispatch(stats);
        stage_fetch(stats);
    }
    
    #ifdef DEBUG
        printf("End-of-cycle dispatch queue usage: %lu\n", processor.disp_q.dispatch_q.size()); // TODO: Fix Me
        printf("End-of-cycle sched queue usage: %lu\n", processor.sche_q.schedule_q.size()); // TODO: Fix Me
        printf("End-of-cycle ROB usage: %lu\n", processor.rob_file.rob_.size()); // TODO: Fix Me
        printf("End-of-cycle Messy RF state:\n"); // PROVIDED
        print_messy_rf(); // PROVIDED
        printf("End-of-cycle MAT state:\n"); // PROVIDED
        print_mat(); // PROVIDED
        printf("End-of-cycle ROB state:\n"); // PROVIDED
        print_rob(); // PROVIDED
        printf("================================ End cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
        print_instruction(NULL); // this makes the compiler happy, ignore it
    #endif
    
    stats->cycles++;
    // Set max_usages and avg_usages
    stats->dispq_max_usage = std::max(stats->dispq_max_usage, processor.disp_q.dispatch_q.size());
    stats->schedq_max_usage = std::max(stats->schedq_max_usage, processor.sche_q.schedule_q.size());
    stats->rob_max_usage = std::max(stats->rob_max_usage, processor.rob_file.rob_.size());

    stats->dispq_total_usage += processor.disp_q.dispatch_q.size();
    stats->schedq_total_usage += processor.sche_q.schedule_q.size();
    stats->rob_total_usage += processor.rob_file.rob_.size();

    // Return the number of instructions we retired this cycle (including the interrupt we retired, if there was one!)
    return retired_this_cycle;
}

// Use this function to free any memory allocated for your simulator and to
// calculate some final statistics.
void procsim_finish(procsim_stats_t *stats) {
    // TODO: fill me in
    stats->dispq_avg_usage = static_cast<double>(stats->dispq_total_usage) /static_cast<double>(stats->cycles);
    stats->schedq_avg_usage = static_cast<double>(stats->schedq_total_usage) / static_cast<double>(stats->cycles);
    stats->rob_avg_usage = static_cast<double>(stats->rob_total_usage) / static_cast<double>(stats->cycles);
    stats->ipc = static_cast<double>(stats->instructions_retired)/static_cast<double>(stats->cycles);
}
