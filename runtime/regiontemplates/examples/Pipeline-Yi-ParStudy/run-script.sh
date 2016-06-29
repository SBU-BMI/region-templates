#!/bin/bash
#SBATCH -J Yi-Pipeline           # job name
#SBATCH -o Yi-Pipeline-8-lhs.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 65              # total number of mpi tasks requested
#SBATCH -N 65            # total number of mpi tasks requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 22:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A TG-ASC130023
##ibrun ls

source ~/.bash_profile 
source ~/.profile 

OUTDIR=.

dakota -i vbd.in -w vbd_50.rst -o vbd_50_1WSI.out > vbd_50_1WSI.stdout


