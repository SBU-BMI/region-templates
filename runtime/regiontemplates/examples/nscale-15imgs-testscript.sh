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
cp PipelineRTFS-NS-Diff-AH/rtconf.xml .



if [ "$testOnlyGis" == "false" ]; then
    echo "Testing PipelineRTFS-NS-Diff-AH..."

    IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH"
             programPath="${program}/${program}"

             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             #mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} > ${outputPath}/${program}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

    START_TIME=$SECONDS

    echo "Testing PipelineRTFS-NS-Diff-AH-PRO..."

     IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH-PRO"
             programPath="${program}/${program}"

             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} > ${outputPath}/${program}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done
fi

echo "Testing PipelineRTFS-NS-Diff-AH-GIS-${hadoopgisMetric}..."

IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH-GIS"
             programPath="${program}/${program}"

             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             #mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} > ${outputPath}/${program}-${hadoopgisMetric}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

echo "Testing PipelineRTFS-NS-Diff-AH-PRO-GIS-${hadoopgisMetric}..."

IMG_COUNTER=1
         while [  $IMG_COUNTER -lt 16 ]; do
             echo "Testing image${IMG_COUNTER}"
             START_TIME=$SECONDS
             program="PipelineRTFS-NS-Diff-AH-PRO-GIS"
             programPath="${program}/${program}"

             cp ${inputPath}/image${IMG_COUNTER}.mask.png ${imgPath}
             cp ${inputPath}/image${IMG_COUNTER}.tiff ${imgPath}

             mpirun -n ${numberOfProcs} ${programPath} -i ${imgPath} > ${outputPath}/${program}-${hadoopgisMetric}-image${IMG_COUNTER}.txt

             rm ${imgPath}/*
             rm /tmp/BGR-*
             rm /tmp/MASK-*

             ELAPSED_TIME=$(($SECONDS - $START_TIME))
             echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
             let IMG_COUNTER=IMG_COUNTER+1
         done

echo "Tests Finished!!"