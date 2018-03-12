#include "cutting_algorithms.hpp"

mincut::weight_t get_reuse_factor(mincut::subgraph_t s1, mincut::subgraph_t s2, std::map<size_t, int> id2task,  
	std::map<int, PipelineComponentBase*> current_stages, std::map<int, ArgumentBase*> &args,
	std::map<string, std::list<ArgumentBase*>> ref) {

	// get the first stage as a base stage
	mincut::subgraph_t::iterator s1_it = s1.begin();
	PipelineComponentBase* current1 = current_stages[id2task[*s1_it]]->clone();
	current1->setLocation(PipelineComponentBase::WORKER_SIDE);
	s1_it++;
	for (; s1_it!=s1.end(); s1_it++) {
		PipelineComponentBase* clone1 = current_stages[id2task[*s1_it]]->clone();
		clone1->setLocation(PipelineComponentBase::WORKER_SIDE);
		if (merging_condition(current1, clone1, args, ref))
			merge_stages(current1, clone1, ref);
		else {
			for (ReusableTask* t : clone1->tasks) {
				current1->tasks.emplace_back(t->clone());
			}
		}
		clone1->remove_outputs = true;
		delete clone1;
	}

	// get the first stage as a base stage
	mincut::subgraph_t::iterator s2_it = s2.begin();
	PipelineComponentBase* current2 = current_stages[id2task[(*s2_it)]]->clone();
	current2->setLocation(PipelineComponentBase::WORKER_SIDE);
	for (s2_it++; s2_it!=s2.end(); s2_it++) {
		PipelineComponentBase* clone2 = current_stages[id2task[*s2_it]]->clone();
		clone2->setLocation(PipelineComponentBase::WORKER_SIDE);
		if (merging_condition(current2, clone2, args, ref))
			merge_stages(current2, clone2, ref);
		else {
			for (ReusableTask* t : clone2->tasks) {
				current2->tasks.emplace_back(t->clone());
			}
		}
		clone2->remove_outputs = true;
		delete clone2;
	}

	int ret = current1->tasks.size()>current2->tasks.size()?current1->tasks.size():current2->tasks.size();

	// clear memory
	current1->remove_outputs = true;
	delete current1;
	current2->remove_outputs = true;
	delete current2;

	return ret;
}

bool cutting_condition(std::list<PipelineComponentBase*> current, float nrS_mksp, float max_mem, 
	std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref) {

	if (calc_stage_proc(current, args, ref) > nrS_mksp || calc_stage_mem(current, args, ref) > max_mem)
		return true;
	else
		return false;
}

pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> get_cut(std::list<PipelineComponentBase*> current_stages, 
	const std::map<int, PipelineComponentBase*> &all_stages, std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref) {

	// generate the reuse matrix and the std::map real-task to min-cut id
	size_t id = 0;
	size_t n = current_stages.size();

	// dynamic allocation of adjMat is needed because if n is waaay too big the stack will overflow
	mincut::weight_t** adjMat = new mincut::weight_t *[n];
	for (size_t i=0; i<n; i++) {
		adjMat[i] = new mincut::weight_t [n];
	}
	
	std::map<size_t, int> id2task;
	for (std::list<PipelineComponentBase*>::iterator s1=current_stages.begin(); s1!= current_stages.end(); s1++, id++) {
		id2task[id] = (*s1)->getId();
		// std::cout << "std::map: " << id << ":" << (*s1)->getId() << std::endl;
		size_t id_j = id;
		for (std::list<PipelineComponentBase*>::iterator s2=s1; s2!= current_stages.end(); s2++, id_j++) {
			if (id == id_j)
				adjMat[id][id] = 0;
			else {
				adjMat[id][id_j] = get_reuse_factor(*s1, *s2, args, ref);
				adjMat[id_j][id] = adjMat[id][id_j];
			}
		}
	}

	// std::cout << std::endl;
	// adj_mat_print(adjMat, id2task, n);
	// std::cout << std::endl;

	// send adjMat to mincut algorithm
	std::list<mincut::cut_t> cuts = mincut::min_cut(n, adjMat);

	// get the cut with the minimal weight, using the number of merged tasks as a tiebreaker
	mincut::cut_t best_cut = cuts.front();
	mincut::weight_t best_weight = mincut::_cut_w(best_cut);
	int best_num_tasks = get_reuse_factor(mincut::_cut_s1(best_cut), mincut::_cut_s2(best_cut), 
		id2task, all_stages, args, ref);
	mincut::weight_t r;

	for (mincut::cut_t c : cuts) {
		// std::cout << "cut: " << mincut::_cut_w(c) << ":" << std::endl;
		// std::cout << "\tS1:" << std::endl;
		// for (mincut::_id_t id : mincut::_cut_s1(c))
		// 	std::cout << "\t\t" << id2task[id] << std::endl;
		// std::cout << "\tS2:" << std::endl;
		// for (mincut::_id_t id : mincut::_cut_s2(c))
		// 	std::cout << "\t\t" << id2task[id] << std::endl;
		// std::cout << std::endl;

		// updates min cut if the weight is less that the best so far
		if (mincut::_cut_w(c) < best_weight) {
			best_cut = c;
			best_weight = mincut::_cut_w(c);
			best_num_tasks = get_reuse_factor(mincut::_cut_s1(c), mincut::_cut_s2(c), 
				id2task, all_stages, args, ref);
		} 
		// updates min cut if the weight are the same but this cut is more balanced
		else if (mincut::_cut_w(c) == best_weight && (r = get_reuse_factor(mincut::_cut_s1(c), 
				mincut::_cut_s2(c), id2task, all_stages, args, ref)) < best_num_tasks) {
			best_cut = c;
			best_num_tasks = r;
		}
	}

	// convert cut to PCB std::lists for return
	pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> best_cut_pcb;
	for (mincut::_id_t i : mincut::_cut_s1(best_cut))
		best_cut_pcb.first.emplace_back(all_stages.at(id2task[i]));
	for (mincut::_id_t i : mincut::_cut_s2(best_cut))
		best_cut_pcb.second.emplace_back(all_stages.at(id2task[i]));

	// clean adjMat
	for (size_t i=0; i<n; i++) {
		delete[] adjMat[i];
	}
	delete[] adjMat;

	return best_cut_pcb;
}

std::list<std::list<PipelineComponentBase*>> recursive_cut(std::list<PipelineComponentBase*> rem, const std::map<int, PipelineComponentBase*> &all_stages, 
	int max_bucket_size, int max_cuts, std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref) {

	std::list<std::list<PipelineComponentBase*>> last_solution;

	// finishes if we can't make any more cuts (i.e size=1) or if we shouldn't (i.e max_cuts == 1)
	if (rem.size() == 1) {
		last_solution.emplace_back(rem);
		// std::cout << "retuned n=" << n << ", size=" << rem.size() << std::endl;
		return last_solution;
	}

	float last_mksp = FLT_MAX;
	float current_mksp = FLT_MAX/10;
	std::list<PipelineComponentBase*> rest;
	std::list<std::list<PipelineComponentBase*>> buckets;
	bool not_evaluated = true;

	// execute while there is improvement
	while(current_mksp < last_mksp || not_evaluated) {
		last_mksp = current_mksp;
		not_evaluated = true;

		// make a copy of this solution in case the next isn't better
		last_solution = buckets;

		if (rem.size() == 1) {
			// std::cout << "retuned state for size=1, n=" << n << ":" << std::endl;
			// for (std::list<PipelineComponentBase*> b : last_solution) {
			// 	std::cout << "\tbucket with cost " << calc_stage_proc(b, ref) << ":" << std::endl;
			// 	for (PipelineComponentBase* s : b) {
			// 		std::cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
			// 	}
			// }
			return last_solution;
		}

		// std::cout << "rem:" << std::endl;
		// for (PipelineComponentBase* s : rem) {
		// 	std::cout << "\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
		// }
		// std::cout << "rest:" << std::endl;
		// for (PipelineComponentBase* s : rest) {
		// 	std::cout << "\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
		// }

		// cut the current bucket
		pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		std::list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		std::list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		// std::cout << "cut: " << std::endl;
		// std::cout << "\tc1" << std::endl;
		// for (PipelineComponentBase* s : c1)
		// 	std::cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
		// std::cout << "\tc2" << std::endl;
		// for (PipelineComponentBase* s : c2)
		// 	std::cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;

		// join c2 with the remaining of rem
		c2.insert(c2.begin(), rest.begin(), rest.end());

		// verify if the max_bucket_size constraint was satisfied
		std::cout << "[" << max_cuts << "]" << "c1.size=" << c1.size() << std::endl;
		if (c1.size() <= max_bucket_size) {
			buckets = recursive_cut(c2, all_stages, max_bucket_size, max_cuts-1, args, ref);
			buckets.emplace_back(c1);
			
			// recalculate current max makespan and set rem backup on last_rem
			float mksp;
			current_mksp = 0;
			for (std::list<PipelineComponentBase*> b : buckets) {
				mksp = calc_stage_proc(b, args, ref);
				if (mksp > current_mksp) {
					current_mksp = mksp;
				}
			}
			std::cout << "mksp: " << current_mksp << std::endl;

			not_evaluated = false;

			return buckets;

			// std::cout << "merged state n=" << n << ":" << std::endl;
			// for (std::list<PipelineComponentBase*> b : buckets) {
			// 	std::cout << "\tbucket with cost " << calc_stage_proc(b, args, ref) << ":" << std::endl;
			// 	for (PipelineComponentBase* s : b) {
			// 		std::cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
			// 	}
			// }
		}

		// set next iteration PCB lists
		rem = c1;
		rest = c2;
	}

	return last_solution;
}

std::list<std::list<PipelineComponentBase*>> montecarlo_cut(std::list<PipelineComponentBase*> rem, const std::map<int, PipelineComponentBase*> &all_stages, 
	int n, std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref) {

	std::list<std::list<PipelineComponentBase*>> ret;

	std::cout << "[montecarlo_cut] n=" << n << std::endl;

	if (n == 1 || rem.size() == 1) {
		ret.emplace_back(rem);
		std::cout << "[montecarlo_cut] n=" << n << std::endl;
		return ret;
	}

	// perform cuts until there is no more tasks to cut, or, there is no more need for another cut
	for (int i=0; i<n-1; i++) {
		pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		std::list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		std::list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		rem = c1;
		ret.emplace_back(c2);

		if (rem.size() == 1) {
			break;
		}
	}

	// add last bucket 
	ret.emplace_back(rem);

	// std::cout << "[montecarlo_cut] n=" << n << "ret:" << std::endl;
	// int i=0;
	// for (std::list<PipelineComponentBase*> bucket : ret) {
	// 	std::cout << "\tb:" << i++ << std::endl;
	// 	for (PipelineComponentBase* s : bucket)
	// 		std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
	// }

	return ret;
}

void montecarlo_dep(int n, std::list<PipelineComponentBase*> c1, std::list<PipelineComponentBase*> c2,
	const std::map<int, PipelineComponentBase*> &all_stages, std::map<int, ArgumentBase*> &args,
	std::map<string, std::list<ArgumentBase*>> ref, float &best_mksp, float &best_var,
	pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> &best_cut) {

	std::cout << "[montecarlo_recursive_cut] n=" << n << " running cut" << std::endl;
	std::cout << "\tc1 - cost=" << calc_stage_proc(c1, args, ref) << ":" << std::endl;
	for (PipelineComponentBase* s : c1)
		std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
	std::cout << "\tc2 - cost=" << calc_stage_proc(c2, args, ref) << ":" << std::endl;
	for (PipelineComponentBase* s : c2)
		std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
	
	std::list<std::list<PipelineComponentBase*>> c = montecarlo_cut(c2, all_stages, n-1, args, ref);
	
	// calculates i cut cost with rest c
	std::cout << "[montecarlo_cut] n=" << n << "ret:" << std::endl;
	int ii=0;
	// mksp selection metric
	float mksp;
	float max_mksp = calc_stage_proc(c1, args, ref);
	float avg = calc_stage_proc(c1, args, ref);
	for (std::list<PipelineComponentBase*> bucket : c) {
		mksp = calc_stage_proc(bucket, args, ref);
		avg += mksp;
		if (mksp > max_mksp)
			max_mksp = mksp;
		std::cout << "\tb" << ii++ << " - mksp=" << mksp<< std::endl;
		for (PipelineComponentBase* s : bucket)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
	}

	// standard deviation selection metric
	avg /= (c.size()+1);
	float var = (calc_stage_proc(c1, args, ref)-avg)*(calc_stage_proc(c1, args, ref)-avg);
	for (std::list<PipelineComponentBase*> bucket : c) {
		mksp = calc_stage_proc(bucket, args, ref);
		var += (mksp-avg)*(mksp-avg);
	}
	var /= (c.size()+1);

	std::cout << "with mksp=" << max_mksp << std::endl;
	std::cout << "with var=" << var << std::endl;

	// // maybe double if?
	if (max_mksp < best_mksp || (max_mksp == best_mksp && var < best_var)) {
	#pragma omp critical
	{
	// updates best cut, if this is the case
	if (max_mksp < best_mksp || (max_mksp == best_mksp && var < best_var)) {
		best_mksp = max_mksp;
		best_var = var;
		std::list<PipelineComponentBase*> flat_c;
		for (std::list<PipelineComponentBase*> bucket : c)
			flat_c.insert(flat_c.begin(), bucket.begin(), bucket.end());
		best_cut = pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>>(c1, flat_c);
	}
	}
	}
}

std::list<std::list<PipelineComponentBase*>> montecarlo_recursive_cut(std::list<PipelineComponentBase*> rem, 
	const std::map<int, PipelineComponentBase*> &all_stages, int n, std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref) {

	// std::list<pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>>> current_cuts;
	pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> best_cut;
	std::list<std::list<PipelineComponentBase*>> final_cut;
	float best_mksp = FLT_MAX;
	float best_var = FLT_MAX;
	int r_size = rem.size();

	std::cout << "[montecarlo_recursive_cut] n=" << n << std::endl;

	// stoping condition
	if (n == 1 || rem.size() == 1) {
		final_cut.emplace_back(rem);
		std::cout << "[montecarlo_recursive_cut] n=" << n << " retuned1" << std::endl;
		std::cout << "\tb0" << std::endl;
		for (PipelineComponentBase* s : rem)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
		return final_cut;
	}

	#pragma omp parallel
	{
	#pragma omp single
	{
	// attempt all possible cuts on this level
	std::list<PipelineComponentBase*> rest;
	while (rem.size() > 1) {
		// attempt a cut
		pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		// store the cut for the rest to be calculated later
		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		std::list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		std::list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		// add rest to c2
		c2.insert(c2.begin(), rest.begin(), rest.end());

		#pragma omp task
		montecarlo_dep(n, c1, c2, all_stages, args, ref, best_mksp, best_var, best_cut);

		std::cout << "[montecarlo_recursive_cut] n=" << n << " adding cut" << std::endl;
		std::cout << "\tc1:" << std::endl;
		for (PipelineComponentBase* s : c1)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
		std::cout << "\tc2:" << std::endl;
		for (PipelineComponentBase* s : c2)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;

		// update rem to get a new cut
		rem = c1;
		rest = c2;
	}
	}
	#pragma barrier
	}

	// std::cout << "---------------------------------------------------" << std::endl;

	// #pragma omp parallel
	// {
	// #pragma omp single
	// {
	// // runs all dependencies a la montecarlo
	// for (std::list<pair<std::list<PipelineComponentBase*>, std::list<PipelineComponentBase*>>>::iterator i=current_cuts.begin();
	// 		i!=current_cuts.end(); i++) {
	// 	// WARNING: best cut is shared between all montecarlo_dep runs
	// 	#pragma omp task
	// 	montecarlo_dep(n, i->first, i->second, all_stages, args, ref, best_mksp, best_var, best_cut);
	// }
	// }

	std::cout << "[montecarlo_recursive_cut] n=" << n << " best cut of the stage with mksp=" << best_mksp << std::endl;
		std::cout << "\tc1:" << std::endl;
		for (PipelineComponentBase* s : best_cut.first)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
		std::cout << "\tc2:" << std::endl;
		for (PipelineComponentBase* s : best_cut.second)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;

	// recursive call on the best rest
	final_cut = montecarlo_recursive_cut(best_cut.second, all_stages, n-1, args, ref);
	final_cut.emplace_back(best_cut.first);

	std::cout << "[montecarlo_recursive_cut] n=" << n << " final cut:" << std::endl;
	int i=0;
	for (std::list<PipelineComponentBase*> bucket : final_cut) {
		std::cout << "\tb:" << i++ << std::endl;
		for (PipelineComponentBase* s : bucket)
			std::cout << "\t\t" << s->getId() << ":" << s->getName() << std::endl;
	}

	return final_cut;
}