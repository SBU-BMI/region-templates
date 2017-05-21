#!/bin/sh

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
# -uid id of the execution
exec_id=""
# -i folder in which input images are stored
input_image_folder=""
# -f nm, pro, ga
tuning_algorithm="ga"
# -d 0:Meanshift, 1:No-Declumping, 2:Watershed
declumpin_method=2
# -m 
quality_weight=1
# -t 
time_weight=0
# -r maximum number of iterations in the tuning algorithm
max_iterations=100
# -x metric used to compare ground thruth and generated mask:  dice, jaccard or dicenc. dicenc is the default metric. dicenc is that average metric that we discussed = (dice+dice not cool)/2.
comparison_metric="dicenc"
# -a input image extension for the program parser to look for. The name of the images and reference masks must be the same (apart from the extension)
image_extension=".tiff"
# -b reference mask extension. .txt for labeled masks, and .png for binary
mask_extension=".txt"

while getopts "u:i:f:d:m:t:r:x:a:b:" opt; do
    case "$opt" in
    u)  exec_id=$OPTARG
	;;
    i)	input_image_folder=$OPTARG
        ;;
    f)  tuning_algorithm=$OPTARG
        ;;
    d)  declumpin_method=$OPTARG
        ;;
    m)  quality_weight=$OPTARG
        ;;
    t)  time_weight=$OPTARG
        ;;
    r)  max_iterations=$OPTARG
        ;;
    x)  comparison_metric=$OPTARG
        ;;
    a)  image_extension=$OPTARG
        ;;
    b)  mask_extension=$OPTARG
        ;;
    esac
done

command="mpirun --allow-run-as-root -n 2 ./Tuning-Yi -i $input_image_folder -f $tuning_algorithm -d $declumpin_method -m $quality_weight -t $time_weight -r $max_iterations -x $comparison_metric -a $image_extension -b $mask_extension"


start_time=`date`
echo "STARTING execution. Command: $command"

$command > outtuning.txt

end_time=`date`

# genarate JSON file
echo "{" > $exec_id.json
echo "\"executionid\":$exec_id," >> $exec_id.json
echo "\"startdatetime\":$start_time," >> $exec_id.json
echo "\"enddatetime\":$end_time," >> $exec_id.json
grep RESULT outtuning.txt | awk '{print $3":\""$4"\",\n"$5":\""$6"\",\n"$7":\""$8"\",\n"$9":\""$10"\",\n"$11":\""$12"\",\n"$13":\""$14"\",\n"$15":\""$16"\"" }' >> $exec_id.json
echo "}" >> $exec_id.json


# End of file
