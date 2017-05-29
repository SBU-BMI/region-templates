#ifndef DYNABLASTER_HPP
#define DYNABLASTER_HPP

#include <list>
#include <map>
#include <set>
#include <stack>
#include <iterator>

#include "merging.hpp"

using namespace std;

// Performs the stage mearging using the reuse tree algorithm
list<list<PipelineComponentBase*>> db_merging(const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int size_limit, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

template <typename T>
struct ll_elm_t {
	T value;
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

// Reductible 2d Vectoral List
// The 2d (or matrix) structure is mirrored
// and don't have the i=j elements
// template <typename T> class R2dVL {
class R2dVL {
private:
	int dimension;
	// ll_t<T> vect_list;
	ll_t<int> vect_list;

public:
	R2dVL(int dimension);
	~R2dVL() {};

	int get(int i, int j);
	void put (int i, int j, int new_value);
	void remove_line(int i);
	void remove_column(int j);

	void print_list();
	void print_matrix();
};


#endif