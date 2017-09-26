#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <cfloat>
#include <algorithm>

#include <omp.h>

#include "SysEnv.h"
#include "Argument.h"
#include "PipelineComponentBase.h"
#include "RTPipelineComponentBase.h"
#include "RegionTemplateCollection.h"
#include "ReusableTask.hpp"

#include "parsing.hpp"

#include "fg_reuse/merging.hpp"
#include "fg_reuse/fgm.hpp"

#include "debug_funs.hpp"

using namespace std;

// Workflow generation functions
void generate_drs(RegionTemplate* rt, const map<int, ArgumentBase*> &expanded_args);
void generate_drs(RegionTemplate* rt, PipelineComponentBase* stage,
	const std::map<int, ArgumentBase*> &expanded_args);
void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, 
	map<int, ArgumentBase*> &merged_arguments, string name="tile");
void generate_pre_defined_stages(FILE* parameters_values_file, map<int, ArgumentBase*> args, 
	map<int, PipelineComponentBase*> base_stages, map<int, ArgumentBase*>& workflow_outputs, 
	map<int, ArgumentBase*>& expanded_args, map<int, PipelineComponentBase*>& expanded_stages,
	bool use_coarse_grain=true, bool clustered_generation=false);

int main(int argc, char* argv[]) {

	// verify arguments
	if (argc > 1 && string(argv[1]).compare("-h") == 0) {
		cout << "usage: ./PipelineRTFS-NS-Diff-FGO -b <max bucket size> -dkt <dakota output file> -ma <merging algorithm> [-f] [-ncg]" << endl;
		cout << "   -f   - shuffle" << endl;
		cout << "   -ncg - don't do coarse grain merging" << endl;
		cout << "   -b   - for -ma 5, represents the number of buckets generated" << endl;
		cout << "avaiable algorithms:" << endl;
		cout << "0 - No fine grain merging algorithm" << endl;
		cout << "1 - naive fine grain merging algorithm" << endl;
		cout << "2 - Smart recursive cut fine grain merging algorithm" << endl;
		cout << "3 - Reuse-Tree fine grain merging algorithm" << endl;
		cout << "4 - Double-prunning reuse-tree fine grain merging algorithm" << endl;
		cout << "5 - Task-Balanced Reuse-tree fine grain merging algorithm" << endl;
		return 0;
	}
	if (argc < 7) {
		cout << "usage: ./PipelineRTFS-NS-Diff-FGO -b <max bucket size> -dkt <dakota output file> -ma <merging algorithm> [-f] [-ncg]" << endl;
		return 0;
	}

	int mpi_rank;
	int mpi_size;
	int mpi_val;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	if (mpi_rank == mpi_size-1) {
		// get arguments
		int max_bucket_size;
		if (find_arg_pos("-b", argc, argv) == -1) {
			cout << "Missing max bucket size." << endl;
			return 0;
		} else
			max_bucket_size = atoi(argv[find_arg_pos("-b", argc, argv)+1]);

		string dakota_file;
		if (find_arg_pos("-dkt", argc, argv) == -1) {
			cout << "Missing dakota file path." << endl;
			return 0;
		} else
			dakota_file = argv[find_arg_pos("-dkt", argc, argv)+1];

		int merging_algorithm;
		if (find_arg_pos("-ma", argc, argv) == -1) {
			cout << "Missing merging algorithm option." << endl;
			return 0;
		} else
			merging_algorithm = atoi(argv[find_arg_pos("-ma", argc, argv)+1]);

		bool shuffle = find_arg_pos("-f", argc, argv) == -1 ? false : true;
		cout << "shuffle: " << shuffle << endl;

		bool use_coarse_grain = find_arg_pos("-ncg", argc, argv) == -1 ? true : false;

		// workflow file
		FILE* workflow_descriptor = fopen("seg_example.t2flow", "r");

		//------------------------------------------------------------
		// Parse pipeline file
		//------------------------------------------------------------

		// get all workflow inputs without their values, returning also the parameters 
		// values (i.e list<ArgumentBase> values) on another map
		map<int, ArgumentBase*> workflow_inputs;
		map<int, list<ArgumentBase*>> parameters_values;
		get_inputs_from_file(workflow_descriptor, workflow_inputs, parameters_values);

		// get all workflow outputs
		map<int, ArgumentBase*> workflow_outputs;
		get_outputs_from_file(workflow_descriptor, workflow_outputs);

		// get all stages, also setting the uid from this context to Task (i.e Task::setId())
		// also returns the list of arguments used
		// the stages dependencies are also set here (i.e Task::addDependency())
		map<int, PipelineComponentBase*> base_stages;
		map<int, ArgumentBase*> interstage_arguments;
		get_stages_from_file(workflow_descriptor, base_stages, interstage_arguments);

		// this map is a dependency structure: stage -> dependency_list
		map<int, list<int>> deps;
		
		// connect the stages inputs/outputs 
		connect_stages_from_file(workflow_descriptor, base_stages, 
			interstage_arguments, workflow_inputs, deps, workflow_outputs);
		map<int, ArgumentBase*> all_argument(workflow_inputs);
		for (pair<int, ArgumentBase*> a : interstage_arguments)
			all_argument[a.first] = a.second;

		//------------------------------------------------------------
		// Iterative merging of stages
		//------------------------------------------------------------

		map<int, ArgumentBase*> args;
		for (pair<int, ArgumentBase*> p : workflow_inputs)
			args[p.first] = p.second;
		for (pair<int, ArgumentBase*> p : interstage_arguments)
			args[p.first] = p.second;

		map<int, ArgumentBase*> expanded_args;
		map<int, PipelineComponentBase*> expanded_stages;

		// expand_stages(args, parameters_values, expanded_args, 
		// 	base_stages, expanded_stages, workflow_outputs);
		FILE* parameters_values_file = fopen(dakota_file.c_str(), "r");
		generate_pre_defined_stages(parameters_values_file, args, base_stages, workflow_outputs, 
			expanded_args, expanded_stages, use_coarse_grain);

		// mapprint(expanded_stages, expanded_args);
		// mapprint(expanded_args);

		// add arguments to each stage
		add_arguments_to_stages(expanded_stages, expanded_args);

		map<int, PipelineComponentBase*> merged_stages;

		// mesure merging exec time
		struct timeval start, end;
		gettimeofday(&start, NULL);

		fgm::merge_stages_fine_grain(merging_algorithm, expanded_stages, base_stages, merged_stages, 
			expanded_args, max_bucket_size, shuffle, dakota_file);

		gettimeofday(&end, NULL);

		long merge_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		ofstream merge_time_f(dakota_file + "-b" + to_string(max_bucket_size) + "merge_time.log", ios::app);
		merge_time_f << merge_time << "\t";
		merge_time_f.close();

		// resolve dependencies of reused stages
		for (pair<int, PipelineComponentBase*> p : merged_stages) {
			// add correct stage dependencies 
			list<int> deps_tmp;
			for (int i=0; i<p.second->getNumberDependencies(); i++) {
				
				// if the i-th dependency of p still exists and is reused, add the dependency of the 
				//   reused stage to p
				if (merged_stages.find(p.second->getDependency(i)) != merged_stages.end() && 
						merged_stages[p.second->getDependency(i)]->reused != NULL) {

					deps_tmp.emplace_back(merged_stages[p.second->getDependency(i)]->reused->getId());
				}
			}
			for (int d : deps_tmp)
				p.second->addDependency(d);

			// connect correct output arguments
			bool updated = true;
			// cout << "Updating arguments of " << p.second->getId() << ":" << p.second->getName() << endl;
			while (updated) {
				updated = false;
				for (ArgumentBase* a : p.second->getArguments()) {
					if (a->getParent() != 0 && merged_stages[a->getParent()]->reused != NULL) {
						// cout << "Replacing argument " << endl;
						updated = true;
						p.second->replaceArgument(a->getId(), 
							merged_stages[a->getParent()]->reused->getArgumentByName(a->getName()));
						break;
					}
				}
			}
		}

		//------------------------------------------------------------
		// Add workflows to Manager to be executed
		//------------------------------------------------------------

		string inputFolderPath = "~/Desktop/images15";

		for (int i=0; i<mpi_size-1; i++)
			MPI_Send(&mpi_val, 1, MPI_INT, i, 99, MPI_COMM_WORLD);

		// Tell the system which libraries should be used
		sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");

		// add all stages to manager
		// cout << endl << "executeComponent" << endl;
		for (pair<int, PipelineComponentBase*> s : merged_stages) {
			if (s.second->reused == NULL) {

				// generate the data regions for each stage
				generate_drs(((RTPipelineComponentBase*)s.second)->
					getRegionTemplateInstance("tile"), s.second, expanded_args);

				cout << "sent component " << s.second->getId() << ":" 
					<< s.second->getName() << " sized " << s.second->size() << " to execute with args:" << endl;
				cout << "\tall args: " << endl;
				for (ArgumentBase* a : s.second->getArguments())
					cout << "\t\t" << a->getId() << ":" << a->getName() << " = " 
						<< a->toString() << " parent " << a->getParent() << endl;
				cout << "\tinputs: " << endl;
				for (int i : s.second->getInputs())
					cout << "\t\t" << i << ":" << expanded_args[i]->getName() << " = " 
						<< expanded_args[i]->toString() << " parent " << expanded_args[i]->getParent() << endl;
				cout << "\toutputs: " << endl;
				for (int i : s.second->getOutputs())
					cout << "\t\t" << i << ":" << expanded_args[i]->getName() << " = " 
						<< expanded_args[i]->toString() << endl;
				cout << "\tdependencies:" << endl;
				for (int i=0; i<s.second->getNumberDependencies(); i++)
					cout << "\t\t" << merged_stages[s.second->getDependency(i)]->getId() << ":" 
						<< merged_stages[s.second->getDependency(i)]->getName() << endl;

				// set task ID to enable dependency enforcement
				((Task*)s.second)->setId(s.second->getId());

				// workaround to make sure that the RTs, if any, won't leak on this part of the algorithm
				s.second->setLocation(PipelineComponentBase::MANAGER_SIDE);
				sysEnv.executeComponent(s.second);
			}
		}

		// execute workflows
		cout << endl << "startupExecution" << endl;
		gettimeofday(&start, NULL);

		sysEnv.startupExecution();

		gettimeofday(&end, NULL);

		long run_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		ofstream run_time_f(dakota_file + "-b" + to_string(max_bucket_size) + "run_time.log", ios::app);
		run_time_f << run_time << "\t";
		run_time_f.close();

		// get results
		cout << endl << "Results: " << endl;
		for (pair<int, ArgumentBase*> output : workflow_outputs) {
			// cout << "\t" << output.second->getName() << ":" << output.second->getId() << " = " << 
			// 	sysEnv.getComponentResultData(output.second->getId()) << endl;
			char *resultData = sysEnv.getComponentResultData(output.second->getId());
	        std::cout << "Diff Id: " << output.second->getId() << " resultData -  ";
			if(resultData != NULL){
	            std::cout << "size: " << ((int *) resultData)[0] << " Diff: " << ((float *) resultData)[1] <<
	            " Secondary Metric: " << ((float *) resultData)[2] << std::endl;
			}else{
				std::cout << "NULL" << std::endl;
			}
		}
	} else {
		int flag;
		do {
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
				&flag, MPI_STATUS_IGNORE);
			usleep(200000);
		} while (!flag);
		MPI_Recv(&mpi_val, 1, MPI_INT, MPI_ANY_SOURCE, 
			MPI_ANY_TAG, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
		sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");
	}

	sysEnv.finalizeSystem();

}

/***************************************************************/
/**************** Workflow generation functions ****************/
/***************************************************************/


void generate_drs(RegionTemplate* rt, 
	const std::map<int, ArgumentBase*> &expanded_args) {

	// verify every argument
	for (pair<int, ArgumentBase*> p : expanded_args) {
		// if an argument is a region template data region
		if (p.second->getType() == ArgumentBase::RT) {
			if (((ArgumentRT*)p.second)->isFileInput) {
				// create the data region and add it to the input region template
				DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
				ddr2d->setName(p.second->getName());
				std::ostringstream oss;
				oss << p.first;
				ddr2d->setId(oss.str());
				ddr2d->setVersion(p.first);
				ddr2d->setIsAppInput(true);
				ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
				ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
				ddr2d->setInputFileName(p.second->toString());
				rt->insertDataRegion(ddr2d);
			}
		}
	}
}

void generate_drs(RegionTemplate* rt, PipelineComponentBase* stage,
	const std::map<int, ArgumentBase*> &expanded_args) {

	// verify every argument
	for (int inp : stage->getInputs()) {
		// if an argument is a region template data region
		if (expanded_args.at(inp)->getType() == ArgumentBase::RT) {
			// if (((ArgumentRT*)expanded_args.at(inp))->isFileInput) {
				// create the data region and add it to the input region template
				DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
				ddr2d->setName(expanded_args.at(inp)->getName());
				std::ostringstream oss;
				oss << inp;
				ddr2d->setId(oss.str());
				ddr2d->setVersion(inp);
				ddr2d->setIsAppInput(true);
				ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
				ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
				ddr2d->setInputFileName(expanded_args.at(inp)->toString());
				rt->insertDataRegion(ddr2d);
			// }
		}
	}
}

void add_arguments_to_stages(std::map<int, PipelineComponentBase*> &merged_stages, 
	std::map<int, ArgumentBase*> &merged_arguments, string name) {

	int i=0;
	for (pair<int, PipelineComponentBase*>&& stage : merged_stages) {
		// create the RT isntance
		RegionTemplate *rt = new RegionTemplate();
		rt->setName(name);

		// add input arguments to stage, adding them as RT as needed
		for (int arg_id : stage.second->getInputs()) {
			ArgumentBase* new_arg = merged_arguments[arg_id]->clone();
			new_arg->setParent(merged_arguments[arg_id]->getParent());
			stage.second->addArgument(new_arg);
			if (new_arg->getType() == ArgumentBase::RT) {
				// std::cout << "input RT : " << merged_arguments[arg_id]->getName() << std::endl;
				// insert the region template on the parent stage if the argument is a DR and if the RT wasn't already added
				if (((RTPipelineComponentBase*)stage.second)->
						getRegionTemplateInstance(rt->getName()) == NULL) {
					((RTPipelineComponentBase*)stage.second)->addRegionTemplateInstance(rt, rt->getName());
				}
				((RTPipelineComponentBase*)stage.second)->addInputOutputDataRegion(
					rt->getName(), new_arg->getName(), RTPipelineComponentBase::INPUT);
			}
			if (merged_arguments[arg_id]->getParent() != 0) {
				// verify if the dependency stage was reused
				int parent = merged_arguments[arg_id]->getParent();
				// std::cout << "[before]Dependency: " << stage.second->getId() << ":" << stage.second->getName()
				// 	<< " ->addDependency( " << parent << " )" << std::endl;
				if (merged_stages[merged_arguments[arg_id]->getParent()]->reused != NULL)
					parent = merged_stages[merged_arguments[arg_id]->getParent()]->reused->getId();
				// std::cout << "Dependency: " << stage.second->getId() << ":" << stage.second->getName()
				// 	<< " ->addDependency( " << parent << " )" << std::endl;
				((RTPipelineComponentBase*)stage.second)->addDependency(parent);
			}
		}

		// add output arguments to stage, adding them as RT as needed
		for (int arg_id : stage.second->getOutputs()) {
			ArgumentBase* new_arg = merged_arguments[arg_id]->clone();
			new_arg->setParent(merged_arguments[arg_id]->getParent());
			new_arg->setIo(ArgumentBase::output);
			stage.second->addArgument(new_arg);
			if (merged_arguments[arg_id]->getType() == ArgumentBase::RT) {
				// std::cout << "output RT : " << merged_arguments[arg_id]->getName() << std::endl;
				// insert the region template on the parent stage if the argument is a DR and if the RT wasn't already added
				if (((RTPipelineComponentBase*)stage.second)->getRegionTemplateInstance(rt->getName()) == NULL)
					((RTPipelineComponentBase*)stage.second)->addRegionTemplateInstance(rt, rt->getName());
				((RTPipelineComponentBase*)stage.second)->addInputOutputDataRegion(rt->getName(), 
					merged_arguments[arg_id]->getName(), RTPipelineComponentBase::OUTPUT);
			}
		}
	}
}

/***************************************************************/
/********* Workflow merging and preparation functions **********/
/***************************************************************/

double str2d (string s) {
	istringstream os(s);
	double d;
	os >> d;
	return d;
}

ArgumentBase* gen_arg(string value, string type) {
	ArgumentBase* arg;
	char* token2;
	switch (get_port_type(type)) {
		case parsing::int_t:
			arg = new ArgumentInt((int)str2d(value));
			break;
		case parsing::string_t:
			arg = new ArgumentString(value);
			break;
		case parsing::float_t:
			arg = new ArgumentFloat((float)str2d(value));
			break;
		case parsing::float_array_t:
			arg = new ArgumentFloatArray();
			token2 = strtok(const_cast<char*>(value.c_str()), "[],");
			while (token2 != NULL) {
				// convert value
				ArgumentFloat f((float)str2d(token2));
				((ArgumentFloatArray*)arg)->addArgValue(f);
				token2 = strtok(NULL, "[],");
			}
			break;
		case parsing::rt_t:
			arg = new ArgumentRT(value);
			break;
		default:
			exit(-4);
	}
	return arg;
}

bool all_inps_in(const list<int>& inps, const map<int, ArgumentBase*>& args, 
	const map<string, list<ArgumentBase*>>& input_arguments, 
	const list<ArgumentBase*>& args_values) {

	for (int i : inps) {
		// cout << "checking arg " << args.at(i)->getId() << ":" << args.at(i)->getName() << endl;
		if (input_arguments.find(args.at(i)->getName()) == input_arguments.end() &&
			find_argument(args_values, args.at(i)->getName()) == NULL) {
			// cout << "not here" << endl;
			return false;
		}
	}
	return true;
}

bool all_inps_in(list<int> inps, map<int, list<ArgumentBase*>> ref) {
	for (int i : inps) {
		if (ref.find(i) == ref.end())
			return false;
	}
	return true;
}

void generate_pre_defined_stages(FILE* parameters_values_file, map<int, ArgumentBase*> args, 
	map<int, PipelineComponentBase*> base_stages, map<int, ArgumentBase*>& workflow_outputs, 
	map<int, ArgumentBase*>& expanded_args, map<int, PipelineComponentBase*>& expanded_stages,
	bool use_coarse_grain, bool clustered_generation) {

	cout << "[generate_pre_defined_stages]" << endl;

	string pe("Parameters for evaluation");

	char* line = NULL;
	size_t length=0;

	// map of arguments, ordered by name, containing all values of each argument
	map<string, list<ArgumentBase*>> input_arguments;

	// list of the stages' arguments
	map<int, list<ArgumentBase*>> stages_arguments;

	// interstage args
	list<ArgumentBase*> args_values;

	// list of stages created here
	list<PipelineComponentBase*> stages_iterative;

	while(getline(&line, &length, parameters_values_file) != -1) {
		// go to the first pe
		int r = 0;
		while (string(line).find(pe) == string::npos && r != -1)
			r = getline(&line, &length, parameters_values_file);

		// finishes loop when the eof is reached
		if (r == -1)
			break;

		list<ArgumentBase*> current_args;

		// get all parameters of a workflow
		while (true) {
			getline(&line, &length, parameters_values_file);

			// get parameter name, value and token
			char* token = strtok(line, " \t");
			string value(token);

			// break loop if the line is empty with only a '\n'
			if (value.compare("\n")==0)
				break;

			token = strtok(NULL, " \t");
			string name(token);
			token = strtok(NULL, " \t\n");
			string type(token);

			ArgumentBase* arg;

			// verify if the argument is on the map and if it contains the current value
			bool found = false;
			for (ArgumentBase* a : input_arguments[name]) {
				ArgumentBase* aa = gen_arg(value, type);

				// cout << "[---] comparing " << a->getName() << ":" << a->toString() << " with " << name << ":" << aa->toString() << endl;
				if (a->getName().compare(name)==0 && a->toString().compare(aa->toString())==0) {
					found = true;
					arg = a;
					delete aa;
					break;
				}
				delete aa;
			}

			char* token2;
			if (!found) {
				arg = gen_arg(value, type);

				// set RT arguments as file inputs
				if (arg->getType() == ArgumentBase::RT)
					((ArgumentRT*)arg)->isFileInput = true;

				// set remaining atributes
				arg->setName(name);
				arg->setId(new_uid());

				input_arguments[name].emplace_back(arg);
			}

			current_args.emplace_back(arg);
		}

		stages_arguments[new_uid()] = current_args;
	}

	// show all generated args
	// for (pair<string, list<ArgumentBase*>> p : input_arguments) {
	// 	cout << "Argument " << p.first << " with values:" << endl;
	// 	for (ArgumentBase* a : p.second) {
	// 		cout << "\t" << a->getId() << ": " << a->toString() << endl;
	// 	}
	// }

	// for (pair<int, list<ArgumentBase*>> p : stages_arguments) {
	// 	cout << "Argument set " << p.first << " with parameters:" << endl;
	// 	for (ArgumentBase* a : p.second) {
	// 		cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
	// 	}
	// }


	// keep expanding stages until there is no stage left
	while (base_stages.size() != 0) {
		// cout << "base_stages size: " << base_stages.size() << endl;
		for (pair<int, PipelineComponentBase*> p : base_stages) {
			// attempt to find a stage witch has all inputs expanded either on the workflow inputs or interstage ones
			// cout << "checking stage " << p.second->getName() << endl;
			if (all_inps_in(p.second->getInputs(), args, input_arguments, args_values)) {
				// cout << "stage " << p.second->getName() << " has all inputs" << endl;

				// A list of concatenated arg values, used as a quick way to verify if a stage with
				// the same args was already created.
				map<string, PipelineComponentBase*> arg_values_list;

				// expands all input values of stage p
				for (pair<int, list<ArgumentBase*>> as : stages_arguments) {
					PipelineComponentBase* tmp = p.second->clone();
					string arg_values = "";
					// add all arguments from stages_arguments that belong to stage p.second
					for (int inp_id : p.second->getInputs()) {
						// cout << "checking input " << args[inp_id]->getName() << " of " << p.second->getName() << " with " << as.second.size() << " parameters" << endl;
						for (ArgumentBase* a : as.second) {
							// cout << "checking arg " << a->getName() << ":" << a->getId() << endl;
							if (args.at(inp_id)->getName().compare(a->getName())==0) {
								arg_values += to_string(a->getId());
								tmp->addInput(a->getId());
								// tmp->addArgument(a->clone());
								// cout << "added arg " << a->getName() << ":" << a->getId() << " = " << a->toString() << endl;
								break;
							}
						}
					}

					// cout << "[arg_values] " << arg_values << endl;
					map<string, PipelineComponentBase*>::iterator it = arg_values_list.find(arg_values);
					// verify if there is no other stage with the same values
					if (it == arg_values_list.end() || !use_coarse_grain) {
						// add current stage and args values to be compared later
						arg_values_list[arg_values] = tmp;

						// finishes to generate the stage
						int id = new_uid();
						tmp->setId(id);
						tmp->setName(p.second->getName());
						tmp->setLocation(PipelineComponentBase::WORKER_SIDE);

						// generate outputs
						for (int out_id : p.second->getOutputs()) {
							int new_id = new_uid();
							ArgumentBase* ab_cpy = args.at(out_id)->clone();
							ab_cpy->setName(args.at(out_id)->getName());
							ab_cpy->setId(new_id);
							ab_cpy->setParent(tmp->getId());
							tmp->replaceOutput(out_id, new_id);						
							
							// add stage's output arguments to current workflow's argument list
							stages_arguments[as.first].emplace_back(ab_cpy);

							// add output to interstage args map
							args_values.emplace_back(ab_cpy);
						}
						
						// add stage to final stages list
						expanded_stages[tmp->getId()] = tmp;
					} else {
						// if the stage already exists, reuse it
						for (int out_id : it->second->getOutputs()) {
							// add reused stage's output arguments to current workflow's argument list
							// cout << "reusing stage " << out_id << " from workflow "
							// 	<< it->second->getId() << endl;
							stages_arguments[as.first].emplace_back(find_argument(args_values, out_id));
						}

						// TODO: solve mem leaking
						// delete tmp;
					}
				}

				// remove stage descriptor since it was already solved and break the loop
				base_stages.erase(p.first);
				break;
			}
		}
	}

	// add stages_arguments inputs to expanded_args
	for (pair<int, list<ArgumentBase*>> p : stages_arguments) {
		for (ArgumentBase* a : p.second) {
			expanded_args[a->getId()] = a;
		}
	}

	// update the output arguments
	map<int, ArgumentBase*> workflow_outputs_cpy = workflow_outputs;
	while (workflow_outputs_cpy.size() != 0) {
		// get the first output argument
		ArgumentBase* old_arg = (workflow_outputs_cpy.begin())->second;

		// remove the current, outdated, argument from final map
		workflow_outputs.erase(old_arg->getId());
		workflow_outputs_cpy.erase(old_arg->getId());

		// add a copy of the old arg with the correct id to the final map for each repeated output
		// cout << "checking output " << old_arg->getId() << ":" << old_arg->getName() << endl;
		for (ArgumentBase* a : args_values) {
			// cout << "comparing with output " << a->getId() << ":" << a->getName() << endl;
			if (a->getName().compare(old_arg->getName()) == 0) {
				// cout << "output found" << endl;
				ArgumentBase* temp = old_arg->clone();
				temp->setParent(old_arg->getParent());
				temp->setId(a->getId());
				workflow_outputs[temp->getId()] = temp;
			}
		}
	}
}


// map<int, ArgumentBase*> args: all args, i.e inputs and interstate arguments.
// map<int, list<ArgumentBase*>> args_values: the list of values for each argument. 
// 		this map will be changed inside and must start with all inputs
// map<int, ArgumentBase*> expanded_args: output of function. map with all args to be used on execution.
// map<int, PipelineComponentBase*> stages: map of all stages with arguments mapped to args
// map<int, PipelineComponentBase*> expanded_stages: output of function. Returns all stages
// 		ready for execution.
void expand_stages(const map<int, ArgumentBase*> &args, 
	map<int, list<ArgumentBase*>> args_values, 
	map<int, ArgumentBase*> &expanded_args,
	map<int, PipelineComponentBase*> stages,
	map<int, PipelineComponentBase*> &expanded_stages,
	map<int, ArgumentBase*> &workflow_outputs) {

	// cout << endl << "args:" << endl;
	// mapprint(args);
	// cout << endl;

	// // cout << endl << "arg_values:" << endl;
	// for (pair<int, list<ArgumentBase*>> p : args_values) {
	// 	// cout << "base argument " << p.first << ":" << args.at(p.first)->getName() << endl;
	// 	for (ArgumentBase* a : p.second) {
	// 		a->setId(new_uid());
	// 		a->setName(args.at(p.first)->getName());
	// 		// cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
	// 	}
	// }

	// cout << endl << "stages:" << endl;
	// mapprint(stages);
	// cout << endl;

	// keep expanding stages until there is no stage left
	while (stages.size() != 0) {
		// cout << "stages size: " << stages.size() << endl;
		for (pair<int, PipelineComponentBase*> p : stages) {
			// attempt to find a stage witch has all inputs expanded
			if (all_inps_in(p.second->getInputs(), args_values)) {
				// cout << "stage " << p.second->getName() << " has all inputs" << endl;
				// create temporary list of stages to be expanded
				list<PipelineComponentBase*> stages_iterative;
				// starts the stages temp list with the current stage without the valued arguments
				stages_iterative.emplace_back(p.second);

				// cout << "expanding stage " << p.second->getName() << " with inputs:" << endl;
				// for (int inp_id : p.second->getInputs())
				// 	cout << "\t" << args.at(inp_id)->getName() << endl;

				// expands all inputs from stage p
				for (int inp_id : p.second->getInputs()) {
					// cout << "expanding input " << args.at(inp_id)->getName() << " with values:" << endl;
					// for (ArgumentBase* a : args_values[inp_id])
					// 	cout << "\t" << a->toString() << endl;
					// expands the values of each inptut in p->getInputs()
					list<PipelineComponentBase*> stages_iterative_temp;
					for (ArgumentBase* a : args_values[inp_id]) {
						// cout << "expanding value " << a->toString() << endl;
						// cout << "stages_iterative: " << endl;
						// for (PipelineComponentBase* pp : stages_iterative)
						// 	cout << "\t" << pp->getId() << ":" << pp->getName() << endl;
						// generate a copy of the current stage for each input-value pair
						for (PipelineComponentBase* pt : stages_iterative) {
							// cout << "updating stage " << pt->getId() << ":" << pt->getName() << endl;
							// clone pt basic infoinfoinfo
							PipelineComponentBase* pt_cpy = pt->clone();
							pt_cpy->setLocation(PipelineComponentBase::WORKER_SIDE);
		
							// set name and id
							pt_cpy->setName(pt->getName());
							pt_cpy->setId(new_uid());

							// copy input list
							for (int inp : pt->getInputs())
								pt_cpy->addInput(inp);

							// replace stock input with current
							pt_cpy->replaceInput(inp_id, a->getId());

							// cout << "all outputs from stage " << pt->getName() << endl;
							// for (int out : pt->getOutputs())
							// 	cout << "\t" << out << endl;

							// add copy to stages_iterative_temp
							stages_iterative_temp.emplace_back(pt_cpy);

							// cout << endl << "expanded_stages after" << endl;
							// mapprint(expanded_stages);
							// cout << endl;
							// cout << endl << "expanded_args after" << endl;
							// mapprint(expanded_args);
							// cout << endl;
						}
					}
					// replace stages_iterative with expanded version
					// TODO: fix leaking
					stages_iterative = stages_iterative_temp;
				}

				// generate all output copies from the current stage
				for (int out_id : p.second->getOutputs()) {
					list<ArgumentBase*> temp;
					for (PipelineComponentBase* pt : stages_iterative) {
						int new_id = new_uid();
						ArgumentBase* ab_cpy = args.at(out_id)->clone();
						ab_cpy->setName(args.at(out_id)->getName());
						ab_cpy->setId(new_id);
						ab_cpy->setParent(pt->getId());
						pt->replaceOutput(out_id, new_id);
						temp.emplace_back(ab_cpy);
					}
					args_values[out_id] = temp;
				}

				// add all stages
				for (PipelineComponentBase* pt : stages_iterative)
					expanded_stages[pt->getId()] = pt;

				// remove the stage from 'stages' since it was fully expanded
				stages.erase(p.first);

				// cout << endl << "arg_values:" << endl;
				// for (pair<int, list<ArgumentBase*>> p : args_values) {
				// 	cout << "base argument " << p.first << ":" << args.at(p.first)->getName() << endl;
				// 	for (ArgumentBase* a : p.second) {
				// 		cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
				// 	}
				// }

				// cout << endl << "stages:" << endl;
				// mapprint(stages);
				// cout << endl;

				// break loop of 'stages' since its content has changed
				break;
			} 
			// else
				// cout << "stage " << p.second->getName() << " have unmet dependencies " << endl;
		}
	}

	// flatten the arg values into expanded_args
	// cout << endl << "arg_values" << endl;
	for (pair<int, list<ArgumentBase*>> p : args_values) {
		// cout << "base argument " << p.first << ":" << args.at(p.first)->getName() << endl;
		for (ArgumentBase* a : p.second) {
			// cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
			expanded_args[a->getId()] = a;
		}
	}

	// update the output arguments
	map<int, ArgumentBase*> workflow_outputs_cpy = workflow_outputs;
	while (workflow_outputs_cpy.size() != 0) {
		// get the first output argument
		ArgumentBase* old_arg = (workflow_outputs_cpy.begin())->second;

		// get the list of parameters (i.e the number of copies and the final ids) of the outputs
		list<ArgumentBase*> l = args_values[old_arg->getId()];

		// remove the current, outdated, argument from final map
		workflow_outputs.erase(old_arg->getId());
		workflow_outputs_cpy.erase(old_arg->getId());

		// add a copy of the old arg with the correct id to the final map for each repeated output
		for (ArgumentBase* a : l) {
			ArgumentBase* temp = old_arg->clone();
			temp->setParent(old_arg->getParent());
			temp->setId(a->getId());
			workflow_outputs[temp->getId()] = temp;
		}
	}
}

