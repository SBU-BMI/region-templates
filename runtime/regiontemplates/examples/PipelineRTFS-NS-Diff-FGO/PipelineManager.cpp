#include <iostream>
#include <map>

using namespace std;

// global uid for any local(this file) entity
int uid=1;
int new_uid() {return uid++;};

int main() {

	// Handler to the distributed execution system environment
	// SysEnv sysEnv;

	// // Tell the system which libraries should be used
	// sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");


	FILE* workflow_descriptor = fopen("seg_example.t2flow", "r");

	//------------------------------------------------------------
	// Parse pipeline file
	//------------------------------------------------------------

	// get all workflow inputs without their values, returning also the parameters values on another map
	{map<int, ArgumentBase> workflow_inputs, map<int, parameter_values_t> parameters_values} = get_inputs_from_file(workflow_descriptor);

	// get all workflow outputs
	map<int, ArgumentBase> workflow_outputs = get_outputs_from_file(workflow_descriptor);

	// get all stages, also setting the uid from this context to Task (i.e Task::setId())
	map<int, RTPipelineComponentBase> base_stages = get_stages_from_file(workflow_descriptor);

	// connect the stages inputs/outputs and, returning the list of arguments used
	// the stages dependencies are also set here (i.e Task::addDependency())
	map<int, ArgumentBase> interstage_arguments = connect_stages_from_file(base_stages, workflow_descriptor);

	//------------------------------------------------------------
	// Add create all combinarions of parameters
	//------------------------------------------------------------

	// expand parameter combinations returning a list of parameters sets
	// each set contains exactly one value for every input parameter
	list<map<int, parameter_value_t>> expanded_parameters = expand_parameters_combinations(parameters_values);

	// generate inputs, outputs and stages copies with updated uids and correct workflow_id's
	list<map<int, ArgumentBase>> all_inputs = expand_inputs(workflow_inputs, expanded_parameters);
	list<map<int, ArgumentBase>> all_outputs = expand_outputs(workflow_outputs);
	// OBS: the tasks ids should also be updated (i.e Task::setId())
	list<map<int, RTPipelineComponentBase>> all_stages = expand_stages(base_stages);
	list<map<int, ArgumentBase>> all_interstage_arguments = expand_arguments(interstage_arguments);

	//------------------------------------------------------------
	// Iterative merging of stages
	//------------------------------------------------------------

	// maybe try to merge the inputs, and then merge stages iteratively
	// again, updating the uids (Task and local id)
	{map<int, RTPipelineComponentBase> merged_stages,
		map<int, ArgumentBase> merged_outputs,
		map<int, ArgumentBase> merged_arguments} = iterative_full_merging(all_inputs, all_outputs, all_stages);

	//------------------------------------------------------------
	// Add workflows to Manager to be executed
	//------------------------------------------------------------

	// add arguments to each stage
	list<RTPipelineComponentBase> ready_stages = add_arguments_to_stages(merged_stages, merged_arguments);

	// add all stages to manager
	for each RTPipelineComponentBase s of ready_stages do
		sysEnv.executeComponent(s);
	end

	// execute workflows
	sysEnv.startupExecution();

	// get results
	for each ArgumentBase output of merged_outputs do
		cout << sysEnv.getComponentResultData(output.getId()) << endl;
	end

}

