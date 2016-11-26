#!/usr/bin/env bash

inputPath="$1"

numberOfProcs=8

algorithm="ga"

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


    echo "Testing Tuning Multiobjective Nscale - GA..."

    IMG_COUNTER=14
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="Tuning-Nscale"
             programPath="${program}/${program}"
             algorithm="GA"

             cp ${inputPath}/image${IMG_COUNTER}_mask.txt ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f ${algorithm} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

    START_TIME=$SECONDS

    echo "Testing Tuning Multiobjective Nscale - NM..."

     IMG_COUNTER=14
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="Tuning-Nscale"
             programPath="${program}/${program}"
             algorithm="NM"

             cp ${inputPath}/image${IMG_COUNTER}_mask.txt ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f ${algorithm} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

echo "Testing Tuning Multiobjective Nscale - PRO..."

IMG_COUNTER=14
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="Tuning-Nscale"
             programPath="${program}/${program}"
             algorithm="PRO"

             cp ${inputPath}/image${IMG_COUNTER}_mask.txt ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f ${algorithm} -m ${metricWeight} -t ${timeWeight} > ${outputPath}/${program}-${algorithm}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

echo "Tests Finished!!"