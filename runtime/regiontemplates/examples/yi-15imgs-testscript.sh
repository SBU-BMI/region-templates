#!/usr/bin/env bash

inputPath="$1"

numberOfProcs=8

hadoopgisMetric="Dice"
testOnlyGis="false"

outputPath="$2"

mkdir ${outputPath}/inputImg

imgPath="$outputPath/inputImg"

if [ "$inputPath" == "" ]; then
	echo "You need to specify the masks path as the first argument!"
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
echo "Starting Tests - 15 imgs"

#Copying the cache rtconf.xml file
cp PipelineRTFS-NS-Diff-AH-PRO-Yi/rtconf.xml .


if [ "$testOnlyGis" == "false" ]; then
    echo "Testing PipelineRTFS-NS-Diff-AH-PRO-Yi..."

    IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH-PRO-Yi"
             programPath="${program}/${program}"


             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}
             echo "Using NM"
             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f nm.so > ${outputPath}/${program}-NM-image${IMG_COUNTER}.txt
             echo "Using PRO"
             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f pro.so > ${outputPath}/${program}-PRO-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

    START_TIME=$SECONDS
fi

echo "Testing PipelineRTFS-NS-Diff-AH-PRO-Yi-GIS-${hadoopgisMetric}..."

IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH-PRO-Yi-GIS"
             programPath="${program}/${program}"

             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             echo "Using NM"
             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f nm.so > ${outputPath}/${program}-${hadoopgisMetric}-NM-image${IMG_COUNTER}.txt
             echo "Using PRO"
             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} -f pro.so > ${outputPath}/${program}-${hadoopgisMetric}-PRO-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done


echo "Tests Finished!!"