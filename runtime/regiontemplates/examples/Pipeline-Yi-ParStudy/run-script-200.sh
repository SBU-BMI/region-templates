#!/bin/bash
#SBATCH -J Yi-Pipeline           # job name
#SBATCH -o Yi-Pipeline-8-lhs.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 5              # total number of mpi tasks requested
#SBATCH -N 5            # total number of mpi tasks requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 48:00:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A TG-ASC130023
##ibrun ls

source ~/.bash_profile 
source ~/.profile 

OUTDIR=.

#dakota -i vbd200.in -r vbd_200-2.rst -w vbd_200-3.rst -o vbd_200_1WSI-3.out > vbd_200_1WSI-3.stdout
dakota -i vbd200.in -r vbd_200-3.rst -w vbd_200-4.rst -o vbd_200_1WSI-4.out > vbd_200_1WSI-4.stdout


