#include "merging.hpp"

int new_uid() {
	return uid++;
}

ArgumentBase* find_argument(PipelineComponentBase* p, string name, map<int, ArgumentBase*> expanded_args) {
	for(int i : p->getInputs()){
		if (expanded_args[i]->getName().compare(name) == 0) {
			return expanded_args[i];
		}
	}
	for(int i : p->getOutputs()){
		if (expanded_args[i]->getName().compare(name) == 0) {
			return expanded_args[i];
		}
	}

	return NULL;
}

// This function assumes that the merged PCB has at least, but not limited to one ReusableTask on tasks
// of type task_name. Also, to_merge must have exactly one ReusableTask of type task_name. These
// conditions are not checked.
bool exists_reusable_task(const PipelineComponentBase* merged, const PipelineComponentBase* to_merge, string task_name) {
	// get the only task of to_merge that has the type task_name
	ReusableTask* to_merge_task = NULL;
	for (ReusableTask* t : to_merge->tasks)
		if (t->getTaskName().compare(task_name) == 0)
			to_merge_task = t;

	// attempt to find the same task on merged
	for (ReusableTask* t : merged->tasks)
		if (t->getTaskName().compare(task_name) == 0 && 
				to_merge_task->reusable(t))
			return true;

	return false;
}

// for the meantime the merging will happen whenever at least the first task is reusable
bool merging_condition(const PipelineComponentBase* merged, const PipelineComponentBase* to_merge, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {
	
	// compatibility with stages that don't implement task reuse
	if (ref.size() == 0)
		return false;

	// if any of the stages were already reused they can't be merged
	if (to_merge->reused != NULL)
		return false;

	// verify if the first task is reusable
	if (!exists_reusable_task(merged, to_merge, ref.begin()->first))
		return false;

	// verify if the stage dependency is the same
	for (ArgumentBase* a1 : merged->getArguments()) {
		ArgumentBase* arg1 = args.at(a1->getId());
		if (arg1->getParent() != 0) {
			for (ArgumentBase* a2 : to_merge->getArguments()) {
				ArgumentBase* arg2 = args.at(a2->getId());
				if (arg1->getParent() == arg2->getParent()) {
					return true;
				}
			}
		}
	}

	return false;
}

// filters all the stages from an input map by the stage's name
void filter_stages(const map<int, PipelineComponentBase*> &all_stages, 
	string stage_name, list<PipelineComponentBase*> &filtered_stages, bool shuffle) {

	vector<PipelineComponentBase*> temp;

	for (pair<int, PipelineComponentBase*> p : all_stages)
		if (p.second->getName().compare(stage_name) == 0)
			temp.emplace_back(p.second);

	if (shuffle) {
		srand(945);
		while (temp.size() > 1) {
			int r = rand()%(temp.size()-1);
			filtered_stages.emplace_back(temp[r]);
			temp.erase(find(temp.begin(), temp.end(), temp[r]));
		}
		filtered_stages.emplace_back(temp.front());
	} else {
		for (PipelineComponentBase* s : temp)
			filtered_stages.emplace_back(s);
	}
}

list<ReusableTask*> task_generator(map<string, list<ArgumentBase*>> &tasks_desc, PipelineComponentBase* p, 
	map<int, ArgumentBase*> expanded_args) {

	list<ReusableTask*> tasks;
	ReusableTask* prev_task = NULL;

	// traverse the map on reverse order to set dependencies
	for (map<string, list<ArgumentBase*>>::reverse_iterator t=tasks_desc.rbegin(); t!=tasks_desc.rend(); t++) {
		// get task args
		list<ArgumentBase*> args;
		for (ArgumentBase* a : t->second) {
			ArgumentBase* aa = find_argument(p, a->getName(), expanded_args);
			if (aa != NULL)
				args.emplace_back(aa);
		}

		// call constructor
		int uid = new_uid();
		ReusableTask* n_task = ReusableTask::ReusableTaskFactory::getTaskFromName(t->first, args, NULL);
		n_task->setId(uid);
		n_task->setTaskName(t->first);
		// set previous task dependency if this isn't the first task generated
		if (t != tasks_desc.rbegin()) {
			prev_task->parentTask = n_task->getId();
		}
		prev_task = n_task;
		tasks.emplace_back(n_task);
		// cout << "[task_generator] new task " << uid << ":" << t->first << " from stage " << p->getId() << endl;
		// cout << "[task_generator] \targs:" << endl;
		// n_task->print();

	}

	return tasks;
}

ReusableTask* find_task(list<ReusableTask*> l, string name) {
	for (ReusableTask* t : l)
		if (t->getTaskName().compare(name) == 0)
			return t;
	return NULL;
}

list<ReusableTask*> find_tasks(list<ReusableTask*> l, string name) {
	list<ReusableTask*> tasks;
	for (ReusableTask* t : l)
		if (t->getTaskName().compare(name) == 0)
			tasks.emplace_back(t);
	return tasks;
}

void merge_stages(PipelineComponentBase* current, PipelineComponentBase* s, map<string, list<ArgumentBase*>> ref) {

	s->reused = current;

	if (s->tasks.size() != ref.size()) {
		exit(-10);
	}

	ReusableTask* current_frontier_reusable_tasks;
	map<std::string, std::list<ArgumentBase*>>::iterator p=ref.begin();
	ReusableTask* prev_reusable_task = NULL;
	for (; p!=ref.end(); p++) {
		// verify if this is the first reusable task
		ReusableTask* t_s = find_task(s->tasks, p->first);
		list<ReusableTask*> t_cur = find_tasks(current->tasks, p->first);

		// check all of the same tasks of current
		bool reusable = false;
		for (ReusableTask* t : t_cur) {
			// verify if t_s is reusable by checking if it's compatible with a task t and
			//   if the prev_reusable_task is also the predecessor of t.
			if (t->reusable(t_s) && (prev_reusable_task == NULL || 
									prev_reusable_task->getId() == t->parentTask)) {
				reusable = true;
				prev_reusable_task = t;
				current_frontier_reusable_tasks = t;
				break;
			}
		}
		if (!reusable) {
			break;
		}

		// free t_s since it was reused
		delete t_s;
	}

	// updates the first non-reusable task dependency
	ReusableTask* frontier = find_task(s->tasks, p->first);
	frontier->parentTask = current_frontier_reusable_tasks->getId();
	current->tasks.emplace_front(frontier);
	p++;

	// adds the remaining non-reusable tasks to current
	for (; p!=ref.end(); p++) {
		current->tasks.emplace_front(find_task(s->tasks, p->first));
	}

	// remove all tasks references since they were either deleted above or have been sent to current
	s->tasks.clear();
}

// attempt to merge a list of PCB, returning the new list with only the merged PCBs
list<PipelineComponentBase*> merge_stages(list<PipelineComponentBase*> stages, 
	map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	if (stages.size() == 1)
		return stages;

	list<PipelineComponentBase*>::iterator i = stages.begin();
	
	for (; i!=stages.end(); i++) {
		for (list<PipelineComponentBase*>::iterator j = next(i); j!=stages.end();) {
			if (merging_condition(*i, *j, args, ref)) {
				merge_stages(*i, *j, ref);
				j = stages.erase(j);
			} else
				j++;
		}
	}

	return stages;
}

// Attempt to merge a list of PCB, returning a list of PCBs with the same size.
// The new list will have the merged PCBs without any tasks and with the reuse
// attribute set as true.
list<PipelineComponentBase*> merge_stages_full(list<PipelineComponentBase*> stages, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>> ref) {

	if (stages.size() == 1)
		return stages;

	list<PipelineComponentBase*>::iterator i = stages.begin();
	
	for (; i!=stages.end(); i++) {
		for (list<PipelineComponentBase*>::iterator j = next(i); j!=stages.end(); j++) {
			// cout << "i tasks=" << (*i)->tasks.size() << endl;
			// cout << "j tasks=" << (*j)->tasks.size() << endl;
			if (merging_condition(*i, *j, args, ref)) {
				merge_stages(*i, *j, ref);
				(*j)->reused = *i;
			}
		}
	}

	return stages;
}

int get_reuse_factor(PipelineComponentBase* s1, PipelineComponentBase* s2, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	if (!merging_condition(s1, s2, args, ref))
		return 0;

	PipelineComponentBase* s1_clone = s1->clone();
	s1_clone->setLocation(PipelineComponentBase::WORKER_SIDE);
	PipelineComponentBase* s2_clone = s2->clone();
	s2_clone->setLocation(PipelineComponentBase::WORKER_SIDE);

	merge_stages(s1_clone, s2_clone, ref);

	int ret = s1->tasks.size() + s2->tasks.size() - s1_clone->tasks.size();

	// clean memory
	s1_clone->remove_outputs = true;
	delete s1_clone;
	s2_clone->remove_outputs = true;
	delete s2_clone;

	return ret;
}

float calc_stage_proc(list<PipelineComponentBase*> s, const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {
	list<PipelineComponentBase*>::iterator i = s.begin();
	

	for (; i!=s.end(); i++) {
		PipelineComponentBase* current = (*i)->clone();
		current->setLocation(PipelineComponentBase::WORKER_SIDE);
		for (list<PipelineComponentBase*>::iterator j = next(i); j!=s.end();) {
			if (merging_condition(*i, *j, args, ref)) {
				PipelineComponentBase* j_clone = (*j)->clone();
				j_clone->setLocation(PipelineComponentBase::WORKER_SIDE);
				merge_stages(current, j_clone, ref);
				j_clone->remove_outputs = true;
				delete j_clone;
				j = s.erase(j);
			} else
				j++;
		}
		(*i) = current;
	}

	float proc_cost = 0;
	for (PipelineComponentBase* p : s) {
		for (ReusableTask* t : p->tasks)
			// proc_cost += t->getProcCost();
			proc_cost++;
		p->remove_outputs = true;
		delete p;
	}

	return proc_cost;
}

// just add PCB s symbolicaly and calc the cost with stages
float calc_stage_proc(list<PipelineComponentBase*> stages, PipelineComponentBase* s, map<int, ArgumentBase*> &args, 
	map<string, list<ArgumentBase*>> ref) {
	stages.emplace_back(s);
	return calc_stage_proc(stages, args, ref);
}

float calc_stage_mem(list<PipelineComponentBase*> s, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {
	list<PipelineComponentBase*>::iterator i = s.begin();

	for (; i!=s.end(); i++) {
		PipelineComponentBase* current = (*i)->clone();
		current->setLocation(PipelineComponentBase::WORKER_SIDE);
		for (list<PipelineComponentBase*>::iterator j = next(i); j!=s.end();) {
			if (merging_condition(*i, *j, args, ref)) {
				PipelineComponentBase* j_clone = (*j)->clone();
				j_clone->setLocation(PipelineComponentBase::WORKER_SIDE);
				merge_stages(current, j_clone, ref);
				j = s.erase(j);
				j_clone->remove_outputs = true;
				delete j_clone;
			} else
				j++;
		}
	}

	float mem_cost = 0;
	for (PipelineComponentBase* p : s) {
		for (ReusableTask* t : p->tasks)
			// mem_cost += t->getMemCost();
			mem_cost+=0;
		p->remove_outputs = true;
		delete p;
	}

	return mem_cost;
}

list<PipelineComponentBase*> cpy_stage_list(const list<PipelineComponentBase*>& stages) {
	list<PipelineComponentBase*> new_list;
	
	for (PipelineComponentBase* s : stages) {
		new_list.emplace_back(s->clone());
	}

	return new_list;
}