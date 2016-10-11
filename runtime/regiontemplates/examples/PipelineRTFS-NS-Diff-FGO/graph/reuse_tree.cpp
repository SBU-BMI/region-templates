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
			// std::cout << "[recursive_insert_stage][" << curr_level << "] " << s->getId() 
			// 	<< " is reusable with " << n->stage_ref->getId() << std::endl;
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
			list<reuse_node_t*> current_leafs_parent_list = generate_leafs_parent_list(n, height-1);
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
		list<reuse_node_t*> current_leafs_parent_list = generate_leafs_parent_list(n, reuse_tree.height-1);
		leafs_parent_list.insert(leafs_parent_list.begin(), 
			current_leafs_parent_list.begin(), current_leafs_parent_list.end());
	}

	return leafs_parent_list;
}

// Remove the node from its parent and keep recurring until there is
// a parent with at least one still usable node
void recursive_update_parent(reuse_tree_t& t, reuse_node_t* n) {
	if (n->parent == NULL) {
		t.parents.remove(n);
		return;
	}

	n->parent->children.remove(n);

	if (n->parent->children.size() == 0)
		recursive_update_parent(t, n->parent);
}

// Prunes the reuse tree based on the leafs_parent_list, returning a list of new buckets
// to be merged, and updates the tree internal structure
list<list<PipelineComponentBase*>> prune_leaf_level(reuse_tree_t& reuse_tree, 
	const list<reuse_node_t*>& leafs_parent_list, int max_bucket_size) {
	
	list<list<PipelineComponentBase*>> new_buckets;

	// attempt to make buckets of every children's leaf list
	for (reuse_node_t* n : leafs_parent_list) {
		std::cout << "[prune_leaf_level] parent " << n->stage_ref->getId() 
			<< " has " << n->children.size() << " children and mbs " 
			<< max_bucket_size << std::endl;

		// finishes the loop if we can't perform any more simple merging operations
		if (n->children.size() < max_bucket_size)
			continue;

		std::cout << "[prune_leaf_level] parent " << n->stage_ref->getId() 
			<< " has reuse" << std::endl;

		// Keep making buckets until the remaining stages number 
		// is less than max_bucket_size
		int count;
		for (list<reuse_node_t*>::iterator c=n->children.begin(); 
			c!=n->children.end() && n->children.size() >= max_bucket_size;) {

			while (n->children.size() >= max_bucket_size) {
				list<PipelineComponentBase*> new_bucket;
				count = 0;
				std::cout << "[prune_leaf_level] starting bucket of "
					<< n->stage_ref->getId() << " with size 0" << std::endl;
				while (count < max_bucket_size) {
					std::cout << "[prune_leaf_level]\tadding "
						<< (*c)->stage_ref->getId() << " to the bucket" << std::endl;
					new_bucket.emplace_back((*c)->stage_ref);
					c = n->children.erase(c);
					count++;
				}
				new_buckets.emplace_back(new_bucket);
			}
		}

		// remove the parent node from its parent if there is no more leafs
		// also, keep performing this recursively
		if (n->children.size() == 0) {
			std::cout << "[prune_leaf_level] performing recursive update on "
				<< n->stage_ref->getId() << std::endl;
			recursive_update_parent(reuse_tree, n);
		}
	}

	// TODO: keep merging until each child has at most one leaf
	// this merging must attempt to keep the leafs of the same child together

	return new_buckets;
}

// Sends the leaf nodes of the reuse tree one level up, to their parents
void move_reuse_tree_up(reuse_tree_t& reuse_tree, 
	const list<reuse_node_t*>& leafs_parent_list) {
	
	// performs the move up operation for every leafs' parent node n
	for (reuse_node_t* n : leafs_parent_list) {
		// adds the node children to its parent
		n->parent->children.insert(n->parent->children.begin(), 
			n->children.begin(), n->children.end());

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
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref) {

	list<list<PipelineComponentBase*>> solution;

	reuse_tree_t reuse_tree = generate_reuse_tree(stages_to_merge, args, ref);


	// keep prunning and rising the tree until the root height
	while (reuse_tree.height > 2) {
		print_reuse_tree(reuse_tree);
		// perform current height merging iteration
		list<reuse_node_t*> leafs_parent_list = generate_leafs_parent_list(reuse_tree);
		// print_leafs_parent_list(leafs_parent_list);
		list<list<PipelineComponentBase*>> new_buckets = prune_leaf_level(
			reuse_tree, leafs_parent_list, max_bucket_size);
		move_reuse_tree_up(reuse_tree, leafs_parent_list);

		// add new_buckets to final solution
		solution.insert(solution.begin(), new_buckets.begin(), new_buckets.end());
	}

	print_reuse_tree(reuse_tree);

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
					while (count < max_bucket_size) {
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
				for (reuse_node_t* nn : n->children)
					final_stage.emplace_back(nn->stage_ref);
				solution.emplace_back(final_stage);
			}
		}
	}

	std::cout << "FINAL SOLUTION" << std::endl;
	for (list<PipelineComponentBase*> b : solution) {
		std::cout << "bucket of size " << b.size() << " with cost " 
			<< calc_stage_proc(b, args, ref) << std::endl;
		for (PipelineComponentBase* s : b) {
			std::cout << "\t" << s->getId() << std::endl;
		}
	}
	std::cout << std::endl << std::endl << std::endl;

	return solution;
}