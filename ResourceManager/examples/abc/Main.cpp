

#include <stdio.h>
#include <sys/time.h>
#include "TaskSum.h"
#include "ExecutionEngine.h"
#include "CallBack.h"


// creates a dataflow as the following: A  -> B ->|-> D
//					     				   -> C ->|

int main(int argc, char **argv){
	
	ExecutionEngine *execEngine = new ExecutionEngine(2, 1, ExecEngineConstants::PRIORITY_QUEUE);

	// Computing threads startup consuming tasks
	execEngine->startupExecution();

	CallBack *taskCallBack = new CallBack();

	int a=0;
	int b=5;
	int c=2;

	int k1=10;
	int k2=5;

	sleep(2);

	execEngine->startTransaction(taskCallBack);

	TaskSum* tA = new TaskSum(&a, &b, &c, "A");

	int bOut;
	TaskSum* tB = new TaskSum(&bOut, &a, &k1, "B");
	tB->addDependency(tA->getId());

	int cOut;
	TaskSum* tC = new TaskSum(&cOut, &a, &k2, "C");
	tC->addDependency(tA->getId());

	int dOut;
	TaskSum* tD = new TaskSum(&dOut, &bOut, &cOut, "D");
	tD->addDependency(tB->getId());
	tD->addDependency(tC->getId());

	// Insert tasks in inverted order of dependency to force the "use" of the tasks dependency mechanism
	execEngine->insertTask(tA);
	execEngine->insertTask(tB);
	execEngine->endTransaction();
	execEngine->insertTask(tC);

	execEngine->insertTask(tD);





	// No more task will be assigned for execution. Waits
	// until all currently assigned have finished.
	execEngine->endExecution();
	
	delete execEngine;

	std::cout << "dOut="<< dOut <<std::endl;
	return 0;
}


