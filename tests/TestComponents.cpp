/*
 * TestArguments.cpp
 *
 *  Created on: Feb 20, 2012
 *      Author: george
 */
#include <assert.h>

#include "PipelineComponentBase.h"


int main(int argc, char **argv){

	PipelineComponentBase *pc = new PipelineComponentBase();
	pc->setComponentName("ComponentAA+");
	ArgumentString *as1 = new ArgumentString("file1");
	ArgumentString *as2 = new ArgumentString("file2");
	pc->addArgument(as1);
	pc->addArgument(as2);

	int size = pc->size();

	char *buff = new char[size];
	int serialized_bytes = pc->serialize(buff);


	PipelineComponentBase *pc_deserialized = new PipelineComponentBase();
	int deserialized_bytes = pc_deserialized->deserialize(buff);

	std::cout << "deserialized_bytes="<< deserialized_bytes <<" serialized_bytes="<<serialized_bytes<<std::endl;
	std::cout << "pc_deserialized.name="<<pc->getComponentName() <<std::endl;

	for(int i = 0; i < pc_deserialized->getArgumentsSize(); i++){
		ArgumentBase *as = pc->getArgument(i);
		std::string *str_ptr = (std::string *)as->getArgPtr();
		std::cout << "pc_deserialized.arg[0].value="<< *str_ptr <<std::endl;
	}

	delete[] buff;
	delete pc;
	delete pc_deserialized;

	return 0;
}


