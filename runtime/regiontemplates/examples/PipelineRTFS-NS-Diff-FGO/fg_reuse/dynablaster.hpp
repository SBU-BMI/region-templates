#ifndef DYNABLASTER_HPP
#define DYNABLASTER_HPP

#include <limits.h>

#include <list>
#include <map>
#include <set>
#include <stack>
#include <iterator>

#include "merging.hpp"

using namespace std;

template <typename T>
struct ll_elm_t {
	T value;
	int i;
	int j;
	ll_elm_t* next;
	ll_elm_t* prev;
};

template <typename T>
struct ll_t {
	// iterative access list
	ll_elm_t<T>* begin;

	// direct access matrix ( O(1) )
	vector<vector<ll_elm_t<T>*>*> da_matrix;

	// last line elements list (needed for line removal with O(1))
	vector<ll_elm_t<T>*> last_list;
};

enum SC_opt {
	task_limit
};

// Reductible 2d Vectoral List
// The 2d (or matrix) structure is mirrored
// and don't have the i=j elements
// template <typename T> class R2dVL {
class R2dVL {
public:
	int dimension;
	// ll_t<T> vect_list;
	ll_t<int> vect_list;

	R2dVL(int dimension);
	~R2dVL() {};

	int get(int i, int j);
	void set (int i, int j, int new_value);
	void increment (int i, int j, int inc);
	void remove_line(int i);
	void remove_column(int j);

	void print_list();
	void print_matrix();
};

// Performs the stage mearging using the reuse tree algorithm
list<list<PipelineComponentBase*>> db_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int size_limit, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

// Merging components
void generate_cost_m(R2dVL& cost_m, const vector<PipelineComponentBase*>& stages,
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);
list<pair<int, int>> get_best_merging_sug(R2dVL& cost_m);
pair<int, int> break_ties(list<pair<int, int>> sugestion);
bool stopping_condition(const pair<int, int>& best_sug, 
	const map<int, pair<list<int>, PipelineComponentBase*>>& merging_solution, 
	const map<string, list<ArgumentBase*>>& ref, SC_opt option, int task_limit_v);
bool sc_task_limit(const pair<int, int>& best_sug, 
	const map<int, pair<list<int>, PipelineComponentBase*>>& merging_solution, 
	const map<string, list<ArgumentBase*>>& ref, int max_tasks);


// Helper functions
int list_sum(list<int> l);
void print_solution(map<int, pair<list<int>, PipelineComponentBase*>> merging_solution);

#endif