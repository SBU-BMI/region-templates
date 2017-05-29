#include "fgm.hpp"

void fgm::merge_stages_fine_grain(int algorithm, const std::map<int, PipelineComponentBase*> &all_stages, 
	const std::map<int, PipelineComponentBase*> &stages_ref, std::map<int, PipelineComponentBase*> &merged_stages, 
	std::map<int, ArgumentBase*> expanded_args, int size_limit, bool shuffle, string dakota_filename) {

	// attempt merging for each stage type
	for (std::map<int, PipelineComponentBase*>::const_iterator ref=stages_ref.cbegin(); ref!=stages_ref.cend(); ref++) {
		// get only the stages from the current stage_ref
		std::list<PipelineComponentBase*> current_stages;
		filter_stages(all_stages, ref->second->getName(), current_stages, shuffle);

		std::cout << "[merge_stages_fine_grain] Generating tasks..." << std::endl;
		
		// generate all tasks
		int nrS = 0;
		double max_nrS_mksp = 0;
		for (list<PipelineComponentBase*>::iterator s=current_stages.begin(); s!=current_stages.end(); ) {
			// if the stage isn't composed of reusable tasks then 
			(*s)->tasks = task_generator(ref->second->tasksDesc, *s, expanded_args);
			if ((*s)->tasks.size() == 0) {
				merged_stages[(*s)->getId()] = *s;
				
				// makespan calculations
				nrS++;
				if ((*s)->getMksp() > max_nrS_mksp)
					max_nrS_mksp = (*s)->getMksp();

				s = current_stages.erase(s);
			} else
				s++;
		}

		// if there are no stages left to attempt to merge, or only one stage, don't perform any merging
		if (current_stages.size() == 1) {
			merged_stages[(*current_stages.begin())->getId()] = *current_stages.begin();
			continue;
		} else if (current_stages.size() == 0) {
			continue;
		}

		std::list<std::list<PipelineComponentBase*>> solution;

		switch (algorithm) {
			case 0:
				// no fine grain merging
				for (PipelineComponentBase* p : current_stages) {
					std::list<PipelineComponentBase*> single_stage_bucket;
					single_stage_bucket.emplace_back(p);
					solution.emplace_back(single_stage_bucket);
				}
				break;

			case 1:
				// naive merging - size_limit is the max bucket size
				for (std::list<PipelineComponentBase*>::iterator s=current_stages.begin(); s!=current_stages.end(); s++) {
					int i;
					std::list<PipelineComponentBase*> bucket;
					bucket.emplace_back(*s);
					for (i=1; i<ceil(size_limit); i++) {
						if ((++s)==current_stages.end())
							break;
						bucket.emplace_back(*s);
						std::cout << "\tadded " << (*s)->getId() << " to the bucket" << std::endl;
					}
					solution.emplace_back(bucket);
					if (s==current_stages.end())
						break;
				}
				break;

			case 2:
				// smart recursive cut - size_limit is the max bucket size
				solution = recursive_cut(current_stages, all_stages, 
					size_limit, ceil(current_stages.size()/size_limit), 
					expanded_args, ref->second->tasksDesc);
				break;

			case 3:
				// reuse-tree merging - size_limit is the max bucket size
				solution = reuse_tree_merging(current_stages, all_stages, 
					size_limit, expanded_args, ref->second->tasksDesc, false);
				break;
			
			case 4:
				// reuse-tree merging with double prunning - size_limit is the max bucket size
				solution = reuse_tree_merging(current_stages, all_stages, 
					size_limit, expanded_args, ref->second->tasksDesc, true);
				break;
			case 5:
				// dynablaster merging
				solution = db_merging(current_stages, all_stages, 
					size_limit, expanded_args, ref->second->tasksDesc);
				break;

		}

		// write merging solution
		ofstream solution_file;
		solution_file.open(dakota_filename + "-b" + std::to_string(size_limit) + "merging_solution.log", ios::trunc);

		std::cout << std::endl << "solution:" << std::endl;
		solution_file << "solution:" << std::endl;
		int total_tasks=0;
		for (std::list<PipelineComponentBase*> b : solution) {
			std::cout << "\tbucket with " << b.size() << " stages and cost "
				<< calc_stage_proc(b, expanded_args, ref->second->tasksDesc) << ":" << std::endl;
			solution_file << "\tbucket with " << b.size() << " stages and cost "
				<< calc_stage_proc(b, expanded_args, ref->second->tasksDesc) << ":" << std::endl;
			total_tasks += calc_stage_proc(b, expanded_args, ref->second->tasksDesc);
			for (PipelineComponentBase* s : b) {
				std::cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
				// solution_file << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << std::endl;
			}
		}
		solution_file.close();

		// write some statistics abou the solution
		ofstream statistics_file;
		statistics_file.open(dakota_filename + "-b" + std::to_string(size_limit) + "merging_statistics.log", ios::trunc);

		statistics_file << current_stages.size() << "\t";
		statistics_file << current_stages.size()*ref->second->tasksDesc.size() << "\t";
		statistics_file << total_tasks << "\t";
		statistics_file << solution.size() << "\t";
		statistics_file << total_tasks/solution.size() << "\t";

		statistics_file.close();

		// merge all stages in each bucket, given that they are mergable
		for (std::list<PipelineComponentBase*> bucket : solution) {
			// std::cout << "bucket merging" << std::endl;
			std::list<PipelineComponentBase*> curr = merge_stages_full(bucket, expanded_args, ref->second->tasksDesc);
			// send rem stages to merged_stages
			for (PipelineComponentBase* s : curr) {
				// std::cout << "\tadding stage " << s->getId() << std::endl;
				merged_stages[s->getId()] = s;
			}
		}
	}
}
