#ifndef REUSE_TREE_HPP
#define REUSE_TREE_HPP

#include <list>
#include <map>
#include <set>
#include <stack>
#include <iterator>

#include "../merging.hpp"

static int r_node_id = 0;
static int new_node_id() {return r_node_id++;};

typedef struct reuse_node_t {
	reuse_node_t* parent;
	int id;
	std::list<reuse_node_t*> children;
	PipelineComponentBase* stage_ref;
} reuse_node_t;

typedef struct {
	std::list<reuse_node_t*> parents;
	int height;
	// int cost;
} reuse_tree_t;

// Performs the stage mearging using the reuse tree algorithm
list<list<PipelineComponentBase*>> reuse_tree_merging(const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_bucket_size, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref,
	bool double_prunning);

list<list<PipelineComponentBase*>> balanced_reuse_tree_merging(const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_buckets, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

list<list<PipelineComponentBase*>> tc_balanced_reuse_tree_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_tasks, int n_nodes, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

#endif