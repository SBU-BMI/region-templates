#ifndef IRREG_TILED_RT_COLLECTION_H_
#define IRREG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"

class IrregTiledRTCollection : public TiledRTCollection {
private:
    int border;
    int nTiles; // Expected number of dense tiles by the end of te tiling
    BGMasker* bgm;
    bool hOnlyDenseSweep;
    bool vOnlyDenseSweep;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    IrregTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int border, BGMasker* bgm, int nTiles=0);

    // Setters for vertical or horizontal only sweeping on dense tiling
    // At most one of them can be set
    void setHOnlySweep();
    void setVOnlySweep();

};

#endif
