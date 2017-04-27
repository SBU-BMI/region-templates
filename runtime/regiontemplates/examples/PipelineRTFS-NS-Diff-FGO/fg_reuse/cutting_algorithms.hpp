#ifndef CUTTING_ALGORITHMS_HPP
#define CUTTING_ALGORITHMS_HPP

#include <map>
#include <list>

#include "PipelineComponentBase.h"

#include "merging.hpp"
#include "graph/min_cut.hpp"

std::list<std::list<PipelineComponentBase*>> recursive_cut(std::list<PipelineComponentBase*> rem, const std::map<int, PipelineComponentBase*> &all_stages, 
	int max_bucket_size, int max_cuts, std::map<int, ArgumentBase*> &args, std::map<string, std::list<ArgumentBase*>> ref);

#endif