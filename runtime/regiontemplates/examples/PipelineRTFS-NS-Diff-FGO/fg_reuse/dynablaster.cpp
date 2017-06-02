#include "dynablaster.hpp"

#include <iostream>
#include <string>

list<list<PipelineComponentBase*>> db_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_bucket_size, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	// generate cost matrix
	// R2dVL<int> cost_m(stages_to_merge.size());
	R2dVL cost_m(stages_to_merge.size());

	// convert the list to a vector for ease of access
	vector<PipelineComponentBase*> stages{begin(stages_to_merge), end(stages_to_merge)};
	generate_cost_m(cost_m, stages, args, ref);
	cost_m.print_matrix();


	// testing functions
	// cost_m.print_list();
	// cout << endl;
	// cost_m.print_matrix();

	// cout << "get (3,5): " << cost_m.get(3,5) << endl;
	// cout << endl;

	// cost_m.put(3,5,10);
	// cout << "get (3,5)=10: " << cost_m.get(3,5) << endl;
	// cost_m.print_matrix();
	// cout << endl;

	// cost_m.remove_line(2);
	// cout << "after removal of line 2: " << endl;
	// cost_m.print_list();
	// cout << endl;

	// cost_m.remove_column(3);
	// cout << "after removal of column 3: " << endl;
	// cost_m.print_list();
	// cout << endl;

	// cost_m.remove_line(0);
	// cout << "after removal of line 0: " << endl;
	// cost_m.print_list();
	// cout << endl;

	// initialize solution map, containing a list of stages to be merged
	// for each stage (containing itself)
	map<int, pair<list<int>, PipelineComponentBase*>> merging_solution;
	for (int i=0; i<stages_to_merge.size(); i++) {
		pair<list<int>, PipelineComponentBase*> s;
		s.first.emplace_back(i);
		s.second = stages[i]->clone();
		merging_solution[i] = s;
	}
	
	// keep attempting to merge until merging sugestion is bad
	while (true) {
		// find best merging sugestion on cost matrix
		list<pair<int, int>> sugestions = get_best_merging_sug(cost_m);

		cout << "Merging sugestions:" << endl;
		for (pair<int, int> p : sugestions) {
			cout << p.first << ":" << p.second << endl;
		}

		// break the ties (if any)
		pair<int, int> best_sug = break_ties(sugestions);
		cout << "Best sugestion: " << best_sug.first 
			<< ":" << best_sug.second << endl;

		// verify if merging sugestion achieves stoping condition
		// if not, perform mergence and update the cost matrix
		if (stopping_condition(best_sug, merging_solution, ref, task_limit, 14)) {
			cout << "Stop condition" << endl;
			break;
		}

		cout << "before:" << endl;
		print_solution(merging_solution);

		// perform symbolical mergence on the stage list
		merging_solution[best_sug.second].first.insert(
			merging_solution[best_sug.second].first.begin(), 
			merging_solution[best_sug.first].first.begin(), 
			merging_solution[best_sug.first].first.end());

		// perform symbolical mergence on the PCB
		cout << "merging of " << best_sug.second << "=" << 
			merging_solution[best_sug.second].second->tasks.size() << 
			" with " <<	best_sug.first << "=" << 
			merging_solution[best_sug.first].second->tasks.size();
		merge_stages(merging_solution[best_sug.second].second, 
			merging_solution[best_sug.first].second, ref);
		cout << " resulting in " << 
			merging_solution[best_sug.second].second->tasks.size() <<
			" with reuse=" << get_reuse_factor(stages[best_sug.first], 
					stages[best_sug.second], args, ref) << endl;


		// remove merged stage
		merging_solution.erase(best_sug.first);

		cout << endl << "after:" << endl;
		print_solution(merging_solution);

		// remove merged line and column
		cost_m.remove_line(best_sug.first);
		cost_m.remove_column(best_sug.second-1);

		// update the new merged stage line
		for (int j=1+best_sug.second; j<cost_m.dimension; j++) {
			int inc = 7-get_reuse_factor(stages[best_sug.first], 
					stages[best_sug.second], args, ref);
			cost_m.increment(best_sug.second, j, inc);
		}

		// update the new merged stage column
		for (int i=0; i<best_sug.second; i++) {
			int inc = 7-get_reuse_factor(stages[best_sug.first], 
					stages[best_sug.second], args, ref);
			cost_m.increment(i, best_sug.second, inc);
		}

		cost_m.print_matrix();
		cost_m.print_list();
	}

	cout << endl << "Solution:" << endl;
	print_solution(merging_solution);

}

void generate_cost_m(R2dVL& cost_m, const vector<PipelineComponentBase*>& stages,
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {
	
	// go though all combinations of mergence
	for (int i=0; i<stages.size(); i++) {
		for (int j=i+1; j<stages.size(); j++) {
			cost_m.set(i, j, 2*stages[i]->tasks.size() -
				get_reuse_factor(stages[i], stages[j], args, ref));
			// cost_m.set(i, j, get_reuse_factor(stages[i], stages[j], args, ref));
		}
	}
}

list<pair<int, int>> get_best_merging_sug(R2dVL& cost_m) {
	list<pair<int, int>> sugs;
	int best_cost = INT_MAX;
	// int best_cost = 0;

	// go through all stages' cost in an ordered manner
	for (ll_elm_t<int>* cost = cost_m.vect_list.begin; cost != NULL; cost=cost->next) {
		if (cost->value == best_cost) {
			// if the cost is the same as the best, add it as another sugestion
			sugs.emplace_back(pair<int, int>(cost->i, cost->j));
		} else if (cost->value < best_cost) {
		// } else if (cost->value > best_cost) {
			// if this is a new best, clear the current solution and add this
			// as the only (or first) suggestion
			sugs.clear();
			sugs.emplace_back(pair<int, int>(cost->i, cost->j));
			best_cost = cost->value;
		}
	}

	return sugs;
}

// TEMPORARY IMPLEMENTATION
pair<int, int> break_ties(list<pair<int, int>> sugestion) {
	return *sugestion.begin();
}

bool stopping_condition(const pair<int, int>& best_sug, 
	const map<int, pair<list<int>, PipelineComponentBase*>>& merging_solution, 
	const map<string, list<ArgumentBase*>>& ref, SC_opt option, int task_limit_v) {

	switch (option) {
		case task_limit:
			return sc_task_limit(best_sug, merging_solution, ref, task_limit_v);
		default:
			cout << "Bad stoping condition option" << endl;
			exit(0);
	}
}

bool sc_task_limit(const pair<int, int>& best_sug, 
	const map<int, pair<list<int>, PipelineComponentBase*>>& merging_solution, 
	const map<string, list<ArgumentBase*>>& ref, int max_tasks) {

	PipelineComponentBase* curr = merging_solution.at(best_sug.second).second->clone();
	PipelineComponentBase* old = merging_solution.at(best_sug.first).second->clone();
	merge_stages(curr, old, ref);

	bool ret = curr->tasks.size() > max_tasks;

	delete curr;
	delete old;
	
	return ret;
}

/*******************************************************************/
/************************ Helper Functions *************************/
/*******************************************************************/

int list_sum(list<int> l) {
	int sum = 0;
	for (int e : l)
		sum += e;
	return sum;
}

void print_solution(map<int, pair<list<int>, PipelineComponentBase*>> merging_solution) {
	for (pair<int, pair<list<int>, PipelineComponentBase*>> bkt : merging_solution) {
		cout << "bucket " << bkt.first << endl;
		cout << "\twith stages:" << endl;
		for (int s : bkt.second.first)
			cout << "\t\t" << s << endl;
		cout << "\t and PCB with " << bkt.second.second->tasks.size() << " tasks" << endl;
	}
}

void print_solution2(map<int, pair<list<int>, PipelineComponentBase*>> merging_solution) {
	for (pair<int, pair<list<int>, PipelineComponentBase*>> bkt : merging_solution) {
		cout << "bucket " << bkt.first << endl;
		cout << "\twith stages:" << endl;
		for (int s : bkt.second.first)
			cout << "\t\t" << s << endl;
		cout << "\t and PCB with " << bkt.second.second->tasks.size() << " tasks" << endl;
	}
}

/*******************************************************************/
/****************************** R2dVL ******************************/
/*******************************************************************/

// responsable for initiating 
// template<typename T>
// R2dVL<T>::R2dVL(int dimension) {
R2dVL::R2dVL(int dimension) {
	this->dimension = dimension;

	vect_list.begin = NULL;

	ll_elm_t<int>* curr_e = NULL;
	ll_elm_t<int>* new_e = NULL;
	ll_elm_t<int>* first_e = NULL;

	// generate each line of the matrix
	for (int i=0; i<dimension-1; i++) {
		// generate a line (double backwards)
		vector<ll_elm_t<int>*>* curr_line = new vector<ll_elm_t<int>*>;
		for (int j=dimension-i; j>1; j--){
			// create a new element
			new_e = new ll_elm_t<int>();
			new_e->value = i*10+j-2;
			new_e->i = i;
			new_e->j = dimension-j+1;
			new_e->prev = curr_e;
			if (curr_e != NULL)
				curr_e->next = new_e;
			else
				first_e = new_e;
			curr_e = new_e;
			curr_line->emplace_back(curr_e);
		}

		// check if this is the first list
		// if such, the first element is the global first
		// of the ll_t
		if (vect_list.begin == NULL) {
			vect_list.begin = first_e;
		}

		// add the current line to the matrix structure of ll_t
		vect_list.da_matrix.emplace_back(curr_line);

		// add the last list element to last_list
		vect_list.last_list.emplace_back(curr_e);
	}
}

int R2dVL::get(int i, int j) {
	if (j-1 < i) {
		cout << __FILE__ << ":" << __LINE__ << " Bad use of get, j must be >= i+1" << endl;
		exit(0);
	}
	return vect_list.da_matrix.at(i)->at(j-i-1)->value;
}

void R2dVL::set(int i, int j, int new_value) {
	if (j-1 < i) {
		cout << __FILE__ << ":" << __LINE__ << " Bad use of get, j must be >= i+1" << endl;
		exit(0);
	}
	vect_list.da_matrix.at(i)->at(j-i-1)->value = new_value;
}

void R2dVL::increment(int i, int j, int inc) {
	if (j-1 < i) {
		cout << __FILE__ << ":" << __LINE__ << " Bad use of get, j must be >= i+1" << endl;
		exit(0);
	}
	vect_list.da_matrix.at(i)->at(j-i-1)->value = 
		vect_list.da_matrix.at(i)->at(j-i-1)->value + inc;
}

void R2dVL::remove_line(int i) {
	if (vect_list.begin == vect_list.da_matrix.at(i)->at(0)) {
		// first line is removed: just take the next of first line's last elem
		vect_list.begin = vect_list.last_list.at(i)->next;
	} else {
		// take the last element of the previous line and set its 'next' as 
		// the i-th line last's 'next'
		vect_list.last_list.at(i-1)->next->next = vect_list.last_list.at(i)->next;
		// update the i-th line last's next element 'prev' to be i-th-1
		// line last element
		vect_list.last_list.at(i-1)->next = vect_list.last_list.at(i)->next;
		// in case i is the last list, no need to update the i-th+1 element
		if (vect_list.last_list.at(i)->next != NULL)
			vect_list.last_list.at(i)->next->prev = vect_list.last_list.at(i-1);
	}
}

void R2dVL::remove_column(int j) {
	for (int i=0; i<j; i++) {
		// update prev's 'next' ref of the j-th-1 column on the i-th line
		if (vect_list.da_matrix.at(i)->at(j-1-i)->prev != NULL) {
			vect_list.da_matrix.at(i)->at(j-1-i)->prev->next = 
				vect_list.da_matrix.at(i)->at(j-1-i)->next;
		}
		// update next's 'prev' ref of the j-th+1 column on the i-th line
		if (vect_list.da_matrix.at(i)->at(j-1-i)->next != NULL) {
			vect_list.da_matrix.at(i)->at(j-1-i)->next->prev = 
				vect_list.da_matrix.at(i)->at(j-1-i)->prev;
		}
	}
}

// template <typename T>
// void R2dVL<T>::print() {
void R2dVL::print_list() {
	ll_elm_t<int>* curr = vect_list.begin;
	while (curr != NULL) {
		cout << "[" << curr->i << ":" << curr->j << "] = " << curr->value << endl;
		curr = curr->next;
	}
}

void R2dVL::print_matrix() {
	int last_size = dimension;
	for (vector<ll_elm_t<int>*>* line : vect_list.da_matrix) {
		for (int i=0; i<last_size-line->size(); i++)
			cout << ".\t";
		for (ll_elm_t<int>* elem : *line) {
			cout << elem->value << "\t";
		}
		cout << endl;
	}
	// cout << "---------------------------------" << endl;
	// for (vector<ll_elm_t<int>*>* line : vect_list.da_matrix) {
	// 	for (int i=0; i<last_size-line->size(); i++)
	// 		cout << ".\t";
	// 	for (ll_elm_t<int>* elem : *line) {
	// 		cout << elem->i << ":" << elem->j << "\t";
	// 	}
	// 	cout << endl;
	// }
}
