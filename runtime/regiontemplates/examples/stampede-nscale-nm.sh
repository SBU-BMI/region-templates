#!/bin/bash
#----------------------------------------------------
# Example SLURM job script to run applications on
# TACC's Stampede system.
#----------------------------------------------------
#
#SBATCH -J test-nscale-nm          # Job name
#SBATCH -o symmetric_job.o%j # Name of stdout output file(%j expands to jobId)
#SBATCH -e symmetric_job.o%j # Name of stderr output file(%j expands to jobId)
#SBATCH -p normal            # Queue name
#SBATCH -N 1                 # Total number of nodes requested (16 cores/node)
#SBATCH -n 2                 # Tasks requested
#SBATCH --mail-user=luisfrt@gmail.com
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -t 10:00:00          # Run time (hh:mm:ss)

export HARMONY_HOME=/work/02542/gteodoro/luis/region-templates/build-luis/regiontemplates/external-src/activeharmony-4.5/


inputPath="$1"

numberOfProcs=2

numberOfRepeatedTests=10

algorithm="nm"

outputPath="$2"

metricWeight="$3"

timeWeight="$4"

mkdir ${outputPath}/inputImg

imgPath="$outputPath/inputImg"

if [ "$inputPath" == "" ]; then
	echo "You need to specify the masks path (.tiff and text files) as the first argument!"
	exit
fi

if [ "$outputPath" == "" ]; then
	echo "You need to specify the output path as the second argument!"
	exit
fi

if [ "$HARMONY_HOME" == "" ]; then
	echo "You need to export the Active HARMONY_HOME path to your system environment!"
	exit
fi

if [ "$metricWeight" == "" ]; then
	metricWeight=1
	exit
fi

if [ "$timeWeight" == "" ]; then
	timeWeight=1
	exit
fi

echo "Metric Weight: "$metricWeight
echo "Time Weight: "$timeWeight


echo "Starting Tests - 15 imgs"

#Copying the cache rtconf.xml file
cp Tuning-Nscale/rtconf.xml .

    START_TIME=$SECONDS

    echo "Testing Tuning Multiobjective Nscale - NM..."

     IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="Tuning-Nscale"
             programPath="${program}/${program}"
             algorithm="NM"

             cp ${inputPath}/image${IMG_COUNTER}_mask.txt ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             TEST_NUM=0
             while [  $TEST_NUM -lt $numberOfRepeatedTests ]; do
                ibrun ${programPath} -i ${imgPath} -f ${algorithm} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-image${IMG_COUNTER}-test${TEST_NUM}.txt
             let TEST_NUM=TEST_NUM+1
             done
             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

echo "Tests Finished!!"