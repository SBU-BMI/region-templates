#! /bin/bash
scriptName="$(basename "$(test -L "$0" && readlink "$0" || echo "$0")")"
#receives 4 params: 1 - path to hadoopgis, 2 - path to tempfolder(write-permission) 3 - mask polygons and 4 - referenceMask polygons
hadoopgisBuildPath="$1"
tempPath="$2"
maskFileName="$3"
referenceMaskFileName="$4"
knumberofelements="$5"

mask="$tempPath$maskFileName"
referenceMask="$tempPath$referenceMaskFileName"

#echo "The script received the following params:"
#echo "$hadoopgisBuildPath"
#echo "$tempPath"
#echo "$maskFileName"
#echo "$referenceMaskFileName"
#echo "$mask"
#echo "$referenceMask"


# get include path
#hgcfg="hadoopgis.cfg"
#source "$hadoopgisPath$hgcfg"
#HADOOPGIS_INC_PATH=/usr/local/include
#HADOOPGIS_LIB_PATH=/usr/local/lib

#echo "$hadoopgisPath$hgcfg"

#echo -e "Extracting MBBs"

# Change to predicate of your choice
PARAMOPTS="-i 2 -j 2 -p st_nearest2 -a $mask -b $referenceMask -k $knumberofelements -r true"
#PARAMOPTS=" --shpidx1=2 --shpidx2=2 --predicate=st_intersects --prefix1=${maskFileName} --prefix2=${referenceMaskFileName}"
# other example
# knn for k=3 example
#spjoinparam=" --shpidx1=2 --shpidx2=2 -p st_nearest2 -k 3"

PARAMOPTS2="-o 0 "${PARAMOPTS}" -x"
#echo "Params for parameter extraction: ${PARAMOPTS2}"

#echo "Extracting mbb from set 1"
export mapreduce_map_input_file="$mask"
#echo ${PARAMOPTS2}
${hadoopgisBuildPath}bin/map_obj_to_tile ${PARAMOPTS2} < ${mask} > ${mask}.mbb
#echo "Done extracting mbb from set 1\n"

#echo "Extracting mbb from set 2"
export mapreduce_map_input_file="$referenceMask"
${hadoopgisBuildPath}bin/map_obj_to_tile ${PARAMOPTS2} < ${referenceMask} > ${referenceMask}.mbb
#echo "Done extracting mbb from set 2\n"

#echo "Getting space dimension"
# In MapReduce this needs to call "get_space_dimension 2" in Mapper phase and "get_space_dimension 1" in Reducer phase
cat ${mask}.mbb ${referenceMask}.mbb | ${hadoopgisBuildPath}bin/get_space_dimension 2 > ${mask}spaceDims.cfg

#echo "Done getting space dimension\n"

# Reading space dimension from output file
read min_x min_y max_x max_y object_count <<< `cat ${mask}spaceDims.cfg`
read total_size <<< `du -cs ${mask} ${referenceMask} | tail -n 1 | cut -f1`
#echo "min_x=${min_x}"
#echo "min_y:${min_y}"
#echo "max_x:${max_x}"
#echo "max_y:${max_y}"
#echo "object_count:${object_count}"
#echo "total_size:${total_size}"

#echo "Computing partition size for block 4MB (data per tile)"
${hadoopgisBuildPath}bin/compute_partition_size 400 1.0 ${total_size} ${object_count} > ${mask}partition_size.cfg
#cat ${mask}partition_size.cfg
source ${mask}partition_size.cfg

#echo "Normalizing MBBs"
PARTITIONPARAM=" -a ${min_x} -b ${min_y} -c ${max_x} -d ${max_y} -f 1 -n"
cat ${mask}.mbb ${referenceMask}.mbb | ${hadoopgisBuildPath}bin/mbb_normalizer ${PARTITIONPARAM} > ${mask}normalizedMbbs

#echo "Partitioning"
cat ${mask}normalizedMbbs | ${hadoopgisBuildPath}bin/bsp -b ${partitionSize} > ${mask}partition.idx
## Different partition methods available
#cat ${mask}normalizedMbbs | ${hadoopgisBuildPath}bin/hc -b ${partitionSize} > ${mask}partition.idx
#cat ${mask}normalizedMbbs | ${hadoopgisBuildPath}bin/fg -b ${partitionSize} > ${mask}partition.idx
#cat ${mask}normalizedMbbs | ${hadoopgisBuildPath}bin/bos -b ${partitionSize} > ${mask}partition.idx

#echo -e "Done partitioning\n"

#echo "Denormalizing"
DENORMPARAM=" -a ${min_x} -b ${min_y} -c ${max_x} -d ${max_y} -f 1 -o"
cat ${mask}partition.idx | ${hadoopgisBuildPath}bin/mbb_normalizer ${DENORMPARAM} > ${mask}denormpartition.idx
#echo "Done denormalizing\n"

#echo "Map object to tile: "
PARAMOPTS3=${PARAMOPTS}" -f 1:1,1:2,2:1,2:2,mindist -c ${mask}denormpartition.idx"

#offset is 1 for mapper
PARAMOPTS4="-o 0 "${PARAMOPTS3}
#echo "Mapping object from data set 1:"
export mapreduce_map_input_file="$mask"
#echo ${PARAMOPTS3}
${hadoopgisBuildPath}bin/map_obj_to_tile ${PARAMOPTS4} < ${mask} > ${mask}outputmapper1
#echo "Done mapping data set 1\n"


#echo "Mapping object from data set 2:"
export mapreduce_map_input_file="$referenceMask"
#echo ${PARAMOPTS3}
${hadoopgisBuildPath}bin/map_obj_to_tile ${PARAMOPTS4} < ${referenceMask} > ${mask}outputmapper2
#echo -e "Done mapping data set 1\n"

# Simulate MapReduce sorting
#echo "Shuffling/sorting"
sort ${mask}outputmapper1 ${mask}outputmapper2 > ${mask}inputreducer
#echo "Done with shuffling/sorting"

#echo "Performing spatial processing"
# Perform reducer/resque processing
# Offset is 3 for reducer (join index and legacy field)
PARAMOPTS5="-o 3 "${PARAMOPTS}" -f 1:1,2:1,mindist,1:2,2:2 -c ${mask}denormpartition.idx"
#PARAMOPTS5="-o 3 "${PARAMOPTS}" -f 1:1,1:2,2:1,2:2,mindist -c ${mask}denormpartition.idx"
#echo ${PARAMOPTS5}
${hadoopgisBuildPath}bin/resque ${PARAMOPTS5} < ${mask}inputreducer > ${mask}.output
#echo "Done with spatial processing"

# Simulate MapReduce sorting
#echo "Performing duplicate removal"
#sort ${mask}outputreducer > ${mask}.output

#${hadoopgisBuildPath}bin/duplicate_remover uniq < ${mask}inputmapperboundary > ${mask}.output

#echo "Removing temp files"
rm ${mask}
rm ${referenceMask}
rm ${mask}.mbb
rm ${referenceMask}.mbb
rm ${mask}spaceDims.cfg
rm ${mask}partition_size.cfg
rm ${mask}normalizedMbbs
rm ${mask}partition.idx
rm ${mask}denormpartition.idx
rm ${mask}outputmapper1
rm ${mask}outputmapper2
rm ${mask}inputreducer
#rm ${mask}outputreducer
#rm ${mask}inputmapperboundary
#REMOVE THE CREATED FILES!!!