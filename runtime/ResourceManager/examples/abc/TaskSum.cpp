#include "TaskSum.h"

TaskSum::TaskSum(int* a, int* b, int* c, std::string name) {
	this->a = a;
	this->b = b;
	this->c = c;
	this->name = name;
}

TaskSum::~TaskSum() {

}

bool TaskSum::run(int procType, int tid)
{
	a[0] = b[0] + c[0];


	// Sleep to make sure that the dependencies are being respected.
	sleep(3);

	// if C, sleep another second
	if(!this->name.compare("C")){
		sleep(1);
	}

	std::cout << this->name <<" a[0]="<< a[0] << " b[0]=" << b[0] << " c[0]="<<c[0]<<std::endl;

	return true;
}



