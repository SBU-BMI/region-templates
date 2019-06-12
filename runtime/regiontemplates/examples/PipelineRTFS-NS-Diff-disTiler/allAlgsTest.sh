# cat results | xclip -sel clip

if (( $# != 4 )); then
    echo "usage: bash allAlgsTest.sh <NP> <IMG> <MASK> <BASE_PATH>"
    exit 1
fi

NP=$1
IMG=$2
MASK=$3
BASE_PATH=$4

# NP=8
# IMG="tcga7m.svs"
# MASK="tcga60m.mask.tiff"
# BASE_PATH="/home/willian/Desktop/"

# TILING_ALGS=( [8]="CBAL_POINT_QUAD_TREE_ALG" [1]="LIST_ALG_HALF" )
# TILES_S=( 2, 4 )
# PRETILING_ALGS=( [1]="yes" )

TILING_ALGS=( [0]="NO_TILER" [1]="LIST_ALG_HALF" [2]="LIST_ALG_EXPECT" \
    [3]="KD_TREE_ALG_AREA" [4]="KD_TREE_ALG_COST" [5]="FIXED_GRID_TILING" \
    [6]="HBAL_TRIE_QUAD_TREE_ALG" [7]="CBAL_TRIE_QUAD_TREE_ALG" \
    [8]="CBAL_POINT_QUAD_TREE_ALG" )
PRETILING_ALGS=( [0]="no" [1]="yes" )
TILES_S=( 2, 4, 8, 16, 32, 64, 128 )

echo "<?xml version=\"1.0\"?>
<opencv_storage>
    <CacheConfig>
        <_>
            <type>GLOBAL</type>
            <device>HDD</device>
            <capacity>1000000</capacity>
            <path>./tmp/</path>
            <level>1</level>
            <policy>FIFO</policy>
        </_>
    </CacheConfig>
</opencv_storage>" > rtconf.xml

rm ttime.txt
rm ftime.txt
rm dstddev.txt
rm astddev.txt
rm ntiles.txt

echo "alg;Pre-tiling;2;4;8;16;32;64;128" >> ttime.txt
echo "alg;Pre-tiling;2;4;8;16;32;64;128" >> ftime.txt
echo "alg;Pre-tiling;2;4;8;16;32;64;128" >> dstddev.txt
echo "alg;Pre-tiling;2;4;8;16;32;64;128" >> astddev.txt
echo "alg;Pre-tiling;2;4;8;16;32;64;128" >> ntiles.txt

for TILING_ALG in "${!TILING_ALGS[@]}" ; do
    for PRETILING_ALG in 0 1; do
        
        echo -n "${TILING_ALGS[$TILING_ALG]};" >> ttime.txt
        echo -n "${PRETILING_ALGS[$PRETILING_ALG]};" >> ttime.txt
        echo -n "${TILING_ALGS[$TILING_ALG]};" >> ftime.txt
        echo -n "${PRETILING_ALGS[$PRETILING_ALG]};" >> ftime.txt
        echo -n "${TILING_ALGS[$TILING_ALG]};" >> dstddev.txt
        echo -n "${PRETILING_ALGS[$PRETILING_ALG]};" >> dstddev.txt
        echo -n "${TILING_ALGS[$TILING_ALG]};" >> astddev.txt
        echo -n "${PRETILING_ALGS[$PRETILING_ALG]};" >> astddev.txt
        echo -n "${TILING_ALGS[$TILING_ALG]};" >> ntiles.txt
        echo -n "${PRETILING_ALGS[$PRETILING_ALG]};" >> ntiles.txt

        for TILES in "${TILES_S[@]}" ; do
            echo "testing A${TILING_ALG} P${PRETILING_ALG} NT${TILES}"
            rm tmp/*
            echo "mpirun -np $NP ./PipelineRTFS-NS-Diff-disTiler -i $IMG -m $MASK -p $BASE_PATH -t $TILES -r $PRETILING_ALG -l $TILING_ALG > fulllog.tmp"
            mpirun -np $NP ./PipelineRTFS-NS-Diff-disTiler -i $IMG \
                -m $MASK -p $BASE_PATH -t $TILES \
                -r $PRETILING_ALG -l $TILING_ALG > fulllog.tmp

            cat fulllog.tmp | grep "PROFILING" > results.tmp

            cat results.tmp | grep "TTIME" | tr '\n' ' ' >> ttime.txt
            echo -n "; " >> ttime.txt
            cat results.tmp | grep "FULLTIME" | tr '\n' ' ' >> ftime.txt
            echo -n "; " >> ftime.txt
            cat results.tmp | grep "DENSESTDDEV" | tr '\n' ' ' >> dstddev.txt
            echo -n "; " >> dstddev.txt
            cat results.tmp | grep "ALLSTDDEV" | tr '\n' ' ' >> astddev.txt
            echo -n "; " >> astddev.txt
            cat results.tmp | grep "NTILES" | tr '\n' ' ' >> ntiles.txt
            echo -n "; " >> ntiles.txt
        done
        # new line
        echo "" >> ttime.txt
        echo "" >> ftime.txt
        echo "" >> dstddev.txt
        echo "" >> astddev.txt
        echo "" >> ntiles.txt
    done
done

echo "full time" > results_np${NP}${IMG}.txt
cat ftime.txt >> results_np${NP}${IMG}.txt
echo "" >> results_np${NP}${IMG}.txt
echo "tiling alg time" >> results_np${NP}${IMG}.txt
cat ttime.txt >> results_np${NP}${IMG}.txt
echo "" >> results_np${NP}${IMG}.txt
echo "dense std dev" >> results_np${NP}${IMG}.txt
cat dstddev.txt >> results_np${NP}${IMG}.txt
echo "" >> results_np${NP}${IMG}.txt
echo "all tiles std dev" >> results_np${NP}${IMG}.txt
cat astddev.txt >> results_np${NP}${IMG}.txt
echo "" >> results_np${NP}${IMG}.txt
echo "actual number of tiles" >> results_np${NP}${IMG}.txt
cat ntiles.txt >> results_np${NP}${IMG}.txt

sed -i 's#\[PROFILING\]\[TTIME\]##g' results_np${NP}${IMG}.txt
sed -i 's#\[PROFILING\]\[FULLTIME\]##g' results_np${NP}${IMG}.txt
sed -i 's#\[PROFILING\]\[DENSESTDDEV\]##g' results_np${NP}${IMG}.txt
sed -i 's#\[PROFILING\]\[ALLSTDDEV\]##g' results_np${NP}${IMG}.txt
sed -i 's#\[PROFILING\]\[NTILES\]##g' results_np${NP}${IMG}.txt
sed -i 's#\.#,#g' results_np${NP}${IMG}.txt # calc uses . for text and , for num

rm ttime.txt
rm ftime.txt
rm dstddev.txt
rm astddev.txt
rm ntiles.txt
rm results.tmp
rm fulllog.tmp

# [PROFILING][TTIME]
# [PROFILING][FULLTIME]
# [PROFILING][DENSESTDDEV]
# [PROFILING][ALLSTDDEV]
# [PROFILING][NTILES]


# enum PreTilerOptions_t {
#     NO_PRE_TILER,
#     DENSE_BG_SEPARATOR,
# }

# enum TilerOptions_t {
#     0 NO_TILER,
#     1 LIST_ALG_HALF,
#     2 LIST_ALG_EXPECT,
#     3 KD_TREE_ALG_AREA,
#     4 KD_TREE_ALG_COST,
#     5 FIXED_GRID_TILING,
#     6 HBAL_TRIE_QUAD_TREE_ALG,
#     7 CBAL_TRIE_QUAD_TREE_ALG,
#     8 CBAL_POINT_QUAD_TREE_ALG,
# }