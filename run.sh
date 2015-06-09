#!/bin/bash
#SBATCH -J myMPI           # job name
#SBATCH -o myMPI.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 2              # total number of mpi tasks requested
#SBATCH -N 2             # total number of mpi tasks requested
#SBATCH -p development     # queue (partition) -- normal, development, etc.
#SBATCH -t 01:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes

##ibrun ls
ibrun ./PipelineManager /work/02542/gteodoro/TCGA-02-0001-01Z-00-DX1.svs-tile/  -cpu 15 -gpu 1 -s priority -w 23
