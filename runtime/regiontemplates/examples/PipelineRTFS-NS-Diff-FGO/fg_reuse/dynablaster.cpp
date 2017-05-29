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


	// keep attempting to merge until merging sugestion is bad
	while (true) {
		// find best merging sugestion on cost matrix

		// break the ties (if any)

		// verify if merging sugestion achieves stoping condition

		// if not, perform mergence and update the cost matrix
	}
}

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

void R2dVL::put(int i, int j, int new_value) {
	if (j-1 < i) {
		cout << __FILE__ << ":" << __LINE__ << " Bad use of get, j must be >= i+1" << endl;
		exit(0);
	}
	vect_list.da_matrix.at(i)->at(j-i-1)->value = new_value;
}

void R2dVL::remove_line(int i) {
	if (i == 0) {
		vect_list.begin = vect_list.last_list.at(0)->next;
	} else {
		vect_list.last_list.at(i-1)->next->next = vect_list.last_list.at(i)->next;
		vect_list.last_list.at(i-1)->next = vect_list.last_list.at(i)->next;
		vect_list.last_list.at(i)->next->prev = vect_list.last_list.at(i-1);
	}
}

void R2dVL::remove_column(int j) {
	for (int i=0; i<=j; i++) {
		vect_list.da_matrix.at(i)->at(j-i)->prev->next = vect_list.da_matrix.at(i)->at(j-i)->next;
		vect_list.da_matrix.at(i)->at(j-i)->next->prev = vect_list.da_matrix.at(i)->at(j-i)->prev;
	}
}

// template <typename T>
// void R2dVL<T>::print() {
void R2dVL::print_list() {
	ll_elm_t<int>* curr = vect_list.begin;
	while (curr != NULL) {
		cout << curr->value << endl;
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
}
