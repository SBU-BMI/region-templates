#!/bin/bash
#SBATCH -J Pipeline           # job name
#SBATCH -o Pipeline-65.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 129              # total number of mpi tasks requested
#SBATCH -N 129             # total number of mpi tasks requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 00:40:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A TG-ASC130023
##ibrun ls

#mpirun -np 2 ./PipelineManager /work/02542/gteodoro/images/128/64.1/32.1/16.1/8.1/4.1/1.1/  -c 14 -g 1 -m 1 -s speedup_multqueues -w 80 
INPUTDIR=/scratch/02542/gteodoro/tiled2/
NODES=129

export OMP_NUM_THREADS=16

source ~/.bash_profile 
cores=16
config1=cacheaware-input
config2=fcfs-input
config3=fcfs-noinput
config4=cacheaware-noinput

cache=2l-1GB
for rep in 1 2 3; do
	rm rtconf.xml
	cp cache-conf/rtconf1-1-level-1TB.xml rtconf.xml
	OUTDIR=results/tiled/${cores}c/${config3}-delete-no/1l-GB-x/${NODES}-nodes/${rep}/
	mkdir -p ${OUTDIR}
	rm /scratch/02542/gteodoro/temp/*
	ibrun   tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -x -i ${INPUTDIR} &> ${OUTDIR}/out.txt
done

#for cacheSize in 1 2 4; do
#	rm rtconf.xml
#	cp cache-conf/rtconf2-level-${cacheSize}GB.xml rtconf.xml
#
#	for rep in 1 2 3; do
#		OUTDIR=results/100/${cores}c/${config1}-nodelete/2l-${cacheSize}-GB/${NODES}-nodes/${rep}/
#		mkdir -p ${OUTDIR}
#		rm /scratch/02542/gteodoro/temp/*
#		ibrun   tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -x -r -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#	
##		OUTDIR=results/tiled2/${cores}c/${config2}/2l-${cacheSize}-GB/${NODES}-nodes/${rep}/
##		mkdir -p ${OUTDIR}
##		rm /scratch/02542/gteodoro/temp/*
##		ibrun   tacc_affinity ./HelloWorldRTFS -c ${cores} -r -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#
##		OUTDIR=results/100/${cores}c/${config3}-nodelete/2l-${cacheSize}-GB/${NODES}-nodes/${rep}/
##		mkdir -p ${OUTDIR}
##		rm /scratch/02542/gteodoro/temp/*
##		ibrun   tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#
#	done
#done
