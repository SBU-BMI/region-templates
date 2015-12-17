#!/usr/bin/env bash
# Run this script in the examples folder: /regiontemplates/examples$ bash HPDC2016-testscript.sh /home/taveira/mestrado/subset

inputPath="$1"

numberOfProcs=20

hadoopgisMetric="Dice"
testOnlyGis="false"

outputPath="$2"

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
echo "Starting Tests"

#Copying the cache rtconf.xml file
cp PipelineRTFS-NS-Diff-AH/rtconf.xml .

if [ "$testOnlyGis" == "false" ]; then
    echo "Testing PipelineRTFS-NS-Diff-AH..."
    START_TIME=$SECONDS
    program="PipelineRTFS-NS-Diff-AH"
    programPath="${program}/${program}"
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask1 > ${outputPath}/${program}-subset-mask1.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask2 > ${outputPath}/${program}-subset-mask2.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask3 > ${outputPath}/${program}-subset-mask3.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask4 > ${outputPath}/${program}-subset-mask4.txt
    ELAPSED_TIME=$(($SECONDS - $START_TIME))
    echo "Elapsed Time: "${ELAPSED_TIME}" seconds."

    echo "Testing PipelineRTFS-NS-Diff-AH-PRO..."
    START_TIME=$SECONDS
    program="PipelineRTFS-NS-Diff-AH-PRO"
    programPath="${program}/${program}"
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask1 > ${outputPath}/${program}-subset-mask1.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask2 > ${outputPath}/${program}-subset-mask2.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask3 > ${outputPath}/${program}-subset-mask3.txt
    mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask4 > ${outputPath}/${program}-subset-mask4.txt
    ELAPSED_TIME=$(($SECONDS - $START_TIME))
    echo "Elapsed Time: "${ELAPSED_TIME}" seconds."
fi

echo "Testing PipelineRTFS-NS-Diff-AH-GIS..."
START_TIME=$SECONDS
program="PipelineRTFS-NS-Diff-AH-GIS"
programPath="${program}/${program}"
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask1 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask1.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask2 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask2.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask3 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask3.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask4 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask4.txt
ELAPSED_TIME=$(($SECONDS - $START_TIME))
echo "Elapsed Time: "${ELAPSED_TIME}" seconds."

echo "Testing PipelineRTFS-NS-Diff-AH-PRO-GIS..."
START_TIME=$SECONDS
program="PipelineRTFS-NS-Diff-AH-PRO-GIS"
programPath="${program}/${program}"
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask1 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask1.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask2 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask2.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask3 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask3.txt
mpirun -n ${numberOfProcs} ${programPath} -i ${inputPath}/subset-mask4 > ${outputPath}/${program}-${hadoopgisMetric}-subset-mask4.txt
ELAPSED_TIME=$(($SECONDS - $START_TIME))
echo "Elapsed Time: "${ELAPSED_TIME}" seconds."

echo "Tests Finished!!"