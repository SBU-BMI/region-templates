#!/bin/bash

BS=( 0 100 )
NS=( 1 2 4 8 16 32 64 128 256)
REP=3

for N in ${NS[@]}; do
    for B in ${BS[@]}; do
        echo "running N${N} B${B}"
        for R in $(seq $REP); do
            mpirun -np 2 --tag-output --bind-to none --oversubscribe \
                ./iwpp ~/Desktop/images/TCGA-02-0058-01A-01-TS1.205d82b6-3bfa-479b-a2b9-68d4ef1fea9a.svs \
                -t $N --npt -d 3 -a 1 -g 1 -c 1 -b $B >> logs/ovhd-prof-n${N}-b${B}.txt;
        done
    done
done


# BS=( 0 5 10 20 30 50 100 ); NS=( 1 2 4 8 16 32 ); for N in ${NS[@]}; do for B in ${BS[@]}; do grep FULL_TIME logs/ovhd-prof-n${N}-b${B}.txt; done; done