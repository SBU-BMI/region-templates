#!/bin/bash
#SBATCH -J Pipeline           # job name
#SBATCH -o Pipeline-128-lhs.o%j       # output and error file name (%j expands to jobID)
#SBATCH -n 128              # total number of mpi tasks requested
#SBATCH -N 128             # total number of mpi tasks requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 6:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=username@tacc.utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A TG-ASC130023
##ibrun ls

INPUTDIR=/scratch/02542/gteodoro/images/
#NODES=64
cores=16

export OMP_NUM_THREADS=16

source ~/.bash_profile 
source ~/.profile 

OUTDIR=.

#export | grep LD

#ls -lh /temp/

#ldd /work/02542/gteodoro/dakota/dakota-6.3.0.Linux.x86_64/bin/dakota

#ls -lh /usr/lib64/libblas.so.3

#dakota -i dakota_nscale_vbd.in -w dakota_vbd-1.rst -o dakota_nscale_vbd_100Sampling_55WSI_128nodes.out > dakota_nscale_vbd_100Sampling_55WSI_128nodes.stdout


dakota -i dakota_nscale_vbd.in -r dakota_dace_vbd_random_100.rst -w dakota_dace_vbd_random_100-sencond-try.rst -o dace_vbd_random_100_55WSI_128n.out > dace_vbd_100_55WSI_128n.stdout

#dakota -i dakota_nscale_dace_random_prunned.in -o dakota_nscale_dace_random_600_55WSI_128nodes.out > dakota_nscale_dace_random_600_55WSI_128nodes.stdout

#dakota -i dakota_nscale_ps_moat.in -o dakota_nscale_ps_moat_80_55WSI_128nodes.out > dakota_nscale_ps_moat_80_55WSI_128nodes.stdout
#dakota -i dakota_nscale_ps_moat.in -o dakota_nscale_ps_moat_debug.out > dakota_nscale_ps_moat_debug.stdout

#ibrun tacc_affinity ./PipelineRTFS-NS-Diff-ParStudy -c ${cores} -i ${INPUTDIR} -a parametersFile.in  &> ${OUTDIR}/out.txt


