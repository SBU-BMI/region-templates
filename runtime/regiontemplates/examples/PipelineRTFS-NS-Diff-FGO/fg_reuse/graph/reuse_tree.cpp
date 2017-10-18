#include "reuse_tree.hpp"

#include <iostream>
#include <string>
#include <set>
#include <algorithm>

void print_reuse_node(const reuse_node_t* n, int tt) {
	for (int i=0; i<tt; i++)
		std::cout << "\t";
	string p = n->parent==NULL ? " no parent" : to_string(n->parent->stage_ref->getId());
	std::cout << n->stage_ref->getId() << ", parent: " << p 
		<< " and id " << n->id << std::endl;
	if (n->children.size() == 0) {
		for (int i=0; i<tt; i++)
			std::cout << "\t";
		cout << "leaf with stage " << n->stage_ref->getId() << endl;
	}
	for (reuse_node_t* nn : n->children)
		print_reuse_node(nn, tt+1);
}

void print_leafs_parent_list(const list<reuse_node_t*>& l) {
	for (reuse_node_t* n : l)
		std::cout << n->stage_ref->getId() << std::endl;
}

int get_nodes_cost (list<reuse_node_t*> children) {
	
	int cost = 0;

	while (children.size() > 0) {
		list<reuse_node_t*> new_children;
		for (reuse_node_t* child : children) {
			cost++;
			if (child->children.size() > 0) {
				new_children.insert(new_children.end(), 
					child->children.begin(), child->children.end());
			}
		}
		children = new_children;
	}

	return cost;
}

int get_rt_cost (const reuse_tree_t& rt) {
	return get_nodes_cost(rt.parents);
}

void print_reuse_tree(const reuse_tree_t& t) {
	std::cout << "tree of height " << t.height 
		<< " and cost " << get_rt_cost(t) << ":" << std::endl;
	for (reuse_node_t* n : t.parents)
		print_reuse_node(n, 1);
}

void print_bucket_list(list<reuse_tree_t> buckets) {
	int i=0;
	for (reuse_tree_t rt : buckets) {
		cout << "bucket " << i++ << " _________________________________________" << endl;
		print_reuse_tree(rt);
	}
}

bool compare_rt (reuse_tree_t first, reuse_tree_t second) {
	return get_rt_cost(first) > get_rt_cost(second);
}

bool compare_rn_by_id (const reuse_node_t* first, const reuse_node_t* second) {
	return first->id > second->id;
}

list<PipelineComponentBase*> rnode_to_PCB_list(const reuse_node_t* n) {
	list<PipelineComponentBase*> ret, tmp;
	if (n->children.size() == 0) {
		ret.emplace_back(n->stage_ref);
	} else {
		for (reuse_node_t* nn : n->children) {
			tmp = rnode_to_PCB_list(nn);
			ret.insert(ret.end(), tmp.begin(), tmp.end());
		}
	}
	return ret;
}

list<PipelineComponentBase*> tree_to_PCB_list(const reuse_tree_t& t) {
	list<PipelineComponentBase*> ret, tmp;
	for (reuse_node_t* n : t.parents) {
		tmp = rnode_to_PCB_list(n);
		ret.insert(ret.end(), tmp.begin(), tmp.end());
	}
	return ret;
}

void recursive_insert_stage(list<reuse_node_t*>& node_list, PipelineComponentBase* s, 
	reuse_node_t* parent, int height, int curr_level, const map<int, ArgumentBase*>& args,
	const map<string, list<ArgumentBase*>>& ref) {

	// stopping condition
	if (curr_level >= height) {
		// std::cout << "[recursive_insert_stage][" << curr_level << "] stop for " 
		// 	<< s->getId() << std::endl;
		return;
	}

	// attempt to find if there is a reuse oportunity on this level
	reuse_node_t* reusable_node = NULL;
	for (reuse_node_t* n : node_list) {
		// palealive solution to merging two equal stages
		if (s->getId() ==  n->stage_ref->getId()) {
			cout << "[SAME_COMPARE] " << s->getId() << endl;
			return;
		}

		// check if the reuse factor matches the minimum required for this level
		// std::cout << "[recursive_insert_stage][" << curr_level << "] checking " 
		// 	<< s->getId() << " with " << n->stage_ref->getId() << std::endl;
		if (get_reuse_factor(s, n->stage_ref, args, ref) > curr_level) {
			// std::cout << "[recursive_insert_stage][" << curr_level << "] " 
			//	<< s->getId() << " is reusable with " 
			//	<< n->stage_ref->getId() << std::endl;
			reusable_node = n;
			break;
		}
	}

	// if there isn't a reusable node then create a new node for s
	if (reusable_node == NULL) {
		// std::cout << "[recursive_insert_stage][" << curr_level 
		// 	<< "] no reusable node for " << s->getId() << std::endl;
		reuse_node_t* new_node = new reuse_node_t;
		node_list.emplace_back(new_node);
		new_node->parent = parent;
		new_node->stage_ref = s;
		new_node->id = new_node_id();
		recursive_insert_stage(new_node->children, s, 
			new_node, height, curr_level+1, args, ref);
	}
	// if there is a reusable stage then continue the recursive traverse on this node
	else {
		// std::cout << "[recursive_insert_stage][" << curr_level 
		// 	<< "] recurring" << std::endl;
		recursive_insert_stage(reusable_node->children, s, 
			reusable_node, height, curr_level+1, args, ref);
	}
}

// Generates the reuse tree based on the reuse level of the stages
reuse_tree_t generate_reuse_tree(const list<PipelineComponentBase*>& stages, 
	const map<int, ArgumentBase*>& args, const map<string, list<ArgumentBase*>>& ref) {
	reuse_tree_t reuse_tree;

	// set the height based on the number of tasks every stage has
	reuse_tree.height = ref.size();

	// add every stage to the reuse tree
	for (PipelineComponentBase* s : stages) {
		// find the reuse level (i.e reuse tree height) node of the stage
		recursive_insert_stage(reuse_tree.parents, s, NULL, 
			reuse_tree.height, 0, args, ref);
	}

	return reuse_tree;
}

// Traverse the tree, getting the leafs' parents' references
list<reuse_node_t*> generate_leafs_parent_list(reuse_node_t* reuse_node, int height) {
	list<reuse_node_t*> leafs_parent_list;

	// check stoping condition: is a leaf node parent?
	if (height == 1) {
		// if it is a leaf node parent, add the reference to itself
		leafs_parent_list.emplace_back(reuse_node);
	} else {
		// if it's another mid node, perform the recursive for all its children
		for (reuse_node_t* n : reuse_node->children) {
			list<reuse_node_t*> current_leafs_parent_list = 
				generate_leafs_parent_list(n, height-1);
			leafs_parent_list.insert(leafs_parent_list.begin(), 
				current_leafs_parent_list.begin(), current_leafs_parent_list.end());
		}
	}

	return leafs_parent_list;
}

// Generate the leafs' parents' references list from the root node
list<reuse_node_t*> generate_leafs_parent_list(reuse_tree_t& reuse_tree) {
	list<reuse_node_t*> leafs_parent_list;

	for (reuse_node_t* n : reuse_tree.parents) {
		list<reuse_node_t*> current_leafs_parent_list = 
			generate_leafs_parent_list(n, reuse_tree.height-1);
		leafs_parent_list.insert(leafs_parent_list.begin(), 
			current_leafs_parent_list.begin(), current_leafs_parent_list.end());
	}

	return leafs_parent_list;
}

// Remove the node from its parent and keep recurring until there is
// a parent with at least one still usable node
void recursive_remove_parent(reuse_tree_t& t, reuse_node_t* n) {
	if (n->parent == NULL) {
		t.parents.remove(n);
	} else {
		n->parent->children.remove(n);

		if (n->parent->children.size() == 0)
			recursive_remove_parent(t, n->parent);
	}
}

void cluster_merging(reuse_tree_t& reuse_tree, 
	list<reuse_node_t*>& leafs_parent_list, 
	list<list<PipelineComponentBase*>>& new_buckets,
	int max_bucket_size) {

	// std::cout << "starting merging with " << leafs_parent_list.size() 
	// 	<< " parents" << std::endl;
	// cluster the parents nodes of leafs_parent_list by their parents
	multimap<reuse_node_t*, reuse_node_t*> parents_clusters;
	set<reuse_node_t*> parents_clusters_keys;
	for (reuse_node_t* n : leafs_parent_list) {
		parents_clusters.emplace(n->parent, n);
		parents_clusters_keys.emplace(n->parent);
	}

	// performs the merging for each one of the clusters
	for (set<reuse_node_t*>::iterator p=parents_clusters_keys.begin(); 
		p!=parents_clusters_keys.end(); p++) {

		// std::cout << "\titerating on cluster of parent node " 
		// 	<< (*p)->stage_ref->getId() << std::endl;

		multimap<reuse_node_t*, reuse_node_t*>::iterator c_it = 
			parents_clusters.lower_bound(*p);
		multimap<reuse_node_t*, reuse_node_t*>::iterator c_end = 
			parents_clusters.upper_bound(*p);

		// makes the parents multimap, grouped by number of children
		multimap<int, reuse_node_t*> parents_by_children_count;
		set<int> parents_by_children_count_ids;
		for (; c_it!=c_end; c_it++) {
			parents_by_children_count.emplace(c_it->second->children.size(), c_it->second);
			parents_by_children_count_ids.emplace(c_it->second->children.size());
		}

		// keeps attempting to merge the yet unmerged parents' children
		for (set<int>::reverse_iterator c_count=parents_by_children_count_ids.rbegin();
			c_count!=parents_by_children_count_ids.rend(); c_count++) {

			// std::cout << "\t\ttrying to merge the parents of c_count " << 
			// 	*c_count << std::endl;

			multimap<int, reuse_node_t*>::iterator n_it = 
				parents_by_children_count.lower_bound(*c_count);
			multimap<int, reuse_node_t*>::iterator n_end = 
				parents_by_children_count.upper_bound(*c_count);

			// Attempts to find a matching parent to make a perfect merge 
			// with each current cluster parent
			for (; n_it!=n_end; n_it++) {
				int current_bucket_size = n_it->first;

				//the new bucket search stack
				stack<reuse_node_t*> bucket_stack;
				bucket_stack.push(n_it->second);

				// std::cout << "\t\t\tstarting a stack with parent " << 
				// 	n_it->second->stage_ref->getId() << " and b_size " 
				// 	<< current_bucket_size << std::endl;

				// starts the merge attempts with the nodes on the current cluster
				multimap<int, reuse_node_t*>::iterator n_it2 = next(n_it, 1);
				for (; n_it2!=n_end; n_it2++) {
					if (current_bucket_size + n_it->first <= max_bucket_size) {
						bucket_stack.push(n_it2->second);
						current_bucket_size += n_it2->first;
						// std::cout << "\t\t\t\tfrom the same " << n_it2->first 
						// 	<< " c_count, added " << n_it2->second->stage_ref->getId() 
						// 	<< " to the stack" << std::endl;
					} else
						break;
				}

				// keeps attempting the merging process with other clusters
				set<int>::reverse_iterator c_count2 = next(c_count, 1);

				// A new parent will be added to the stack if possible. If the perfect 
				// match isn't found at the end of an insert iteration then the stack
				// is popped and the search continues from the stages with children_count
				// next to the popped parent. If any merging is impossible than the 
				// stack will become empty, with current_bucket_size=0.
				while (current_bucket_size != max_bucket_size 
					&& current_bucket_size != 0) {

					// std::cout << "\t\t\t\tsearching other c_counts with b_size " 
					// 	<< current_bucket_size << std::endl;

					// starts the search on the current c_count2
					for (; c_count2!=parents_by_children_count_ids.rend(); c_count2++) {
						n_it2 = parents_by_children_count.lower_bound(*c_count2);
						multimap<int, reuse_node_t*>::iterator n_end2 = 
							parents_by_children_count.upper_bound(*c_count2);
						
						// std::cout << "\t\t\t\t\tsearching c_count " 
						// 	<< *c_count2 << std::endl;

						// goes through current c_count2 cluster
						for (; n_it2!=n_end2; n_it2++) {
							if (current_bucket_size + n_it2->first <= max_bucket_size) {
								bucket_stack.push(n_it2->second);
								current_bucket_size += n_it2->first;
								// std::cout << "\t\t\t\t\t\tadded " 
								// 	<< n_it2->second->stage_ref->getId() 
								// 	<< " to the stack with size "
								// 	<< n_it2->first << std::endl;
							} else
								break;
						}
					}

					// if it isn't a perfect sized bucket then pop the last added parent
					if (current_bucket_size != max_bucket_size) {
						// std::cout << "\t\t\t\tbad bucket size: " 
						// 	<< current_bucket_size << std::endl;
						c_count2 = find(parents_by_children_count_ids.rbegin(), 
							parents_by_children_count_ids.rend(),
							bucket_stack.top()->children.size());
						current_bucket_size -= *c_count2;
						c_count2 = next(c_count2, 1);
						bucket_stack.pop();
						// std::cout << "\t\t\t\tremoving " << *c_count2 
						// 	<< " from the stack" << std::endl;
					}
				}

				// checks if a viable bucket was found
				if (current_bucket_size == max_bucket_size) {

					// std::cout << "\t\t\tnew solution found:" << std::endl;

					// adds the new bucket to the solution
					list<PipelineComponentBase*> new_bucket;
					while (!bucket_stack.empty()) {
						reuse_node_t* n = bucket_stack.top();
						bucket_stack.pop();
						// adds all of the parent's children
						for (list<reuse_node_t*>::iterator nn=n->children.begin();
							nn!=n->children.end(); ) {

							// std::cout << "\t\t\t\t" << 
							// 	(*nn)->stage_ref->getId() << std::endl;
							new_bucket.emplace_back((*nn)->stage_ref);
							nn = n->children.erase(nn);
						}
						
						// performs the recursive parent removal if necessary
						if (n->children.size() == 0)
							recursive_remove_parent(reuse_tree, n);

						// removes the merged nodes from the leafs_parent_list
						leafs_parent_list.remove(n);
					}
					new_buckets.emplace_back(new_bucket);

					// breaks the search since the nodes removal messes the iterators up
					return;
				}
			}
		}

		// std::cout << "\tcluster of parent node " << (*p)->stage_ref->getId() 
		// 	<< " don't have any more reuse oportunities" << std::endl;
		
		// If this point was reached it means that the current cluster don't have any more 
		// reuse oportunities to be taken, so we remove the cluster's parents from the
		// leafs_parent_list
		c_it = parents_clusters.lower_bound(*p);
		c_end = parents_clusters.upper_bound(*p);
		for (; c_it!=c_end; c_it++) {
			leafs_parent_list.remove(c_it->second);
		}
	}
}

// Prunes the reuse tree based on the leafs_parent_list, returning a list of new buckets
// to be merged, and updates the tree internal structure
list<list<PipelineComponentBase*>> prune_leaf_level(reuse_tree_t& reuse_tree, 
	list<reuse_node_t*> leafs_parent_list, int max_bucket_size, bool double_prunning) {
	
	list<list<PipelineComponentBase*>> new_buckets;

	// pruning first pass -------------------
	// attempt to make buckets of every children's leaf list
	for (list<reuse_node_t*>::iterator n=leafs_parent_list.begin();
		n!=leafs_parent_list.end(); ) {
		// std::cout << "[prune_leaf_level] parent " << (*n)->stage_ref->getId() 
		// 	<< " has " << (*n)->children.size() << " children and mbs " 
		// 	<< max_bucket_size << std::endl;

		// finishes the loop if we can't perform any more simple merging operations
		if ((*n)->children.size() < max_bucket_size) {
			n++;
			continue;
		}

		// std::cout << "[prune_leaf_level] parent " << (*n)->stage_ref->getId() 
		// 	<< " has reuse" << std::endl;

		// Keeps making buckets until the remaining stages number 
		// is less than max_bucket_size
		for (list<reuse_node_t*>::iterator c=(*n)->children.begin(); 
			c!=(*n)->children.end() && (*n)->children.size() >= max_bucket_size;) {

			while ((*n)->children.size() >= max_bucket_size) {
				list<PipelineComponentBase*> new_bucket;
				int count = 0;
				// std::cout << "[prune_leaf_level] starting bucket of "
				// 	<< (*n)->stage_ref->getId() << " with size 0" << std::endl;
				while (count < max_bucket_size) {
					// std::cout << "[prune_leaf_level]\tadding "
					// 	<< (*c)->stage_ref->getId() << " to the bucket" << std::endl;
					new_bucket.emplace_back((*c)->stage_ref);
					c = (*n)->children.erase(c);
					count++;
				}
				new_buckets.emplace_back(new_bucket);
			}
		}

		// removes the parent node from its parent if there is no more leafs
		// also, keeps performing this recursively
		if ((*n)->children.size() == 0) {
			// std::cout << "[prune_leaf_level] performing recursive update on "
			// 	<< (*n)->stage_ref->getId() << std::endl;
			recursive_remove_parent(reuse_tree, *n);

			// removes the parent from the list if there are no more children to merge
			n = leafs_parent_list.erase(n);
		} else
			n++;
	}

	// pruning second pass -------------------
	if (double_prunning)
		while (leafs_parent_list.size() > 0)
			cluster_merging(reuse_tree, leafs_parent_list, new_buckets, max_bucket_size);

	return new_buckets;
}

// Sends the leaf nodes of the reuse tree one level up, to their parents
void move_reuse_tree_up(reuse_tree_t& reuse_tree, 
	const list<reuse_node_t*>& leafs_parent_list) {
	
	// performs the move up operation for every leafs' parent node n
	for (reuse_node_t* n : leafs_parent_list) {
		// adds the node children to its parent
		for (reuse_node_t* nn : n->children) {
			nn->parent = n->parent;
			n->parent->children.emplace_back(nn);
		}
		
		// removes itself from the parent node
		n->parent->children.remove(n);
	}

	// update the tree height
	reuse_tree.height--;
}

// Performs the stage mearging using the reuse tree algorithm
list<list<PipelineComponentBase*>> reuse_tree_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_bucket_size, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref,
	bool double_prunning) {

	list<list<PipelineComponentBase*>> solution;

	reuse_tree_t reuse_tree = generate_reuse_tree(stages_to_merge, args, ref);


	// keep prunning and rising the tree until the root height
	while (reuse_tree.height > 2) {
		// std::cout << "before:" << std::endl;
		// print_reuse_tree(reuse_tree);
		// perform current height merging iteration
		list<reuse_node_t*> leafs_parent_list = generate_leafs_parent_list(reuse_tree);
		// print_leafs_parent_list(leafs_parent_list);
		list<list<PipelineComponentBase*>> new_buckets = prune_leaf_level(
			reuse_tree, leafs_parent_list, max_bucket_size, double_prunning);
		// std::cout << "after:" << std::endl;
		// print_reuse_tree(reuse_tree);
		move_reuse_tree_up(reuse_tree, leafs_parent_list);

		// add new_buckets to final solution
		solution.insert(solution.begin(), new_buckets.begin(), new_buckets.end());
	}

	// print_reuse_tree(reuse_tree);

	// add the remaining unmerged stages to the final solution as single stage buckets
	if (reuse_tree.parents.size() > 0) {
		// go through all parent nodes
		for (reuse_node_t* n : reuse_tree.parents) {
			// go though all parent child nodes
			for (list<reuse_node_t*>::iterator nn=n->children.begin(); 
				nn!=n->children.end() && n->children.size() >= max_bucket_size;) {

				// keep creating buckets if possible
				while (n->children.size() >= max_bucket_size) {
					list<PipelineComponentBase*> new_bucket;
					int count = 0;
					// std::cout << "starting empty bucket:" << std::endl;
					while (count < max_bucket_size) {
						// std::cout << "adding to bucket " 
						// 	<< (*nn)->stage_ref->getId() << std::endl;
						new_bucket.emplace_back((*nn)->stage_ref);
						nn = n->children.erase(nn);
						count++;
					}
					solution.emplace_back(new_bucket);
				}
			}

			// create a final bucket with all remaining stages, if there are any
			if (n->children.size() > 0) {
				list<PipelineComponentBase*> final_stage;
				// std::cout << "starting final empty bucket:" << std::endl;
				for (reuse_node_t* nn : n->children) {
					final_stage.emplace_back(nn->stage_ref);
					// std::cout << "adding to bucket " 
					// 		<< nn->stage_ref->getId() << std::endl;
				}
				solution.emplace_back(final_stage);
			}
		}
	}

	// std::cout << "FINAL SOLUTION" << std::endl;
	// for (list<PipelineComponentBase*> b : solution) {
	// 	for (PipelineComponentBase* s : b) {
	// 		std::cout << "\t" << s->getId() << std::endl;
	// 	}
	// 	std::cout << "bucket of size " << b.size() << " with cost " 
	// 		<< calc_stage_proc(b, args, ref) << std::endl;
	// }
	// std::cout << std::endl << std::endl << std::endl;

	return solution;
}

reuse_node_t* nt_clone(reuse_node_t* nt) {
	reuse_node_t* new_nt = new reuse_node_t;
	reuse_node_t* tmp_nt;
	new_nt->stage_ref = nt->stage_ref;
	new_nt->id = nt->id;
	new_nt->parent = NULL;
	for (reuse_node_t* n : nt->children) {
		tmp_nt = nt_clone(n);
		tmp_nt->parent = new_nt;
		new_nt->children.emplace_back(tmp_nt);
	}
	return new_nt;
}

reuse_tree_t rt_clone(reuse_tree_t rt) {
	reuse_tree_t new_rt;
	new_rt.height = rt.height;
	for (reuse_node_t* n : rt.parents) {
		new_rt.parents.emplace_back(nt_clone(n));
	}
	return new_rt;
}

void nt_delete(reuse_node_t *n) {
	for (reuse_node_t* nn : n->children)
		nt_delete(nn);
	delete n;
}

void rt_delete(const reuse_tree_t& rt) {
	for (reuse_node_t* n : rt.parents) {
		nt_delete(n);
	}
}

bool remove_node_by_id(list<reuse_node_t*>& ns, int id) {
	if (ns.size() == 1 && ns.front()->children.size() == 0 && ns.front()->id == id) {
		// cout << "clearing ns" << endl;
		// print_reuse_node(ns.front(), 0);
		nt_delete(ns.front());
		ns.clear();
		return true;
	}

	for (list<reuse_node_t*>::iterator n=ns.begin(); n!=ns.end(); n++) {
		if ((*n)->id == id) {
			// cout << "1 erasing " << (*n)->id << " from " << (*n)->parent->id << endl;
			// print_reuse_node((*n), 0);
			nt_delete(*n);
			ns.erase(n);
			return true;
		}
		if (remove_node_by_id((*n)->children, id) && (*n)->children.size() == 0) {
			// cout << "2 erasing " << (*n)->id << " from " << (*n)->parent->id << endl;
			// print_reuse_node((*n), 0);
			nt_delete(*n);
			ns.erase(n);
			return true;
		}
	}

	return false;
}

void swap_node(reuse_tree_t& from_rt, reuse_tree_t&  to_rt, reuse_node_t* n,
	const map<int, ArgumentBase*>& args, const map<string, list<ArgumentBase*>>& ref) {
	
	// cout << "before insert:" << endl;
	// print_reuse_tree(from_rt);
	// print_reuse_tree(to_rt);
	
	for (PipelineComponentBase* pcb : rnode_to_PCB_list(n)) {
		recursive_insert_stage(to_rt.parents, pcb, 
			NULL, to_rt.height, 0, args, ref);
	}

	// cout << "after insert:" << endl;
	// print_reuse_tree(from_rt);
	// print_reuse_tree(to_rt);
	
	remove_node_by_id(from_rt.parents, n->id);

	// cout << "after remove:" << endl;
	// print_reuse_tree(from_rt);
	// print_reuse_tree(to_rt);
}

reuse_node_t* balance(list<reuse_node_t*> children, reuse_tree_t big_rt, 
	reuse_tree_t small_rt, const map<int, ArgumentBase*>& args, 
	const map<string, list<ArgumentBase*>>& ref, int k=0) {

	int unbalance = get_rt_cost(big_rt) - get_rt_cost(small_rt);
	reuse_node_t* solution = NULL;

	// cout << "starting balancing " << k << " of " << unbalance << endl;

	list<reuse_node_t*> rt_both;
	set_intersection(big_rt.parents.begin(), big_rt.parents.end(),
					 small_rt.parents.begin(), small_rt.parents.end(),
					 back_inserter(rt_both), compare_rn_by_id);
	
	if (children.size() == 0 || (rt_both.size() == big_rt.parents.size() &&
								 rt_both.size() == small_rt.parents.size())) {
		// cout << "[balance " << k << "] no more children" << endl;
		return NULL;
	}

	while (children.size() == 1 && children.front()->children.size() > 0) {
		children = children.front()->children;
		// cout << "[balance " << k << "] one child only" << endl;
	}

	// if (children.size() > 0) {
	// make sure that there is only one search attempt
	//   per node cost, and also, try any balance from 
	//   the next level
	map<int, reuse_node_t*> children_by_cost;
	// cout << "[balance] analysing " << children.size() << " children";
	// for (int i=0; i<k; i++)
	// 	cout << "#";
	// cout << endl;
	for (reuse_node_t* n : children) {
		// try to solve the unbalancement with the best solution
		//   from the next level
		// cout << "[balance " << k << "] trying to balance children of " << endl;
		// print_reuse_node(n, 0);
		reuse_node_t* sol = balance(n->children, big_rt, small_rt, args, ref, k+1);
		// cout << "[balance " << k << "] checking if last balance got a solution" << endl;
		if (sol != NULL) {
			// cout << "[balance " << k << "] Solution:" << endl;
			reuse_tree_t big_tmp = rt_clone(big_rt);
			reuse_tree_t small_tmp = rt_clone(small_rt);
			swap_node(big_tmp, small_tmp, sol, args, ref);
			int cost_tmp = abs(get_rt_cost(big_tmp) - get_rt_cost(small_tmp));
			rt_delete(big_tmp);
			rt_delete(small_tmp);
			// print_reuse_node(sol, 0);
			// cout << "big_rt with cost " << get_rt_cost(big_tmp) << endl;
			// print_reuse_tree(big_tmp);
			// cout << "small_rt with cost " << get_rt_cost(small_tmp) << endl;
			// print_reuse_tree(small_tmp);
			if (cost_tmp < unbalance) {
				solution = sol;
				unbalance = cost_tmp;
				// cout << "[balance " << k << "] new best solution of cost " 
				// 	<< unbalance << endl;
				// print_reuse_node(solution, 0);
			}
		}

		// make sure that the current level solution test will
		//    only be made once
		children_by_cost[get_nodes_cost(n->children)] = n;
	}

	// try the current level solutions
	// cout << "[balance " << k << "] trying current level solutions" << endl;
	for (pair<int, reuse_node_t*> p : children_by_cost) {
		// cout << "[balance " << k << "] trying node" << endl;
		// print_reuse_node(p.second, 0);
		reuse_tree_t big_tmp = rt_clone(big_rt);
		reuse_tree_t small_tmp = rt_clone(small_rt);
		swap_node(big_tmp, small_tmp, p.second, args, ref);
		// cout << "big_rt with cost " << get_rt_cost(big_tmp) << endl;
		// print_reuse_tree(big_tmp);
		// cout << "small_rt with cost " << get_rt_cost(small_tmp) << endl;
		// print_reuse_tree(small_tmp);
		int cost_tmp = abs(get_rt_cost(big_tmp) - get_rt_cost(small_tmp));
		rt_delete(big_tmp);
		rt_delete(small_tmp);
		if (cost_tmp < unbalance) {
			solution = p.second;
			unbalance = cost_tmp;
			// cout << "[balance " << k << "] new best solution of cost " 
			// 		<< unbalance << endl;
		}
	}
	// } 

	// cout << "done balancing " << k << " with new balance: " << unbalance 
	// 	<< " and solution node" << endl;
	if (solution != NULL) {
		// print_reuse_node(solution,0);
		reuse_tree_t big_tmp = rt_clone(big_rt);
		reuse_tree_t small_tmp = rt_clone(small_rt);
		swap_node(big_tmp, small_tmp, solution, args, ref);
		rt_delete(big_tmp);
		rt_delete(small_tmp);
	}
	// cout << "big_rt with cost " << get_rt_cost(big_tmp) << endl;
	// print_reuse_tree(big_tmp);
	// cout << "small_rt with cost " << get_rt_cost(small_tmp) << endl;
	// print_reuse_tree(small_tmp);

	return solution;
}

int count_stages(std::multiset<reuse_tree_t,bool(*)(reuse_tree_t,reuse_tree_t)> rts) {
	int count = 0;
	for (reuse_tree_t rt : rts)
		for (reuse_node_t* n : rt.parents)
			count += rnode_to_PCB_list(n).size();

	return count;
}

inline reuse_tree_t ms_front(std::multiset<reuse_tree_t,
	bool(*)(reuse_tree_t,reuse_tree_t)> buckets) {

	return *(buckets.begin());
}

inline reuse_tree_t ms_back(std::multiset<reuse_tree_t,
	bool(*)(reuse_tree_t,reuse_tree_t)> buckets) {

	return *(buckets.rbegin());
}

int get_unbal_group(std::multiset<reuse_tree_t,bool(*)(reuse_tree_t,reuse_tree_t)>
	buckets) {
	
	int unbalance = 0;
	int max_unbalance = get_rt_cost(ms_front(buckets));
	for (reuse_tree_t rt : buckets) {
		if (get_rt_cost(rt) < max_unbalance)
			break;
		unbalance++;
	}
	return unbalance;
}

list<list<PipelineComponentBase*>> balanced_reuse_tree_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_buckets, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	list<list<PipelineComponentBase*>> solution;

	// generate reuse tree
	reuse_tree_t initial_reuse_tree = generate_reuse_tree(stages_to_merge, args, ref);

	if (max_buckets > stages_to_merge.size()) {
		// max_buckets = initial_reuse_tree.parents.size();
		max_buckets = stages_to_merge.size()*0.9;
		// cout << "MAX_BUCKETS RESIZED!!!!!! to " << max_buckets << endl;
	}

	// cout << "init" << endl;
	// print_reuse_tree(initial_reuse_tree);

	// perform full merge
	list<reuse_node_t*> children = initial_reuse_tree.parents;
	while (children.size() < max_buckets) {
		list<reuse_node_t*> new_children;
		for (reuse_node_t* child : children) {
			new_children.insert(new_children.end(), 
				child->children.begin(), child->children.end());
		}
		children = new_children;
	}

	// cout << "buckets" << endl;
	// for (reuse_node_t* rn : children) {
	// 	print_reuse_node(rn, 0);
	// 	cout << endl;
	// }

	// generate first bucket list
	//list<reuse_tree_t> buckets;
	bool(*fn_pt)(reuse_tree_t,reuse_tree_t) = compare_rt;
	std::multiset<reuse_tree_t,bool(*)(reuse_tree_t,reuse_tree_t)> buckets (fn_pt);
	for (reuse_node_t* n : children) {
		// prepare current bucket
		reuse_tree_t bucket;
		bucket.height = initial_reuse_tree.height;

		// insert each leaf node on the new bucket
		list<reuse_node_t*> child_list = n->children;
		while (child_list.size() > 0) {
			list<reuse_node_t*> new_child_list;
			for (reuse_node_t* child : child_list) {
				if (child->children.size() == 0) {
					recursive_insert_stage(bucket.parents, child->stage_ref, NULL, 
						bucket.height, 0, args, ref);
				} else {
					new_child_list.insert(new_child_list.end(), 
						child->children.begin(), child->children.end());
				}
			}
			child_list = new_child_list;
		}

		//buckets.emplace_back(bucket);
		buckets.insert(bucket);
	}

	// cout << "buckets" << endl;
	// for (reuse_tree_t rt : buckets) {
	// 	print_reuse_tree(rt);
	// 	cout << endl;
	// }

	// sort bucket list by descending cost
	// buckets.sort(compare_rt);

	// cout << "sorted buckets" << endl;
	// for (reuse_tree_t rt : buckets) {
	// 	print_reuse_tree(rt);
	// 	cout << endl;
	// }

	// perform single merges, if needed
	while (buckets.size() > max_buckets) {
		// list<reuse_tree_t> new_buckets;
		std::multiset<reuse_tree_t,bool(*)(reuse_tree_t,reuse_tree_t)> 
			new_buckets (fn_pt);

		// remove the most costly buckets from the 'fold' merging and add them to the
		//   current solution 'as is'
		list<reuse_tree_t> folded_buckets;
		int f;
		if (buckets.size() > 2*max_buckets)
			f = buckets.size() % 2;
		else
			f = 2*max_buckets - buckets.size();
		// cout << "f=" << f << endl;

		for (reuse_tree_t rt : buckets) {
			if (f-- <= 0) {
				folded_buckets.emplace_back(rt);
			} else {
				// new_buckets.emplace_back(rt);
				new_buckets.insert(rt);
			}
		}

		// perform fold merging
		while (folded_buckets.size() > 0) {
			reuse_tree_t rt;
			rt.height = folded_buckets.begin()->height;
			rt.parents.insert(rt.parents.end(),
				folded_buckets.back().parents.begin(),
				folded_buckets.back().parents.end());
			rt.parents.insert(rt.parents.end(), 
				folded_buckets.front().parents.begin(), 
				folded_buckets.front().parents.end());
			folded_buckets.pop_front();
			folded_buckets.pop_back();
			// new_buckets.emplace_back(rt);
			new_buckets.insert(rt);
		}

		buckets = new_buckets;
	}

	// cout << "semifinal buckets" << endl;
	// for (reuse_tree_t rt : buckets) {
	// 	print_reuse_tree(rt);
	// 	cout << endl;
	// }

	// sort again bucket list by descending cost
	// buckets.sort(compare_rt);
	// cout << "got " << count_stages(buckets) << " stages" << endl;

	// balance the task costs
	bool improvement = true;
	int unbalance = get_rt_cost(ms_front(buckets)) - get_rt_cost(ms_back(buckets));
	while (improvement) {
		// cout << "unbalancement: " << unbalance << " with " 
		// 	<< get_unbal_group(buckets) << " maxed buckets" << endl;
		// cout << "got " << count_stages(buckets) << " stages" << endl;
		improvement = false;

		// get the bucket with the highest cost
		reuse_tree_t big_rt = rt_clone(ms_front(buckets));
		// cout << "big_rt cost: " << get_rt_cost(big_rt) << endl;

		// IMPROVEMENT: maybe try balancing big_rt with all other buckets
		// IMPROVEMENT[2]: maybe try balancing big_rt with all buckets with
		//    the same min cost

		// get the bucket with the lowest cost
		reuse_tree_t small_rt = rt_clone(ms_back(buckets));
		// cout << "small_rt cost: " << get_rt_cost(small_rt) << endl;

		// try to balance big and small buckets
		list<reuse_node_t*> children = big_rt.parents;
		reuse_tree_t big_rt_tmp = rt_clone(big_rt);
		reuse_tree_t small_rt_tmp = rt_clone(small_rt);
		reuse_node_t* n = balance(big_rt.parents, big_rt_tmp, small_rt_tmp, args, ref);
		rt_delete(big_rt_tmp);
		rt_delete(small_rt_tmp);

		// verify if there is an improvement
		if (n != NULL) {
			swap_node(big_rt, small_rt, n, args, ref);
			int unbalance_tmp = abs(get_rt_cost(big_rt) - get_rt_cost(small_rt));
			// cout << "balancement attempt: " << unbalance_tmp << endl;
			// if the balanced rt with the greatest cost is as costly as 
			//   the current most expensive bucket (i.e. ms_front(buckets)) then
			//   there isn't an improvement
			if (max(get_rt_cost(big_rt), get_rt_cost(small_rt)) 
				< get_rt_cost(ms_front(buckets))) {

				// if such remove old, unbalanced rt's ...
				rt_delete(ms_front(buckets));
				buckets.erase(buckets.begin());
				rt_delete(ms_back(buckets));
				buckets.erase(--buckets.end());
				// ... add new, balanced, rt's ...
   // TODO: try adding hints....................................................
				buckets.insert(big_rt);
				buckets.insert(--buckets.end(), small_rt);
				// ... re-sort the bucket list ...
				// buckets.sort(compare_rt);
				// ... and update the unbalance value
				// unbalance_tmp = get_rt_cost(ms_front(buckets)) - get_rt_cost(ms_back(buckets));
				improvement = true;
				unbalance = unbalance_tmp;
				// cout << "improved!" << endl;
			} 
			// } else
			// 	cout << "bad improvement: " << unbalance_tmp 
			// 		<< " vs " << unbalance << endl;
			

			// cout << "imprv buckets" << endl;
			// for (reuse_tree_t rt : buckets) {
			// 	print_reuse_tree(rt);
			// 	cout << endl;
			// }
		} 
		// } else
		// 	cout << "no balancement" << endl;
	}


	// cout << "final buckets" << endl;
	// for (reuse_tree_t rt : buckets) {
	// 	print_reuse_tree(rt);
	// 	cout << endl;
	// }

	// convert the tree structured solution to a PCB bucket list
	for (reuse_tree_t rt : buckets) {
		list<PipelineComponentBase*> bucket = tree_to_PCB_list(rt);
		solution.emplace_back(bucket);
		rt_delete(rt);
	}

	// cout << "done" << endl;

	return solution;

}

list<list<PipelineComponentBase*>> tc_balanced_reuse_tree_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_tasks, int n_nodes, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	list<list<PipelineComponentBase*>> solution;

	int full_m_tasks = 0;

	int buckets_wo_reuse;
	int buckets_w_reuse;
	float reuse_factor_inv;
	int total_tasks = stages_to_merge.size() * 
		stages_to_merge.front()->tasks.size();

	// perform a full-merge on which there is a single bucket with all stages 
	std::list<PipelineComponentBase*> full_merge = 
		merge_stages_full(cpy_stage_list(stages_to_merge), args, ref);
	// get the number of tasks of the single full-merged bucket
	for (PipelineComponentBase* s : full_merge) {
		if (s->reused == NULL) {
			full_m_tasks += s->tasks.size();
		}
	}
	reuse_factor_inv = (float)(full_m_tasks) / (float)(total_tasks);

	buckets_wo_reuse = ceil(total_tasks / max_tasks);
	buckets_w_reuse = ceil(buckets_wo_reuse * reuse_factor_inv);

	cout << "buckets_wo_reuse: " << buckets_wo_reuse << endl;
	cout << "buckets_w_reuse: " << buckets_w_reuse << endl;
	cout << "n_nodes: " << n_nodes << endl;

	// get the best performance case with the memory constraint
	if (n_nodes > buckets_w_reuse)
		buckets_w_reuse = n_nodes;
	if (n_nodes > buckets_wo_reuse)
		buckets_wo_reuse = n_nodes;

	// run reuse alg with optimistic max_buxkets
	solution = balanced_reuse_tree_merging(cpy_stage_list(stages_to_merge), 
		all_stages, buckets_w_reuse, args, ref);

	// check if all buckets comply with the memory restriction
	bool fesible = true;
	for (list<PipelineComponentBase*> b : solution) {
		int task_count = 0;
		for (PipelineComponentBase* s : b) {
			if (s->reused == NULL) {
				task_count += s->tasks.size();
			}
		}
		if (task_count > max_tasks) {
			fesible = false;
			break;
		}
	}

	// If optimistic solution isn't good enough, go to persimistic solution
	// Even if the optimistic solution is good the algorithm must run again on
	// the real stages_to_merge list, instead of a copy, to commit the internal
	// changes
	if (!fesible) {
		solution = balanced_reuse_tree_merging(stages_to_merge, 
			all_stages, buckets_wo_reuse, args, ref);
		cout << "optimistic is unfesiable" << endl;
	} else
		solution = balanced_reuse_tree_merging(stages_to_merge, 
			all_stages, buckets_w_reuse, args, ref);

	// get the reuse factor - for comparison reasons only
	full_m_tasks = 0;
	int maxc = 0;
	int minc = INT_MAX;
	for (list<PipelineComponentBase*> b : solution) {
		list<PipelineComponentBase*> tmp;
		for (PipelineComponentBase* s : b) {
			if (s->reused == NULL) {
				tmp.emplace_back(s->clone());
			}
		}
		int curr = 0;
		for (PipelineComponentBase* s : merge_stages_full(tmp, args, ref)) {
			curr += s->tasks.size();
		}
		if (curr > maxc)
			maxc = curr;
		if (curr < minc)
			minc = curr;
		full_m_tasks += curr;
	}

	cout << "solution inv reuse factor X best inv reuse factor: " 
		<< (float)(full_m_tasks) / (float)(total_tasks) 
		<< " X " << reuse_factor_inv << endl;
	cout << "[min/max] cost of the " << solution.size() 
		<< " buckets: [" << minc << "/" << maxc << "]" << endl;

	return solution;
}