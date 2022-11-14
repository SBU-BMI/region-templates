#include <string>

#include <LightGBM/boosting.h>
#include <LightGBM/prediction_early_stop.h>

int main(int argc, char const *argv[]) {

    std::string filename = "./model.txt";
    auto booster = LightGBM::Boosting::CreateBoosting("gbdt", filename.c_str());

    double features[] = {0.99448228725896,        0.4987962037019174,
                         -0.5465914044851826,     1437.0710676908493,
                         0.000006084898975177666, 352.75,
                         1435.6301460266113,      129508.0};
    double output[1];

    auto early_stop = CreatePredictionEarlyStopInstance(
        "none", LightGBM::PredictionEarlyStopConfig());
    booster->InitPredict(0, booster->NumberOfTotalModel(), false);
    booster->PredictRaw(features, output, &early_stop);
    std::cout << output[0] << "\n";

    return 0;
}

// solidity rel-centr-x rel-centr-y perimeter circularity
// euler hull-perimeter hul-area
