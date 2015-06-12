

#include <stdio.h>
#include <sys/time.h>
#include "TaskSum.h"
#include "TaskMultiply.h"
#include "ExecutionEngine.h"


// creates a dataflow as the following: A  -> B ->|-> D -> E ->|-> G
//										   -> C ->|     -> F ->|

int main(int argc, char **argv){
	
	ExecutionEngine *execEngine = new ExecutionEngine(2, 1, ExecEngineConstants::PRIORITY_QUEUE);


	int a=0;
	int b=5;
	int c=2;

	int k1=10;
	int k2=5;

	TaskSum* tA = new TaskSum(&a, &b, &c);

	int bOut;
	TaskSum* tB = new TaskSum(&bOut, &a, &k1);
	tB->addDependency(tA->getId());

	int cOut;
	TaskSum* tC = new TaskSum(&cOut, &a, &k2);
	tC->addDependency(tA->getId());

	int dOut;
	TaskSum* tD = new TaskSum(&dOut, &bOut, &cOut);
	tD->addDependency(tB->getId());
	tD->addDependency(tC->getId());

	int k3=2;
	int eOut, fOut, gOut;

	TaskMultiply *tE = new TaskMultiply(&eOut, &dOut, &k3);
	TaskMultiply *tF = new TaskMultiply(&fOut, &dOut, &k3);
	tE->addDependency(tD->getId());
	tF->addDependency(tD->getId());


	TaskSum* tG = new TaskSum(&gOut, &eOut, &fOut);
	tG->addDependency(tE->getId());
	tG->addDependency(tF->getId());

	// Insert tasks in inverted order of dependency to force the "use" of the tasks dependency mechanism
	execEngine->insertTask(tA);
	execEngine->insertTask(tB);
	execEngine->insertTask(tC);
	execEngine->insertTask(tD);
	execEngine->insertTask(tE);
	execEngine->insertTask(tF);
	execEngine->insertTask(tG);

	// Computing threads startup consuming tasks
	execEngine->startupExecution();

	// No more task will be assigned for execution. Waits
	// until all currently assigned have finished.
	execEngine->endExecution();
	
	delete execEngine;

	std::cout << "dOut="<< dOut <<std::endl;
	return 0;
}


