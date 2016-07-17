#ifndef TASK_DICE_DIFF_POLYLIST_H_
#define TASK_DICE_DIFF_POLYLIST_H_

#include <regiontemplates/DenseDataRegion2D.h>
#include "../../comparativeanalysis/hadoopgis/Hadoopgis.h"

class TaskDiceDiffPolyList : public Hadoopgis {
protected:
    std::string scriptName;

    void callScript(std::string pathToScript, std::string pathToHadoopgisBuild, std::string maskFileName,
                    std::string referenceMaskFileName);

public:

    TaskDiceDiffPolyList(DenseDataRegion2D *tileMask, PolygonListDataRegion *polyListDRFromReferenceMask,
                         float *diffPixels);

    void parseOutput(std::string pathToMaskOutputtedByTheScript, double area1, double area2);

};

#endif /* TASK_DICE_DIFF_POLYLIST_H_ */
