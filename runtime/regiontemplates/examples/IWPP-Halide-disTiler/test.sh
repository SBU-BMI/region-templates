tilingAlgs=( 1 2 3 4 )
borders=( 0 10 20 )
nTiless=( 2 4 8 )

pretilingAlgs=0 # no pre-tiling
# pretilingAlgs = { 0 } # no pre-tiling
cpuThreads=1
# cpuThreads = { 1 }
# gpuThreads = { 0 }

for tilingAlg in ${tilingAlgs[@]}; do
    for border in ${borders[@]}; do
        for nTiles in ${nTiless[@]}; do
            echo "Running Alg: $tilingAlg, Border: ${border}, nTiles: ${nTiles}"
            echo "Alg: ${tilingAlg}, Border: ${border}, nTiles: ${nTiles}" >> time.log
            { time mpirun --tag-output -np 2 ./iwpp tcga7m.svs \
                tcga7m.mask.tiff -c $cpuThreads -t $nTiles -l $tilingAlg \
                -b $border -r $pretilingAlgs >> outputs.log ; \
            } 2>> time.log
            echo "" >> time.log
        done
    done
done