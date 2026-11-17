#!/bin/bash


matrix_sizes=(1000 4000 10000)
process_counts=(2 4 8)

output_file="mpi_test_results1.txt"
echo "MPI Test Results" > $output_file
echo "=================" >> $output_file


nodes=($(cat machines))  
num_nodes=${#nodes[@]}   

specific_node_counts=(1 5 15 30)

hostfile="hostfile"
> $hostfile  
for node in "${nodes[@]}"; do
    echo "$node" >> $hostfile  
done


for n in "${matrix_sizes[@]}"; do
    for p in "${process_counts[@]}"; do
        for node_count in "${specific_node_counts[@]}"; do
            if [ $node_count -le $num_nodes ]; then
                echo "Running test: n=$n, processes=$p, nodes=$node_count"
                echo "Test: n=$n, processes=$p, nodes=$node_count" >> $output_file
                echo "n=$n, processes=$p, nodes=$node_count" >> $output_file
                
                mpiexec -n $p -hostfile $hostfile ./exercise_2 -n $n >> $output_file
                echo "---------------------------------" >> $output_file
            else
                echo "Skipping test: n=$n, processes=$p, nodes=$node_count (not enough nodes)" >> $output_file
            fi
        done
    done
done

echo "All tests completed. Results saved in $output_file"
