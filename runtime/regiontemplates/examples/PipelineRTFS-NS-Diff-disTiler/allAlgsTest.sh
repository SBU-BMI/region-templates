NP=8

rm ttime.txt
rm ftime.txt
rm dstddev.txt
rm astddev.txt
rm ntiles.txt

for TILING_ALG in 0 1 2 3 4 5 ; do
    for PRETILING_ALG in 0 1; do
        for TILES in 2 4 8 16 32 64 128 ; do
            echo "testing A${TILING_ALG} P${PRETILING_ALG} NT${TILES}"
            rm tmp/*
            mpirun -np $NP ./PipelineRTFS-NS-Diff-disTiler -i tcga7m.svs \
                -m tcga7m.mask.tiff -p /home/willian/Desktop/ -t $TILES \
                -r $PRETILING_ALG -l $TILING_ALG > fulllog.tmp

            cat fulllog.tmp | grep "PROFILING" > results.tmp

            cat results.tmp | grep "TTIME" | tr '\n' ' ' >> ttime.txt
            echo -n ", " >> ttime.txt
            cat results.tmp | grep "FULLTIME" | tr '\n' ' ' >> ftime.txt
            echo -n ", " >> ftime.txt
            cat results.tmp | grep "DENSESTDDEV" | tr '\n' ' ' >> dstddev.txt
            echo -n ", " >> dstddev.txt
            cat results.tmp | grep "ALLSTDDEV" | tr '\n' ' ' >> astddev.txt
            echo -n ", " >> astddev.txt
            cat results.tmp | grep "NTILES" | tr '\n' ' ' >> ntiles.txt
            echo -n ", " >> ntiles.txt
        done
        # new line
        echo "" >> ttime.txt
        echo "" >> ftime.txt
        echo "" >> dstddev.txt
        echo "" >> astddev.txt
        echo "" >> ntiles.txt
    done
done


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
# }