#include "reuse_tree.hpp"

#include <iostream>
#include <string>

void print_reuse_node(const reuse_node_t* n, int tt) {
	for (int i=0; i<tt; i++)
		std::cout << "\t";
	string p = n->parent==NULL ? " no parent" : to_string(n->parent->stage_ref->getId());
	std::cout << n->stage_ref->getId() << ", parent: " << p << std::endl;
	for (reuse_node_t* nn : n->children)
		print_reuse_node(nn, tt+1);
}

void print_reuse_tree(const reuse_tree_t& t) {
	std::cout << "tree of height " << t.height << ":" << std::endl;
	for (reuse_node_t* n : t.parents)
		print_reuse_node(n, 1);
}

void print_leafs_parent_list(const list<reuse_node_t*>& l) {
	for (reuse_node_t* n : l)
		std::cout << n->stage_ref->getId() << std::endl;
}

int get_rt_cost (const reuse_tree_t& rt) {
	
	list<reuse_node_t*> children = rt.parents;
	int cost = 0;

	while (children.size() > 0) {
		list<reuse_node_t*> new_children;
		for (reuse_node_t* child : children) {
			cost++;
			if (child->children.size() > 0) {
				new_children.insert(new_children.end(); 
					child->children.begin(), child->children.end());
			} else {
				cost++;
			}
		}
		children = new_children;
	}

	return cost;
}

bool compare_rt (const reuse_tree_t& first, const reuse_tree_t& second) {
	return get_rt_cost(first) > get_rt_cost(second);
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
								(*nn)->stage_ref->getId() << std::endl;
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

list<list<PipelineComponentBase*>> balanced_reuse_tree_merging(
	const list<PipelineComponentBase*>& stages_to_merge, 
	const map<int, PipelineComponentBase*> &all_stages, int max_buckets, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	list<list<PipelineComponentBase*>> solution;
	
	// generate reuse tree
	reuse_tree_t initial_reuse_tree = generate_reuse_tree(stages_to_merge, args, ref);

	// perform full merge
	list<reuse_node_t*> children = initial_reuse_tree.children;
	while (children.size() < max_buckets) {
		list<reuse_node_t*> new_children;
		for (reuse_node_t* child : children) {
			new_children.insert(new_children.end(), 
				child->children.begin(), child->children.end());
		}
		children = new_children;
	}

	// generate first bucket list
	list<reuse_tree_t> buckets;
	for (reuse_node_t* n : children) {
		// prepare current bucket
		reuse_tree_t bucket;
		bucket.height = initial_reuse_tree.height;

		// insert each leaf node on the new bucket
		list<reuse_node_t*> child_list = n.children;
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
	}

	// sort bucket list by descending cost
	buckets.sort(compare_rt);

	// perform single merges, if needed
	if (buckets.size() > max_buckets) {
		// remove the most costly buckets from the 'fold' merging and add them to the
		//   current solution 'as is'
		list<reuse_tree_t> folded_buckets;
		int f = 2*(buckets.size()-floor(buckets.size()/max_buckets)*max_buckets);
		for (reuse_tree_t rt : buckets) {
			if (f-- <= 0) {
				folded_buckets.emplace_back(rt);
			}
		}

		// perform fold merging
	}

	// balance the task costs
}

