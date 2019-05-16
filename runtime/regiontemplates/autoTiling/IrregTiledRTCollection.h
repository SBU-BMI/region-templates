#ifndef IRREG_TILED_RT_COLLECTION_H_
#define IRREG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "SvsDataRegion.h"
#include "TiledRTCollection.h"
#include "costFuncs/BGMasker.h"

class IrregTiledRTCollection : public TiledRTCollection {
private:
    int border;
    BGMasker* bgm;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    IrregTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int border, BGMasker* bgm);
};

#endif
