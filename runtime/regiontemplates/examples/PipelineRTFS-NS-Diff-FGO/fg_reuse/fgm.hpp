#ifndef FGM_HPP_
#define FGM_HPP_

#include <map>
#include <list>
#include <iostream>
#include <fstream>

#include "PipelineComponentBase.h"
#include "RTPipelineComponentBase.h"

#include "merging.hpp"
#include "cutting_algorithms.hpp"
#include "graph/reuse_tree.hpp"

namespace fgm {

void merge_stages_fine_grain(int merging_algorithm, const std::map<int, PipelineComponentBase*> &all_stages, 
	const std::map<int, PipelineComponentBase*> &stages_ref, std::map<int, PipelineComponentBase*> &merged_stages, 
	std::map<int, ArgumentBase*> expanded_args, int max_bucket_size, int n_nodes, bool shuffle, string dakota_filename);

}

#endif