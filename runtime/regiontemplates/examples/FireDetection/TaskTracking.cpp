#include "TaskTracking.h"

TaskTracking::TaskTracking(int componentId) {
	this->setComponentId(componentId);
}

TaskTracking::~TaskTracking() {
}

bool TaskTracking::run(int procType, int tid) {

	/* Call the MCR and library initialization functions */
	if( !mclInitializeApplication(NULL,0) )
	{
		fprintf(stderr, "Could not initialize the application.\n");
		exit(1);
	}else{
		printf("Init app done\n");
	}
	if (!testVideoInitialize())
	{
		fprintf(stderr,"Could not initialize the library.\n");
		exit(1);
	}else{
		printf("Init lib done\n");
	}

	mxArray *resMain=NULL;
	mxArray *videoPathMex, *compIdMex, *extractVideoMex, *initFrameMex, *endFrameMex;

	// assign input data location
	const char path_s[] = "/home/george/fireDetection_C++/code/det01_015_cif_30_trim.avi";
	videoPathMex = mxCreateString(path_s);

	// Get integer component id and convert it to a string
	stringstream ss;
	ss << this->getComponentId();
	compIdMex = mxCreateString(ss.str().c_str());

	extractVideoMex = mxCreateString("0");
	initFrameMex = mxCreateString("1");
	endFrameMex = mxCreateString("450");


	uint64_t t1 = Util::ClockGetTimeProfile();

	mlfTestVideo(1, &resMain, videoPathMex, compIdMex, extractVideoMex, initFrameMex, endFrameMex);

	uint64_t t2 = Util::ClockGetTimeProfile();

	mxDestroyArray(resMain);
	mxDestroyArray(videoPathMex);
	mxDestroyArray(compIdMex);
	mxDestroyArray(extractVideoMex);
	mxDestroyArray(initFrameMex);
	mxDestroyArray(endFrameMex);

	std::cout << "Executing TaskTracking. id: "<< this->getId()<< " exec time: "<< t2-t1<<std::endl;

	/* Call the library termination function */
	testVideoTerminate();
	mclTerminateApplication();

	return true;
}

int TaskTracking::getComponentId() const {
	return componentId;
}

void TaskTracking::setComponentId(int componentId) {
	this->componentId = componentId;
}
