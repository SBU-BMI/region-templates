#include "SingleTileRTCollection.h"

void SingleTileRTCollection::customTiling() {
    for (std::string img : this->initialPaths) {
        this->tiles[img].clear();
        this->tiles[img].push_back(
            cv::Rect_<int64_t>(xi, yi, xo - xi, yo - yi));
    }
}

// Performs the tiling using the current algorithm
// Also used for mat tiling, instead of tiling form the svs image
void SingleTileRTCollection::tileMat(cv::Mat                       &mat,
                                     std::list<cv::Rect_<int64_t>> &tiles,
                                     std::list<cv::Rect_<int64_t>> &bgTiles) {}

SingleTileRTCollection::SingleTileRTCollection(std::string name,
                                               std::string refDDRName,
                                               std::string tilesPath, long xi,
                                               long xo, long yi, long yo)
    : xi(xi), xo(xo), yi(yi), yo(yo),
      TiledRTCollection(name, refDDRName, tilesPath, 0, NULL) {
    std::cout << "tiles size: " << this->tiles.size() << "\n";
}
