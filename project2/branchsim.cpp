/**
 * @file branchsim.hpp
 * @brief Project 2: Tomasulo & Branch Predictor Simulator
 * 
 * @author Bo Pang
 * @date 2024-04-01
 */
#include "branchsim.hpp"
#include <vector>
#include <iostream>
#include <cstring>
using namespace std;

static uint64_t* history_table;
static uint64_t* pattern_table;
static uint64_t P;
static uint64_t H;
uint64_t PC_mask;    // H one
uint64_t HT_mask;    // P one
// PC hash-> H bits
// In history table, 2**H entries, each P bits
// In pattern table, 2**P entries, each 2 bits
void yeh_patt_init_predictor(branchsim_conf_t *sim_conf) {
    // create history table of 2^H entries (all entries initialized to 0)
    history_table = new uint64_t[(1 << sim_conf->H)]();       // new int[5]{0}
    // create pattern table of 2^P entries init to weakly not taken
    pattern_table = new uint64_t[(1 << sim_conf->P)];
    for (int i = 0; i < (1 << sim_conf->P); ++i) {
        pattern_table[i] = 0b01;
    }
    P = sim_conf->P;
    H = sim_conf->H;
    PC_mask = (1 << H) - 1;
    HT_mask = (1 << P) - 1;
#ifdef DEBUG
    printf("Yeh-Patt: Creating a history table of %" PRIu64 " entries of length %" PRIu64 "\n", (uint64_t)(1 << sim_conf->H), P); // TODO: FIX ME
    printf("Yeh-Patt: Creating a pattern table of %" PRIu64 " 2-bit saturating counters\n", (uint64_t)(1 << sim_conf->P)); // TODO: FIX ME
#endif
}

bool yeh_patt_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tYeh-Patt: Predicting... \n"); // PROVIDED
#endif
    uint64_t PC_hash = (br->ip >> 2) & PC_mask;    // right shift address to get the pc_hash
    uint64_t HT_bits =  history_table[PC_hash] ;
    uint64_t PT_state =  pattern_table[HT_bits] ;
    

#ifdef DEBUG
    printf("\t\tHT index: 0x%" PRIx64 ", History: 0x%" PRIx64 ", PT index: 0x%" PRIx64 ", Prediction: %d\n", PC_hash, HT_bits, HT_bits, PT_state >= 0b10); // TODO: FIX ME
#endif
    return PT_state >= 0b10;    
}

uint64_t state_switch(uint64_t PT_state, branch_t *br){
    switch(PT_state){     // this can be done using logic expression
        case 0b11:
            if(br->is_taken) PT_state = 0b11;
            else PT_state = 0b10;
            break;
        case 0b10:
            if(br->is_taken) PT_state = 0b11;
            else PT_state = 0b01;
            break;
        case 0b01:
            if(br->is_taken) PT_state = 0b10;
            else PT_state = 0b00;
            break;
        case 0b00:
            if(br->is_taken) PT_state = 0b01;
            else PT_state = 0b00;
            break;
    }
    return PT_state;
}
void yeh_patt_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tYeh-Patt: Updating based on actual behavior: %d\n", (int) br->is_taken); // PROVIDED
#endif
    uint64_t PC_hash = (br->ip >> 2) & PC_mask;
    uint64_t HT_bits = history_table[PC_hash] & HT_mask;
    uint64_t PT_state =  pattern_table[HT_bits];
    history_table[PC_hash] = ((HT_bits << 1) + br->is_taken) & HT_mask;   // Update history table
    pattern_table[HT_bits] = state_switch(PT_state, br);    // Update Pattern table
#ifdef DEBUG
    printf("\t\tHT index: 0x%" PRIx64 ", History: 0x%" PRIx64 ", PT index: 0x%" PRIx64 ", New Counter: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n", PC_hash, HT_bits, HT_bits, pattern_table[HT_bits], history_table[PC_hash]); // TODO: FIX ME
#endif
}

void yeh_patt_cleanup_predictor() {
    // add your code here
    delete[] history_table;
    delete[] pattern_table;
}

// static uint64_t* pattern_table; already defined
// static uint64_t P;   already defined
static uint64_t GHR; // P bits
// uint64_t PC_mask;    // H one already defined
uint64_t GHR_mask;    // P one

void gshare_init_predictor(branchsim_conf_t *sim_conf) {
    // create pattern table of 2^P entries init to weakly not taken
    pattern_table = new uint64_t[(1 << sim_conf->P)];
    P = sim_conf->P;
    for(int i=0;i < (1 << sim_conf->P);i++){
        pattern_table[i] = 0b01;
    }
    // initialize GHR to 0
    GHR = 0;
    PC_mask = (1<<P) - 1;   // P bits of 1
    GHR_mask = PC_mask;
#ifdef DEBUG
    printf("Gshare: Creating a pattern table of %" PRIu64 " 2-bit saturating counters\n", (uint64_t)(1 << sim_conf->P)); // TODO: FIX ME
#endif
}

bool gshare_predict(branch_t *br) {
#ifdef DEBUG
    printf("\tGshare: Predicting... \n"); // PROVIDED
#endif
    uint64_t PC_hash = (br->ip >> 2) & PC_mask;
    // add your code here
    uint64_t Hash_share = PC_hash ^ GHR;
    uint64_t PT_state = pattern_table[Hash_share];
#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", Prediction: %d\n", GHR, Hash_share, PT_state >= 0b10);
#endif
    return PT_state >= 0b10; // TODO: FIX ME
}

void gshare_update_predictor(branch_t *br) {
#ifdef DEBUG
    printf("\tGshare: Updating based on actual behavior: %d\n", (int) br->is_taken); // PROVIDED
    uint64_t history_GHR = GHR; 
#endif

    uint64_t PC_hash = (br->ip >> 2) & PC_mask;
    // add your code here
    uint64_t Hash_share = PC_hash ^ GHR;
    uint64_t PT_state = pattern_table[Hash_share];
    GHR = ((GHR << 1) + br->is_taken) & GHR_mask;               // update the GHR
    pattern_table[Hash_share] = state_switch(PT_state, br);     // Update the pattern table

#ifdef DEBUG
    printf("\t\tHistory: 0x%" PRIx64 ", Counter index: 0x%" PRIx64 ", New Counter value: 0x%" PRIx64 ", New History: 0x%" PRIx64 "\n", history_GHR, Hash_share, pattern_table[Hash_share], GHR); // TODO: FIX ME
#endif
}

void gshare_cleanup_predictor() {
    // add your code here
    delete[] pattern_table;
}

/**
 *  Function to update the branchsim stats per prediction
 *
 *  @param prediction The prediction returned from the predictor's predict function
 *  @param br Pointer to the branch_t that is being predicted -- contains actual behavior
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_update_stats(bool prediction, branch_t *br, branchsim_stats_t *sim_stats) {
    sim_stats->num_branch_instructions++;
    if(prediction == br->is_taken) sim_stats->num_branches_correctly_predicted++;
    else sim_stats->num_branches_mispredicted++;
}

/**
 *  Function to cleanup branchsim statistic computations such as prediction rate, etc.
 *
 *  @param sim_stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_finish_stats(branchsim_stats_t *sim_stats) {
    sim_stats->fraction_branch_instructions = (double)sim_stats->num_branch_instructions / sim_stats->total_instructions;
    sim_stats->prediction_accuracy = (double) sim_stats->num_branches_correctly_predicted / sim_stats->num_branch_instructions;
}


