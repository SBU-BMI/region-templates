#ifndef REG_TILED_RT_COLLECTION_H_
#define REG_TILED_RT_COLLECTION_H_

#include <string>
#include <list>
#include <vector>

#include <opencv/cv.hpp>

#include "openslide.h"

#include "TiledRTCollection.h"

class RegTiledRTCollection : public TiledRTCollection {
private:
    int64_t tw;
    int64_t th;
    int border;

protected:
    // Template method hook for a custom tiling method.
    // Defaults to returning the input images with a single
    //   tile containing the full image.
    void customTiling();

public:
    RegTiledRTCollection(std::string name, std::string refDDRName, 
        std::string tilesPath, int64_t tw, int64_t th, int border);
};

#endif
