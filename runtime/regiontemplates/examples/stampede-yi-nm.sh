#!/bin/bash
#----------------------------------------------------
# Example SLURM job script to run applications on
# TACC's Stampede system.
#----------------------------------------------------
#
#SBATCH -J test-yi-nm          # Job name
#SBATCH -o symmetric_job.o%j # Name of stdout output file(%j expands to jobId)
#SBATCH -e symmetric_job.o%j # Name of stderr output file(%j expands to jobId)
#SBATCH -p normal            # Queue name
#SBATCH -N 1                 # Total number of nodes requested (16 cores/node)
#SBATCH -n 2                 # Tasks requested
#SBATCH --mail-user=luisfrt@gmail.com
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -t 48:00:00          # Run time (hh:mm:ss)

export HARMONY_HOME=/work/02542/gteodoro/luis/region-templates/build-luis/regiontemplates/external-src/activeharmony-4.5/


inputPath="$1"

numberOfProcs=2


algorithm="nm"

outputPath="$2"

metricWeight="$3"

timeWeight="$4"

TEST_NUM="$5"

numberOfRepeatedTests="$6"

#rm ${outputPath}/*
mkdir ${outputPath}/inputImg
rm -r ${outputPath}/inputImg/*

echo ${outputPath}

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

if [ "$TEST_NUM" == "" ]; then
	TEST_NUM=0
	exit
fi

if [ "$numberOfRepeatedTests" == "" ]; then
	numberOfRepeatedTests=10
	exit
fi

echo "Metric Weight: "$metricWeight
echo "Time Weight: "$timeWeight


echo "Starting Tests - 15 imgs"

#Copying the cache rtconf.xml file
cp Tuning-Yi/rtconf.xml .


    echo "Testing Tuning Multiobjective Yi - NM..."

    IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="Tuning-Yi"
             programPath="${program}/${program}"
             algorithm="NM"

             cp ${inputPath}/image${IMG_COUNTER}_mask.txt ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             TEST_NUM="$5"
             if [ "$TEST_NUM" == "" ]; then
	            TEST_NUM=0
             fi
             while [  $TEST_NUM -lt $numberOfRepeatedTests ]; do
                 declumpingType="2"
                 ibrun ${programPath} -i ${imgPath} -f ${algorithm} -d ${declumpingType} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-declumping_${declumpingType}-image${IMG_COUNTER}-test${TEST_NUM}.txt
                 declumpingType="1"
                 ibrun ${programPath} -i ${imgPath} -f ${algorithm} -d ${declumpingType} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-declumping_${declumpingType}-image${IMG_COUNTER}-test${TEST_NUM}.txt
                 declumpingType="0"
                 ibrun ${programPath} -i ${imgPath} -f ${algorithm} -d ${declumpingType} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-declumping_${declumpingType}-image${IMG_COUNTER}-test${TEST_NUM}.txt
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