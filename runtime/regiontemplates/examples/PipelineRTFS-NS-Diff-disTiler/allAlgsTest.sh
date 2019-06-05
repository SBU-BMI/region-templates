NP=8

for TILING_ALG in 0 1 2 3 4 5 ; do
    for PRETILING_ALG in 0 1; do
        for TILES in 2 4 8 16 32 64 ; do
            rm tmp/*
            mpirun -np $NP ./PipelineRTFS-NS-Diff-disTiler -i tcga7m.svs \
                -m tcga70m.mask.tiff -p /home/willian/Desktop/ -t $TILES \
                -r $PRETILING_ALG -l $TILING_ALG > fulllog.tmp

            cat fulllog.tmp | grep "PROFILING" > results.tmp

            cat results.tmp | grep "TTIME" >> ttime.txt
            "," >> ttime.txt
            cat results.tmp | grep "FULLTIME" >> ftime.txt
            "," >> ftime.txt
            cat results.tmp | grep "DENSESTDDEV" >> dstddev.txt
            "," >> dstddev.txt
            cat results.tmp | grep "ALLSTDDEV" >> astddev.txt
            "," >> astddev.txt
            cat results.tmp | grep "NTILES" >> ntiles.txt
            "," >> ntiles.txt
        done
        "\n" >> ttime.txt
        "\n" >> ftime.txt
        "\n" >> dstddev.txt
        "\n" >> astddev.txt
        "\n" >> ntiles.txt
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
#     NO_TILER,
#     LIST_ALG_HALF,
#     LIST_ALG_EXPECT,
#     KD_TREE_ALG_AREA,
#     KD_TREE_ALG_COST,
#     FIXED_GRID_TILING,
# }