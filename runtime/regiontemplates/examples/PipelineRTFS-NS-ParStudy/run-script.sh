#!/bin/bash
#SBATCH -J Pipeline           # job name
#SBATCH -o Pipeline-9.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 2              # total number of mpi tasks requested
#SBATCH -N 2             # total number of mpi tasks requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 04:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A TG-ASC130023
##ibrun ls

INPUTDIR=/scratch/02542/gteodoro/images/
NODES=2
cores=16

export OMP_NUM_THREADS=16

source ~/.bash_profile 
source ~/.profile 

OUTDIR=.

#export | grep LD

#ls -lh /temp/

#ldd /work/02542/gteodoro/dakota/dakota-6.3.0.Linux.x86_64/bin/dakota

#ls -lh /usr/lib64/libblas.so.3
dakota -i dakota_nscale.in -o dakota_nscale_moat_prunned.out > dakota_nscale_moat_prunned.stdout

#ibrun tacc_affinity ./PipelineRTFS-NS-Diff-ParStudy -c ${cores} -i ${INPUTDIR} -a parametersFile.in  &> ${OUTDIR}/out.txt


#config1=cacheaware-input
#config2=fcfs-input
#config3=fcfs-noinput
#config4=cacheaware-noinput
#
#cache=2l-1GB
#for rep in 1 2; do
#	rm rtconf.xml
#
#	cp cache-conf/rtconf1-1-level-1TB.xml rtconf.xml
#	OUTDIR=results/${cores}c/${config3}-delete-noi-vary-B1-1-tif/tiled2-50/1l-GB/${NODES}-nodes/${rep}/
#	mkdir -p ${OUTDIR}
#	rm /scratch/02542/gteodoro/temp/*
#	ibrun   tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#
#done
#
#for cacheSize in 4; do
#	rm rtconf.xml
#	cp cache-conf/rtconf2-level-${cacheSize}GB-lru.xml rtconf.xml
#
#	for rep in 1 2; do
##		OUTDIR=results/${cores}c/${config4}-nodelete-lru/2l-${cacheSize}-GB/${NODES}-nodes/${rep}/
##		mkdir -p ${OUTDIR}
##		rm /scratch/02542/gteodoro/temp/*
##		ibrun tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -i ${INPUTDIR} &> ${OUTDIR}/out.txt
##
#		OUTDIR=results/${cores}c/${config4}-nodelete-vary-B1-1-tif/tiled2-50/2l-${cacheSize}-GB/${NODES}-nodes/${rep}-x/
#		mkdir -p ${OUTDIR}
#		rm /scratch/02542/gteodoro/temp/*
#		ibrun tacc_affinity ./PipelineRTFS-NS-Diff -x -c ${cores} -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#
##		OUTDIR=results/${cores}c/${config4}-nodelete-vary-B4-1/tiled2-50/2l-${cacheSize}-GB/${NODES}-nodes/${rep}-x-r/
##		mkdir -p ${OUTDIR}
##		rm /scratch/02542/gteodoro/temp/*
##		ibrun tacc_affinity ./PipelineRTFS-NS-Diff -x -r -c ${cores} -i ${INPUTDIR} &> ${OUTDIR}/out.txt
##
##
#
##		OUTDIR=results/${cores}c/${config4}-nodelete/2l-${cacheSize}-GB/${NODES}-nodes/${rep}-r-x/
##		mkdir -p ${OUTDIR}
##		rm /scratch/02542/gteodoro/temp/*
##		ibrun tacc_affinity ./PipelineRTFS-NS-Diff -c ${cores} -x -r -i ${INPUTDIR} &> ${OUTDIR}/out.txt
#	done
#done
