#include "RegularTilingStage.h"

RegularTilingStage::RegularTilingStage() {

}

std::string RegularTilingStage::getName() {
    return "RegularTilingStage";
}

void RegularTilingStage::setTileSize(std::string arg) {
    this->tileSize = atoi(arg);
}

// Tiling of unfinishedTiles which updates it and adds tiles to finishedTiles
void RegularTilingStage::tile(const cv::Mat& refImg, 
    std::vector<cv::Rect_<int64_t>>& unfinishedTiles,
    std::vector<cv::Rect_<int64_t>>& finishedTiles) {


    
}
