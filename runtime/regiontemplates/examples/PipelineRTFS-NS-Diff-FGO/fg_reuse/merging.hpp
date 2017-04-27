#ifndef MERGING_HPP
#define MERGING_HPP

#include <vector>

#include "Argument.h"
#include "PipelineComponentBase.h"

// global uid for any local(this file and PipelineManager) entity
static int uid=1;
int new_uid();

ArgumentBase* find_argument(PipelineComponentBase* p, string name, map<int, ArgumentBase*> expanded_args);

// This function assumes that the merged PCB has at least, but not limited to one ReusableTask on tasks
// of type task_name. Also, to_merge must have exactly one ReusableTask of type task_name. These
// conditions are not checked.
bool exists_reusable_task(const PipelineComponentBase* merged, const PipelineComponentBase* to_merge, string task_name);

// for the meantime the mearging will happen whenever at least the first task is reusable
bool merging_condition(const PipelineComponentBase* merged, const PipelineComponentBase* to_merge, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

// filters all the stages from an input map by the stage's name
void filter_stages(const map<int, PipelineComponentBase*> &all_stages, 
	string stage_name, list<PipelineComponentBase*> &filtered_stages, bool shuffle);

list<ReusableTask*> task_generator(map<string, list<ArgumentBase*>> &tasks_desc, PipelineComponentBase* p, 
	map<int, ArgumentBase*> expanded_args);

ReusableTask* find_task(list<ReusableTask*> l, string name);

list<ReusableTask*> find_tasks(list<ReusableTask*> l, string name);

void merge_stages(PipelineComponentBase* current, PipelineComponentBase* s, map<string, list<ArgumentBase*>> ref);

// Attempt to merge a list of PCB, returning a list of PCBs with the same size.
// The new list will have the merged PCBs without any tasks and with the reuse
// atribute set as true.
list<PipelineComponentBase*> merge_stages_full(list<PipelineComponentBase*> stages, 
	map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref);

int get_reuse_factor(PipelineComponentBase* s1, PipelineComponentBase* s2, 
	const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

float calc_stage_proc(list<PipelineComponentBase*> s, const map<int, ArgumentBase*> &args, const map<string, list<ArgumentBase*>>& ref);

// just add PCB s symbolicaly and calc the cost with stages
float calc_stage_proc(list<PipelineComponentBase*> stages, PipelineComponentBase* s, map<int, ArgumentBase*> &args, 
	map<string, list<ArgumentBase*>> ref);

float calc_stage_mem(list<PipelineComponentBase*> s, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref);

#endif
