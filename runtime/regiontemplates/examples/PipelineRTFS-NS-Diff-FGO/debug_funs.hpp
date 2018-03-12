#ifndef DEBUGF_HPP_
#define DEBUGF_HPP_

void mapprint(map<int, ArgumentBase*> mapp) {
	for (pair<int, ArgumentBase*> p : mapp)
		cout << p.first << ":" << p.second->getName() << endl;
}

void mapprint(map<int, list<ArgumentBase*>> mapp) {
	for (pair<int, list<ArgumentBase*>> p : mapp) {
	// for (map<int, ArgumentBase*>::iterator i=map.begin(); i!=map.end();i++)
		cout << p.first << ":" << endl;
		for (ArgumentBase* a : p.second)
			cout << "\t" << a->getName() << ":" << a->toString() << endl;
	}
}

void mapprint(map<int, PipelineComponentBase*> mapp) {
	for (map<int, PipelineComponentBase*>::iterator i=mapp.begin(); i!=mapp.end();i++)
		cout << i->first << ":" << i->second->getName() << endl;
}

void mapprint(map<int, PipelineComponentBase*> mapp, map<int, ArgumentBase*> args) {
	cout << endl<< "merged: " << endl;
	for (pair<int, PipelineComponentBase*> p : mapp) {
		cout << "stage " << p.second->getId() << ":" << p.second->getName() << endl;
		cout << "\tinputs: " << endl;
		for (int i : p.second->getInputs())
			cout << "\t\t" << i << ":" << args[i]->getName() << " = " 
				<< args[i]->toString() << endl;
		cout << "\toutputs: " << endl;
		for (int i : p.second->getOutputs())
			cout << "\t\t" << i << ":" << args[i]->getName() << endl;
	}
}

void listprint(list<PipelineComponentBase*> mapp) {
	for (list<PipelineComponentBase*>::iterator i=mapp.begin(); i!=mapp.end();i++)
		cout << (*i)->getId() << ":" << (*i)->getName() << endl;
}

void adj_mat_print(mincut::weight_t** adjMat, size_t n) {
	for (int i=0; i<n; i++) {
		for (int j=0; j<n; j++) {
			cout << adjMat[i][j] << "\t";
		}
		cout << endl;
	}
}

void adj_mat_print(mincut::weight_t** adjMat, map<size_t, int> id2task, size_t n) {
	cout << "x\t";
	for (int i=0; i<n; i++)
		cout << id2task[i] << "\t";
	cout << endl;
	for (int i=0; i<n; i++) {
		cout << id2task[i] << "\t";
		for (int j=0; j<n; j++) {
			cout << adjMat[i][j] << "\t";
		}
		cout << endl;
	}
}

#endif