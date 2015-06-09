#!/bin/sh
#PBS -N NucleiPipelineBundle
#PBS -j oe
#PBS -A UT-NTNL0111

### Unused PBS options ###
## If left commented, must be specified when the job is submitted:
## 'qsub -l walltime=hh:mm:ss,nodes=12:ppn=4'
##
#PBS -l walltime=00:05:00
#PBS -l nodes=1:ppn=1:gpus=3

### End of PBS options ###

date
hostname
cd $PBS_O_WORKDIR

IMAGE_ROOT=/lustre/medusa/gteodor/images/TCGA-02/
for rep in 1;do
#	for gpus in 3; do
#		LOG_FOLDER=results/1-node/image3/gpu/rep-${rep}/${gpus}-placement
#		mkdir -p ${LOG_FOLDER}
#
#		mpirun  NucleiPipelinePerf ${IMAGE_ROOT}/${IMAGE_3} . -g ${gpus} -c 0 &> ${LOG_FOLDER}/res.txt
#	done
#done
##	for cpus in 1 12; do
##		LOG_FOLDER=results/1-node/image3/cpu/rep-${rep}/${cpus}
##		mkdir -p ${LOG_FOLDER}
##
##		mpirun  NucleiPipelinePerf ${IMAGE_ROOT}/${IMAGE_3} . -g 0 -c $cpus &> ${LOG_FOLDER}/res.txt
##	done
#done
# CPU-GPU
#

#	LOG_FOLDER=results/m2090/1-node/image3/cpu-gpu/bundle/fcfs/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_3} . -g 3 -c 9 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image3/cpu-gpu/bundle/priority/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_3} . -g 3 -c 9 -s priority &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image3/cpu/bundle/1-core/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_2} . -g 0 -c 1 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image2/cpu/bundle/12-core/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_2} . -g 0 -c 12 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image1/gpu/bundle/1-gpu/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_1} . -g 1 -c 0 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image1/gpu/bundle/2-gpus/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_1} . -g 2 -c 0 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/m2090/1-node/image1/gpu/bundle/3-gpus/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#
#	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_1} . -g 3 -c 0 -s fcfs &> ${LOG_FOLDER}/res.txt
#
#	mpirun /nics/e/sw/local/keeneland/opt/ddt/3.1-22442/bin/ddt-client --ddtsessionfile /nics/c/home/gteodor/.ddt/session/kidlogin2.nics.utk.edu-1 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_1} . -g 3 -c 9 -s fcfs &> ${LOG_FOLDER}/res.txt

	LOG_FOLDER=results/testshared/1-node/image2/cpu-gpu/bundle/fcfs/rep-${rep}/
	mkdir -p ${LOG_FOLDER}
	mpirun -np 2 NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_2} . -g 3 -c 9 -p fcfs &> ${LOG_FOLDER}/res.txt
#
#	LOG_FOLDER=results/32-node/image3/cpu-gpu/bundle/fcfs/rep-${rep}/
#	mkdir -p ${LOG_FOLDER}
#	mpirun  NucleiPipelineBundle ${IMAGE_ROOT}/${IMAGE_3} . -g 3 -c 9 -p fcfs &> ${LOG_FOLDER}/res.txt



done
echo "done"
date
# run the program


# eof
