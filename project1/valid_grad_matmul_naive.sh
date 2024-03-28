#!/bin/bash

# Define ranges for -b, -s, and -S parameters
b_values=(5 6 7)
s_values=(0 0 0 0 1 1 1 1 2 2 2 2 3 3 3 3 4 4 4 4 5 5 5 5)  # Requirement: S+B >= 5
S_values=(2 3 4 5 2 3 4 5 2 3 4 5 3 4 5 6 4 5 6 7 5 6 7 8)  # Requirement: S+B >= 7

insert_policy=(mip lip)
replacement_policy=(lru lfu)
prefetch_mode=(0 1 2)
# Array of trace files
trace_files=("matmul_naive.trace")  # Add more trace files as needed

#trace_files=("gcc.trace" "leela.trace" "linpack.trace" "matmul_naive.trace" "matmul_tiled.trace" "mcf.trace")


# Path to the trace files
trace_path="./traces"

# Loop over each b value
for b in "${b_values[@]}"; do
    # Loop over indices of s and S values
    for i in {0..11}; do
        s=${s_values[i]}
        S=${S_values[i]}
        
        # Now loop over each trace file
        for trace in "${trace_files[@]}"; do
            for insert in "${insert_policy[@]}"; do
                for r in "${replacement_policy[@]}"; do
                    for p in "${prefetch_mode[@]}"; do
                        ./cachesim -c 15 -b "$b" -s "$s" -C 17 -S "$S" -P "$p" -I "$insert" -r "$r" -f "$trace_path/$trace"
                        # Print the command with dynamic variables
                        echo "Ran: -c 15 -b $b -s $s -C 17 -S $S -P $p -I $insert -r $r -f $trace_path/$trace"
                        sleep 1
                    done
                done
            done
        done
    done
done
