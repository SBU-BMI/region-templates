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
#include <regex>
#include <cfloat>
#include <algorithm>

#include <omp.h>

#include "json/json.h"

#include "SysEnv.h"
#include "Argument.h"
#include "PipelineComponentBase.h"
#include "RTPipelineComponentBase.h"
#include "RegionTemplateCollection.h"
#include "ReusableTask.hpp"
#include "merging.hpp"
#include "graph/min_cut.hpp"
#include "graph/reuse_tree.hpp"

using namespace std;

namespace parsing {
enum port_type_t {
	int_t, string_t, float_t, float_array_t, rt_t, error
};
}

// general xml field structure
typedef struct {
	string type;
	string data;
} general_field_t;

void mapprint(map<int, ArgumentBase*> mapp) {
	for (pair<int, ArgumentBase*> p : mapp)
		cout << p.first << ":" << p.second->getName() << endl;
}

void mapprint(map<int, list<ArgumentBase*>> mapp) {
	for (pair<int, list<ArgumentBase*>> p : mapp) {
	// for (map<int, ArgumentBase*>::iterator i=map.begin(); i!=map.end();i++)
		cout << p.first << ":" << endl;
		for (ArgumentBase* a : p.second)
			cout << "\t" << a->getName() << ":" << a->toString() << endl;
	}
}

void mapprint(map<int, PipelineComponentBase*> mapp) {
	for (map<int, PipelineComponentBase*>::iterator i=mapp.begin(); i!=mapp.end();i++)
		cout << i->first << ":" << i->second->getName() << endl;
}

void listprint(list<PipelineComponentBase*> mapp) {
	for (list<PipelineComponentBase*>::iterator i=mapp.begin(); i!=mapp.end();i++)
		cout << (*i)->getId() << ":" << (*i)->getName() << endl;
}

void adj_mat_print(mincut::weight_t** adjMat, size_t n) {
	for (int i=0; i<n; i++) {
		for (int j=0; j<n; j++) {
			cout << adjMat[i][j] << "\t";
		}
		cout << endl;
	}
}

void adj_mat_print(mincut::weight_t** adjMat, map<size_t, int> id2task, size_t n) {
	cout << "x\t";
	for (int i=0; i<n; i++)
		cout << id2task[i] << "\t";
	cout << endl;
	for (int i=0; i<n; i++) {
		cout << id2task[i] << "\t";
		for (int j=0; j<n; j++) {
			cout << adjMat[i][j] << "\t";
		}
		cout << endl;
	}
}

// Workflow parsing functions
void get_inputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_inputs, 
	map<int, list<ArgumentBase*>> &parameters_values);
void get_outputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_outputs);
void get_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments);
void connect_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments, map<int, ArgumentBase*> &input_arguments,
	map<int, list<int>> &deps, map<int, ArgumentBase*> &workflow_outputs);
void expand_stages(const map<int, ArgumentBase*> &args, map<int, list<ArgumentBase*>> args_values, 
	map<int, ArgumentBase*> &expanded_args,map<int, PipelineComponentBase*> stages,
	map<int, PipelineComponentBase*> &expanded_stages, map<int, ArgumentBase*> &workflow_outputs);
void merge_stages_fine_grain(const map<int, PipelineComponentBase*> &all_stages, 
	const map<int, PipelineComponentBase*> &stages_ref, map<int, PipelineComponentBase*> &merged_stages, 
	RegionTemplate* rt, map<int, ArgumentBase*> expanded_args, int max_bucket_size);
void generate_drs(RegionTemplate* rt, const map<int, ArgumentBase*> &expanded_args);
void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, 
	map<int, ArgumentBase*> &merged_arguments,
	RegionTemplate *rt);
void generate_pre_defined_stages(FILE* parameters_values_file, map<int, ArgumentBase*> args, 
	map<int, PipelineComponentBase*> base_stages, map<int, ArgumentBase*>& workflow_outputs, 
	map<int, ArgumentBase*>& expanded_args, map<int, PipelineComponentBase*>& expanded_stages);

// Workflow parsing helper functions
list<string> line_buffer;
int get_line(char** line, FILE* f);
string get_workflow_name(FILE* workflow);
string get_workflow_field(FILE* workflow, string field);
void get_workflow_arguments(FILE* workflow, list<ArgumentBase*> &output_arguments);
vector<general_field_t> get_all_fields(FILE* workflow, string start, string end);
PipelineComponentBase* find_stage(map<int, PipelineComponentBase*> stages, string name);
int find_stage_id(map<int, PipelineComponentBase*> stages, string name);
ArgumentBase* find_argument(const map<int, ArgumentBase*>& arguments, string name);
ArgumentBase* find_argument(const list<ArgumentBase*>& arguments, int id);
ArgumentBase* find_argument(const list<ArgumentBase*>& arguments, string name);
ArgumentBase* new_typed_arg_base(string type);
parsing::port_type_t get_port_type(string s);

int main(int argc, char* argv[]) {

	int mpi_rank;
	int mpi_size;
	int mpi_val;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	if (mpi_rank == mpi_size-1) {
		// region template used by all stages
		RegionTemplate *rt = new RegionTemplate();
		rt->setName("tile");

		// get arguments
		if (argc != 3) {
			cout << "usage: ./PipelineRTFS-NS-Diff-FGO <max_bucket_size> <DAKOTA_OUTPUT_FILE>" << endl;
			return 0;
		}

		int max_bucket_size = atoi(argv[1]);
		string dakota_file = argv[2];

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

		// cout << "workflow_inputs:" << endl;
		// for (pair<int, ArgumentBase*> p : workflow_inputs)
		// 	cout << p.first << ":" << p.second->getName() << endl;

		// cout << endl << "parameters_values:" << endl;
		// for (pair<int, list<ArgumentBase*>> p : parameters_values) {
		// 	cout << "argument " << p.first << ":" << workflow_inputs[p.first]->getName() << ":" << endl;
		// 	for (ArgumentBase* a : p.second)
		// 		cout << "\t" << a->toString() << endl;
		// }

		// get all workflow outputs
		map<int, ArgumentBase*> workflow_outputs;
		get_outputs_from_file(workflow_descriptor, workflow_outputs);
		// cout << endl << "workflow_outputs " << endl;
		// mapprint(workflow_outputs);
		// for (pair<int, ArgumentBase*> p : workflow_outputs)
		// 	cout << p.second->getId() << ":" << p.second->getName() << endl;

		// get all stages, also setting the uid from this context to Task (i.e Task::setId())
		// also returns the list of arguments used
		// the stages dependencies are also set here (i.e Task::addDependency())
		map<int, PipelineComponentBase*> base_stages;
		map<int, ArgumentBase*> interstage_arguments;
		get_stages_from_file(workflow_descriptor, base_stages, interstage_arguments);
		// cout << endl << "base_stages:" << endl;
		// for (pair<int, PipelineComponentBase*> p : base_stages) {
		// 	cout << p.first << ":" << p.second->getName() << endl;
		// 	cout << "\toutputs: " << p.second->getOutputs().size() << endl << endl;
		// 	for (int i : p.second->getOutputs())
		// 		cout << "\t\t" << i << ":" << interstage_arguments[i]->getName() << endl;
		// 	cout << "\t task descriptors:" << endl;
		// 	for (pair<string, list<ArgumentBase*>> d : p.second->tasksDesc) {
		// 		cout << "\t\ttask: " << d.first << endl;
		// 		for (ArgumentBase* a : d.second)
		// 			cout << "\t\t\t" << a->getName() << endl;
		// 	}
		// }


		// cout << endl << "interstage_arguments:" << endl;
		// for (pair<int, ArgumentBase*> p : interstage_arguments)
		// 	cout << p.first << ":" << p.second->getName() << endl;

		// this map is a dependency structure: stage -> dependency_list
		map<int, list<int>> deps;
		
		// connect the stages inputs/outputs 
		connect_stages_from_file(workflow_descriptor, base_stages, 
			interstage_arguments, workflow_inputs, deps, workflow_outputs);
		map<int, ArgumentBase*> all_argument(workflow_inputs);
		for (pair<int, ArgumentBase*> a : interstage_arguments)
			all_argument[a.first] = a.second;

		// mapprint(all_argument);

		// cout << endl << "all_arguments:" << endl;
		// for (pair<int, ArgumentBase*> p : all_argument) {
		// 	cout << p.second->getId() << ":" << p.second->getName() 
		// 		<< " parent " << p.second->getParent() << endl;
		// }

		// cout << endl << "connected base_stages:" << endl;
		// for (pair<int, PipelineComponentBase*> p : base_stages) {
		// 	cout << p.first << ":" << p.second->getName() << endl;
		// 	cout << "\tinputs: " << p.second->getInputs().size() << endl;
		// 	for (int i : p.second->getInputs())
		// 		cout << "\t\t" << i << ":" << all_argument[i]->getName() << endl;
		// 	cout << "\toutputs: " << p.second->getOutputs().size() << endl;
		// 	for (int i : p.second->getOutputs())
		// 		cout << "\t\t" << i << ":" << all_argument[i]->getName() << endl;
		// }

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
			expanded_args, expanded_stages);

		cout << endl<< "merged: " << endl;
		for (pair<int, PipelineComponentBase*> p : expanded_stages) {
			cout << "stage " << p.second->getId() << ":" << p.second->getName() << endl;
			cout << "\tinputs: " << endl;
			for (int i : p.second->getInputs())
				cout << "\t\t" << i << ":" << expanded_args[i]->getName() << " = " 
					<< expanded_args[i]->toString() << endl;
			cout << "\toutputs: " << endl;
			for (int i : p.second->getOutputs())
				cout << "\t\t" << i << ":" << expanded_args[i]->getName() << endl;
		}

		// cout << endl << "merged args" << endl;
		// for (pair<int, ArgumentBase*> p : expanded_args)
		// 	cout << "\t" << p.first << ":" << p.second->getName() << " = " 
		// 		<< p.second->toString() << " sized: " << p.second->size() << endl;

		// add arguments to each stage
		cout << endl << "add_arguments_to_stages" << endl;
		add_arguments_to_stages(expanded_stages, expanded_args, rt);

		map<int, PipelineComponentBase*> merged_stages;

		// mesure merging exec time
		struct timeval start, end;
		gettimeofday(&start, NULL);

		merge_stages_fine_grain(expanded_stages, base_stages, merged_stages, rt, expanded_args, max_bucket_size);

		gettimeofday(&end, NULL);

		long merge_time = ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec));
		ofstream exec_time("exec_time_b" + to_string(max_bucket_size), ios::app);
		exec_time << "merge time: " << merge_time << endl;

		cout << endl<< "merged-fine before deps resolution: " << endl;
		for (pair<int, PipelineComponentBase*> p : merged_stages) {
			string s = p.second->reused!=NULL?" - reused":"";
			cout << "stage " << p.second->getId() << ":" << p.second->getName() << s << endl;
			cout << "\ttasks: " << endl;
			for (ReusableTask* t : p.second->tasks) {
				cout << "\t\t" << t->getId() << ":" << t->getTaskName() << endl;
				cout << "\t\t\tparent_task: " << t->parentTask << endl;
			}
		}

		// resolve dependencies of reused stages
		for (pair<int, PipelineComponentBase*> p : merged_stages) {
			// add correct stage dependencies 
			list<int> deps_tmp;
			for (int i=0; i<p.second->getNumberDependencies(); i++) {
				if (merged_stages.find(p.second->getDependency(i)) != merged_stages.end() && 
						merged_stages[p.second->getDependency(i)]->reused != NULL) {
					deps_tmp.emplace_back(merged_stages[p.second->getDependency(i)]->reused->getId());
				}
			}
			for (int d : deps_tmp)
				p.second->addDependency(d);

			// connect correct output arguments
			bool updated = true;
			while (updated) {
				updated = false;
				for (ArgumentBase* a : p.second->getArguments()) {
					if (a->getParent() != 0 && merged_stages[a->getParent()]->reused != NULL) {
						updated = true;
						p.second->replaceArgument(a->getId(), 
							merged_stages[a->getParent()]->reused->getArgumentByName(a->getName()));
						break;
					}
				}
			}

			// replace arguments with clones to avoid double free when stages are finished
			// map<int, ArgumentBase*> args_clones;
			// for (ArgumentBase* a : p.second->getArguments()) {
			// 	ArgumentBase* cpy = a->clone();
			// 	cpy->setId(new_uid());
			// 	args_clones[a->getId()] = cpy;
			// }
			// for (pair<int, ArgumentBase*> a : args_clones) 
			// 	p.second->replaceArgument(a.first, a.second);
		}

		ofstream solution_file;
		solution_file.open("fine-grain-merging-solution-b" + to_string(max_bucket_size), ios::app);
		cout << endl<< "merged-fine: " << endl;
		solution_file << endl<< "merged-fine: " << endl;
		for (pair<int, PipelineComponentBase*> p : merged_stages) {
			string s = p.second->reused!=NULL ? " - reused" : (" with " + to_string(p.second->tasks.size()) + " tasks");
			cout << "stage " << p.second->getId() << ":" << p.second->getName() << s << endl;
			solution_file << "stage " << p.second->getId() << ":" << p.second->getName() << s << endl;
			cout << "\ttasks: " << endl;
			for (ReusableTask* t : p.second->tasks) {
				cout << "\t\t" << t->getId() << ":" << t->getTaskName() << endl;
				cout << "\t\t" << t->getId() << ":" << t->getTaskName() << endl;
			}
		}
		solution_file.close();

		//------------------------------------------------------------
		// Add workflows to Manager to be executed
		//------------------------------------------------------------

		string inputFolderPath = "~/Desktop/images15";
		cout << endl << "generate_drs" << endl;
		generate_drs(rt, expanded_args);

		for (int i=0; i<mpi_size-1; i++)
			MPI_Send(&mpi_val, 1, MPI_INT, i, 99, MPI_COMM_WORLD);

		// Tell the system which libraries should be used
		sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");

		// add all stages to manager
		cout << endl << "executeComponent" << endl;
		for (pair<int, PipelineComponentBase*> s : merged_stages) {
			if (s.second->reused == NULL) {
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
		exec_time << "run time: " << run_time << endl << endl;
		exec_time.close();

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
/***************** Workflow parsing functions ******************/
/***************************************************************/

// returns by reference a map of input arguments to an uid from 'workflow_descriptor' with the
// list if all possible values each argument can get on a separate map, linked with the same uid
void get_inputs_from_file(FILE* workflow_descriptor, 
	map<int, ArgumentBase*> &workflow_inputs, 
	map<int, list<ArgumentBase*>> &parameters_values) {

	char *line = NULL;
	size_t len = 0;

	// initial ports section beginning and end
	string ip("<inputPorts>");
	string ipe("</inputPorts>");
	
	// ports section beginning and end
	string p("<port>");
	string pe("</port>");

	// argument name, stored before to be consumed when the ArgumentBase type can be set
	string name;

	// go to the initial ports beginning
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ip) == string::npos);
	// cout << "port init begin: " << line << endl;

	// keep getting ports until it reaches the end of initial ports
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ipe) == string::npos) {
		// consumes the port beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "port begin: " << line << endl;

		// finds the name field
		name = get_workflow_name(workflow_descriptor);
		// cout << "name: " << name << endl;

		// finds the description field
		string description = get_workflow_field(workflow_descriptor, "text");
		// cout << "description: " << description << endl;

		// parse the description to get the input value(s)
		Json::Reader reader;
		bool wellFormed;
		Json::Value data;

		wellFormed = reader.parse(description, data, false);
		if(!wellFormed) {
			cout << "Failed to parse JSON: " << description << endl << reader.getFormattedErrorMessages() << endl;
			exit(-3);
		}

		// create the propper Argument object for each type case also getting
		// the parameters' values
		ArgumentBase* inp_arg;
		list<ArgumentBase*> inp_values;
		switch (get_port_type(data["type"].asString())) {
			case parsing::int_t:
				// create the argument
				inp_arg = new ArgumentInt();
				// cout << "int argument: " << name << ", values: " << endl;		
				// get all possible values for the argument
				for (int i=0; i<data["values"].size(); i++) {
					ArgumentBase* val = new ArgumentInt(data["values"][i].asInt());
					// cout << ((ArgumentInt*)val)->toString() << endl;
					inp_values.emplace_back(val);
				}
				// cout << endl;
				break;
			case parsing::string_t:
				// create the argument
				inp_arg = new ArgumentString();
				// cout << "string argument: " << name << ", values: " << endl;		
				// get all possible values for the argument
				for (int i=0; i<data["values"].size(); i++) {
					ArgumentBase* val = new ArgumentString(data["values"][i].asString());
					// cout << ((ArgumentString*)val)->toString() << endl;
					inp_values.emplace_back(val);
				}
				// cout << endl;
				break;
			case parsing::float_t:
				// create the argument
				inp_arg = new ArgumentFloat();
				// cout << "float argument: " << name << ", values: " << endl;		
				// get all possible values for the argument
				for (int i=0; i<data["values"].size(); i++) {
					ArgumentBase* val = new ArgumentFloat(data["values"][i].asFloat());
					// cout << ((ArgumentFloat*)val)->toString() << endl;
					inp_values.emplace_back(val);
				}
				// cout << endl;
				break;
			case parsing::float_array_t:
				// create the argument
				inp_arg = new ArgumentFloatArray();
				// cout << "floatarray argument: " << name << ", values: " << endl;		
				// get all possible values for the argument
				for (int i=0; i<data["values"].size(); i++) {
					ArgumentBase* val = new ArgumentFloatArray();
					// cout << "[";
					for (int j=0; j<data["values"][i].size(); j++) {
						ArgumentFloat temp(data["values"][i][j].asFloat());
						((ArgumentFloatArray*)val)->addArgValue(temp);
						// cout << temp.toString() << endl;
					}
					// cout << "]";
					inp_values.emplace_back(val);
				}
				// cout << endl;
				break;
			case parsing::rt_t:
				// create the argument
				inp_arg = new ArgumentRT();
				// cout << "string argument: " << name << ", values: " << endl;		
				// get all possible values for the argument
				for (int i=0; i<data["values"].size(); i++) {
					ArgumentBase* val = new ArgumentRT(data["values"][i].asString());
					((ArgumentRT*)val)->isFileInput = true;
					// cout << ((ArgumentString*)val)->toString() << endl;
					inp_values.emplace_back(val);
				}
				// cout << endl;
				break;
			default:
				exit(-4);
		}

		// set inp_arg name, id and input type
		inp_arg->setName(name);
		int arg_id = new_uid();
		inp_arg->setId(arg_id);
		inp_arg->setIo(ArgumentBase::input);

		// add input argument to map
		workflow_inputs[arg_id] = inp_arg;

		// add list of argument values to map
		parameters_values[arg_id] = inp_values;

		// consumes the port ending
		while (get_line(&line, workflow_descriptor) != -1 && string(line).find(pe) == string::npos);
			// cout << "not port end: " << line << endl;
		// cout << "port end: " << line << endl;
	}
}

// returns by reference a map of output arguments, mapped by an uid, 'workflow_outputs'
void get_outputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_outputs) {

	char *line = NULL;
	size_t len = 0;

	// initial ports section beginning and end
	string ip("<outputPorts>");
	string ipe("</outputPorts>");
	
	// ports section beginning and end
	string p("<port>");
	string pe("</port>");

	// go to the initial ports beginning
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ip) == string::npos);
	// cout << "port init begin: " << line << endl;

	// keep getting ports until it reaches the end of initial ports
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ipe) == string::npos) {
		// consumes the port beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "port begin: " << line << endl;

		// finds the name field
		string name = get_workflow_name(workflow_descriptor);
		// cout << "name: " << name << endl;

		// finds the description field
		string description = get_workflow_field(workflow_descriptor, "text");
		// cout << "description: " << description << endl;

		// parse the description to get the input value(s)
		Json::Reader reader;
		bool wellFormed;
		Json::Value data;

		wellFormed = reader.parse(description, data, false);
		if(!wellFormed) {
			cout << "Failed to parse JSON: " << description << endl << reader.getFormattedErrorMessages() << endl;
			exit(-3);
		}

		// create the propper Argument object for each type case also getting
		// the parameters' values
		ArgumentBase* out_arg;
		switch (get_port_type(data["type"].asString())) {
			case parsing::int_t:
				// create the argument
				out_arg = new ArgumentInt();
				// cout << "int output: " << name << endl;		
				break;
			case parsing::string_t:
				// create the argument
				out_arg = new ArgumentString();
				// cout << "string output: " << name << endl;		
				break;
			case parsing::float_t:
				// create the argument
				out_arg = new ArgumentFloat();
				// cout << "float output: " << name << endl;		
				break;
			case parsing::float_array_t:
				// create the argument
				out_arg = new ArgumentFloatArray();
				// cout << "floatarray output: " << name << endl;		
				break;
			case parsing::rt_t:
				// create the argument
				out_arg = new ArgumentRT();
				// cout << "string output: " << name << endl;		
				break;
			default:
				exit(-4);
		}
		
		out_arg->setName(name);
		out_arg->setIo(ArgumentBase::output);
		int new_id = new_uid();
		out_arg->setId(new_id);
		workflow_outputs[new_id] = out_arg;

		// consumes the port ending
		while (get_line(&line, workflow_descriptor) != -1 && string(line).find(pe) == string::npos);
			// get_line << "not port end: " << line << endl;
		// cout << "port end: " << line << endl;
	}
	// cout << "port init end: " << line << endl;
}

// returns by reference the map of stages on its uid, 'base_stages', and also a map
// of all output arguments of all stages on its uid, 'interstage_arguments'.
void get_stages_from_file(FILE* workflow_descriptor, 
	map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments) {

	char *line = NULL;
	size_t len = 0;

	string ps("<processors>");
	string pse("</processors>");

	string p("<processor>");
	string pe("</processor>");

	// go to the processors beginning
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ps) == string::npos);
		// cout << "not processor init begin: " << line << endl;
	 // cout << "processor init begin: " << line << endl;

	// keep getting single processors until it reaches the end of all processors
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(pse) == string::npos) {
		// consumes the processor beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "processor begin: " << line << endl;

		// get stage fields
		string name = get_workflow_name(workflow_descriptor);
		// cout << "name: " << name << endl;

		// get stage command
		string command = get_workflow_field(workflow_descriptor, "command");
		// cout << "command: " << command << endl;

		PipelineComponentBase* stage = PipelineComponentBase::ComponentFactory::getComponentFactory(command)();
		stage->setName(name);

		// workaround to make sure that the RTs, if any, won't leak on the mearging part of the algorithm
		stage->setLocation(PipelineComponentBase::WORKER_SIDE);

		// get outputs and add them to the map of arguments
		// list<string> inputs = get_workflow_ports(workflow_descriptor, "inputPorts");
		// cout << "outputs:" << endl;
		list<ArgumentBase*> outputs;
		get_workflow_arguments(workflow_descriptor, outputs);
		for(list<ArgumentBase*>::iterator i=outputs.begin(); i!=outputs.end(); i++) {
			// cout << "\t" << (*i)->getId() << ":" << (*i)->getName() << endl;
			interstage_arguments[(*i)->getId()] = *i;
			stage->addOutput((*i)->getId());
		}

		int stg_id = new_uid();
		// cout << "uid: " << stg_id << endl;
		// setting task id
		stage->setId(stg_id);
		base_stages[stg_id] = stage;

		// consumes the processor ending
		while (get_line(&line, workflow_descriptor) != -1 && string(line).find(pe) == string::npos);
			// cout << "not processor end: " << line << endl;
		// cout << "processor end: " << line << endl;
	}
}

// returns by reference the map of stages to its ids updated. each stage now has
// the list of input ids
void connect_stages_from_file(FILE* workflow_descriptor, 
	map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments,
	map<int, ArgumentBase*> &input_arguments,
	map<int, list<int>> &deps,
	map<int, ArgumentBase*> &workflow_outputs) {

	char *line = NULL;
	size_t len = 0;

	string ds("<datalinks>");
	string dse("</datalinks>");

	string d("<datalink>");
	string de("</datalink>");

	string sink("<sink type=");
	string sinke("</sink>");

	string source("<source type=");
	string sourcee("</source>");

	// go to the datalinks beginning
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(ds) == string::npos);
		// cout << "not datalink init begin" << line << endl;
	// cout << "datalink init begin" << line << endl;

	// keep getting single datalinks until it reaches the end of all datalinks
	while (get_line(&line, workflow_descriptor) != -1 && string(line).find(dse) == string::npos) {
		// consumes the datalink beginning
		while (string(line).find(d) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "datalink begin" << line << endl;

		// get sink and source fields
		vector<general_field_t> all_sink_fields = get_all_fields(workflow_descriptor, sink, sinke);
		vector<general_field_t> all_source_fields = get_all_fields(workflow_descriptor, source, sourcee);

		// verify if it's a task sink instead of a workflow sink
		if (all_sink_fields.size() != 1) {
			// get the sink stage
			PipelineComponentBase* sink_stg = find_stage(base_stages, all_sink_fields[0].data);
			// cout << "stage " << sink_stg->getId() << ":" << sink_stg->getName() << " sink" << endl;

			ArgumentBase* arg;
			// check whether the source is from the workflow arguments or another stage
			if (all_source_fields.size() == 1) {
				// if source is workflow argument:
				arg = find_argument(input_arguments, all_source_fields[0].data);
				// cout << "source from workflow is " << arg->getId() << ":" << arg->getName() << endl;
			} else {
				// if the source is another stage
				deps[sink_stg->getId()].emplace_back(find_stage(base_stages, all_source_fields[0].data)->getId());
				arg = find_argument(interstage_arguments, all_source_fields[1].data);
				arg->setParent(find_stage(base_stages, all_source_fields[0].data)->getId());
				// cout << "source from stage " << all_source_fields[0].data << " is " << arg->getId() << ":" 
				// 	<< arg->getName() << " parent " << arg->getParent() << endl;
			}

			// add the link to the sink stage
			sink_stg->addInput(arg->getId());
		} 
		else {
			// cout << "workflow argument sink: " << all_sink_fields[0].data << endl;
			// update workflow output id in order to access it later to retreive the output
			
			ArgumentBase* itstg_argument = find_argument(interstage_arguments, all_source_fields[1].data);
			// cout << "Output " << all_sink_fields[0].data << " connects to argument " << itstg_argument->getId() << 
			// 	":" << itstg_argument->getName() << endl;
			ArgumentBase* output = find_argument(workflow_outputs, all_sink_fields[0].data);
			// cout << "Output " << all_sink_fields[0].data << " had id " << output->getId() << endl;
			
			// remove reference of old id from map
			workflow_outputs.erase(output->getId());

			// update the output id
			output->setId(itstg_argument->getId());
			// cout << "Output " << all_sink_fields[0].data << " now has id " << output->getId() << endl;
			
			// re-insert the output with the new id
			workflow_outputs[output->getId()] = output;
		}

		// consumes the datalink ending
		while (string(line).find(de) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "datalink end" << line << endl;
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

void generate_pre_defined_stages(FILE* parameters_values_file, map<int, ArgumentBase*> args, 
	map<int, PipelineComponentBase*> base_stages, map<int, ArgumentBase*>& workflow_outputs, 
	map<int, ArgumentBase*>& expanded_args, map<int, PipelineComponentBase*>& expanded_stages) {

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
					break;
				}
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
	for (pair<string, list<ArgumentBase*>> p : input_arguments) {
		cout << "Argument " << p.first << " with values:" << endl;
		for (ArgumentBase* a : p.second) {
			cout << "\t" << a->getId() << ": " << a->toString() << endl;
		}
	}

	// for (list<ArgumentBase*> s : stages_arguments) {
	// 	cout << "got stage:" << endl;
	// 	for (ArgumentBase* a : s) {
	// 		cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
	// 	}
	// }

	// keep expanding stages until there is no stage left
	while (base_stages.size() != 0) {
		// cout << "base_stages size: " << base_stages.size() << endl;
		for (pair<int, PipelineComponentBase*> p : base_stages) {
			// attempt to find a stage witch has all inputs expanded either on the workflow inputs or interstage ones
			cout << "checking stage " << p.second->getName() << endl;
			if (all_inps_in(p.second->getInputs(), args, input_arguments, args_values)) {
				cout << "stage " << p.second->getName() << " has all inputs" << endl;

				// A list of concatenated arg values, used as a quick way to verify if a stage with
				// the same args was already created.
				map<string, PipelineComponentBase*> arg_values_list;

				// expands all input values of stage p
				for (pair<int, list<ArgumentBase*>> as : stages_arguments) {
					PipelineComponentBase* tmp = p.second->clone();
					string arg_values = "";
					// add all arguments from stages_arguments that belong to stage p.second
					for (int inp_id : p.second->getInputs()) {
						// cout << "checking input " << args[inp_id]->getName() << " of " << p.second->getName() << endl;
						for (ArgumentBase* a : as.second) {
							if (args.at(inp_id)->getName().compare(a->getName())==0) {
								arg_values += to_string(a->getId());
								tmp->addInput(a->getId());
								// tmp->addArgument(a->clone());
								break;
							}
						}
					}
					map<string, PipelineComponentBase*>::iterator it = arg_values_list.find(arg_values);
					// verify if there is no other stage with the same values
					if (it == arg_values_list.end()) {
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
		cout << "checking output " << old_arg->getId() << ":" << old_arg->getName() << endl;
		for (ArgumentBase* a : args_values) {
			cout << "comparing with output " << a->getId() << ":" << a->getName() << endl;
			if (a->getName().compare(old_arg->getName()) == 0) {
				cout << "output found" << endl;
				ArgumentBase* temp = old_arg->clone();
				temp->setParent(old_arg->getParent());
				temp->setId(a->getId());
				workflow_outputs[temp->getId()] = temp;
			}
		}
	}
}

bool all_inps_in(list<int> inps, map<int, list<ArgumentBase*>> ref) {
	for (int i : inps) {
		if (ref.find(i) == ref.end())
			return false;
	}
	return true;
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

mincut::weight_t get_reuse_factor(mincut::subgraph_t s1, mincut::subgraph_t s2, map<size_t, int> id2task,  
	map<int, PipelineComponentBase*> current_stages, map<int, ArgumentBase*> &args,
	map<string, list<ArgumentBase*>> ref) {

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

bool cutting_condition(list<PipelineComponentBase*> current, float nrS_mksp, float max_mem, 
	map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	if (calc_stage_proc(current, args, ref) > nrS_mksp || calc_stage_mem(current, args, ref) > max_mem)
		return true;
	else
		return false;
}

pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> get_cut(list<PipelineComponentBase*> current_stages, 
	const map<int, PipelineComponentBase*> &all_stages, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	// generate the reuse matrix and the map real-task to min-cut id
	size_t id = 0;
	size_t n = current_stages.size();

	// dynamic allocation of adjMat is needed because if n is waaay too big the stack will overflow
	mincut::weight_t** adjMat = new mincut::weight_t *[n];
	for (size_t i=0; i<n; i++) {
		adjMat[i] = new mincut::weight_t [n];
	}
	
	map<size_t, int> id2task;
	for (list<PipelineComponentBase*>::iterator s1=current_stages.begin(); s1!= current_stages.end(); s1++, id++) {
		id2task[id] = (*s1)->getId();
		// cout << "map: " << id << ":" << (*s1)->getId() << endl;
		size_t id_j = id;
		for (list<PipelineComponentBase*>::iterator s2=s1; s2!= current_stages.end(); s2++, id_j++) {
			if (id == id_j)
				adjMat[id][id] = 0;
			else {
				adjMat[id][id_j] = get_reuse_factor(*s1, *s2, args, ref);
				adjMat[id_j][id] = adjMat[id][id_j];
			}
		}
	}

	// cout << endl;
	// adj_mat_print(adjMat, id2task, n);
	// cout << endl;

	// send adjMat to mincut algorithm
	list<mincut::cut_t> cuts = mincut::min_cut(n, adjMat);

	// get the cut with the minimal weight, using the number of merged tasks as a tiebreaker
	mincut::cut_t best_cut = cuts.front();
	mincut::weight_t best_weight = mincut::_cut_w(best_cut);
	int best_num_tasks = get_reuse_factor(mincut::_cut_s1(best_cut), mincut::_cut_s2(best_cut), 
		id2task, all_stages, args, ref);
	mincut::weight_t r;

	for (mincut::cut_t c : cuts) {
		// cout << "cut: " << mincut::_cut_w(c) << ":" << endl;
		// cout << "\tS1:" << endl;
		// for (mincut::_id_t id : mincut::_cut_s1(c))
		// 	cout << "\t\t" << id2task[id] << endl;
		// cout << "\tS2:" << endl;
		// for (mincut::_id_t id : mincut::_cut_s2(c))
		// 	cout << "\t\t" << id2task[id] << endl;
		// cout << endl;

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

	// convert cut to PCB lists for return
	pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> best_cut_pcb;
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

list<list<PipelineComponentBase*>> recursive_cut(list<PipelineComponentBase*> rem, const map<int, PipelineComponentBase*> &all_stages, 
	int n, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	list<list<PipelineComponentBase*>> last_solution;

	// finishes if there is no more need for a cut (i.e n=1) or if we can't make any more cuts (i.e size=1)
	if (n == 1 || rem.size() == 1) {
		last_solution.emplace_back(rem);
		// cout << "retuned n=" << n << ", size=" << rem.size() << endl;
		return last_solution;
	}

	float last_mksp = FLT_MAX;
	float current_mksp = FLT_MAX/10;
	list<PipelineComponentBase*> rest;
	list<list<PipelineComponentBase*>> buckets;

	// execute while there is improvement
	while(current_mksp < last_mksp) {
		last_mksp = current_mksp;

		// make a copy of this solution in case the next isn't better
		last_solution = buckets;

		if (rem.size() == 1) {
			// cout << "retuned state for size=1, n=" << n << ":" << endl;
			// for (list<PipelineComponentBase*> b : last_solution) {
			// 	cout << "\tbucket with cost " << calc_stage_proc(b, ref) << ":" << endl;
			// 	for (PipelineComponentBase* s : b) {
			// 		cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
			// 	}
			// }
			return last_solution;
		}

		// cout << "rem:" << endl;
		// for (PipelineComponentBase* s : rem) {
		// 	cout << "\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
		// }
		// cout << "rest:" << endl;
		// for (PipelineComponentBase* s : rest) {
		// 	cout << "\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
		// }

		// cut the current bucket
		pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		// cout << "cut: " << endl;
		// cout << "\tc1" << endl;
		// for (PipelineComponentBase* s : c1)
		// 	cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
		// cout << "\tc2" << endl;
		// for (PipelineComponentBase* s : c2)
		// 	cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;

		// join c2 with the remaining of rem
		c2.insert(c2.begin(), rest.begin(), rest.end());

		buckets = recursive_cut(c2, all_stages, n-1, args, ref);
		buckets.emplace_back(c1);
		
		// recalculate current max makespan and set rem backup on last_rem
		float mksp;
		current_mksp = 0;
		for (list<PipelineComponentBase*> b : buckets) {
			mksp = calc_stage_proc(b, args, ref);
			if (mksp > current_mksp)
				current_mksp = mksp;
		}

		for (int i=0; i<n-2; i++)
			cout << "\t";
		cout << "mksp: " << current_mksp << endl;

		// cout << "merged state n=" << n << ":" << endl;
		// for (list<PipelineComponentBase*> b : buckets) {
		// 	cout << "\tbucket with cost " << calc_stage_proc(b, args, ref) << ":" << endl;
		// 	for (PipelineComponentBase* s : b) {
		// 		cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
		// 	}
		// }

		// set next iteration PCB lists
		rem = c1;
		rest = c2;
	}

	return last_solution;
}

list<list<PipelineComponentBase*>> montecarlo_cut(list<PipelineComponentBase*> rem, const map<int, PipelineComponentBase*> &all_stages, 
	int n, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	list<list<PipelineComponentBase*>> ret;

	cout << "[montecarlo_cut] n=" << n << endl;

	if (n == 1 || rem.size() == 1) {
		ret.emplace_back(rem);
		cout << "[montecarlo_cut] n=" << n << endl;
		return ret;
	}

	// perform cuts until there is no more tasks to cut, or, there is no more need for another cut
	for (int i=0; i<n-1; i++) {
		pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		rem = c1;
		ret.emplace_back(c2);

		if (rem.size() == 1) {
			break;
		}
	}

	// add last bucket 
	ret.emplace_back(rem);

	// cout << "[montecarlo_cut] n=" << n << "ret:" << endl;
	// int i=0;
	// for (list<PipelineComponentBase*> bucket : ret) {
	// 	cout << "\tb:" << i++ << endl;
	// 	for (PipelineComponentBase* s : bucket)
	// 		cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
	// }

	return ret;
}

void montecarlo_dep(int n, list<PipelineComponentBase*> c1, list<PipelineComponentBase*> c2,
	const map<int, PipelineComponentBase*> &all_stages, map<int, ArgumentBase*> &args,
	map<string, list<ArgumentBase*>> ref, float &best_mksp, float &best_var,
	pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> &best_cut) {

	cout << "[montecarlo_recursive_cut] n=" << n << " running cut" << endl;
	cout << "\tc1 - cost=" << calc_stage_proc(c1, args, ref) << ":" << endl;
	for (PipelineComponentBase* s : c1)
		cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
	cout << "\tc2 - cost=" << calc_stage_proc(c2, args, ref) << ":" << endl;
	for (PipelineComponentBase* s : c2)
		cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
	
	list<list<PipelineComponentBase*>> c = montecarlo_cut(c2, all_stages, n-1, args, ref);
	
	// calculates i cut cost with rest c
	cout << "[montecarlo_cut] n=" << n << "ret:" << endl;
	int ii=0;
	// mksp selection metric
	float mksp;
	float max_mksp = calc_stage_proc(c1, args, ref);
	float avg = calc_stage_proc(c1, args, ref);
	for (list<PipelineComponentBase*> bucket : c) {
		mksp = calc_stage_proc(bucket, args, ref);
		avg += mksp;
		if (mksp > max_mksp)
			max_mksp = mksp;
		cout << "\tb" << ii++ << " - mksp=" << mksp<< endl;
		for (PipelineComponentBase* s : bucket)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
	}

	// standard deviation selection metric
	avg /= (c.size()+1);
	float var = (calc_stage_proc(c1, args, ref)-avg)*(calc_stage_proc(c1, args, ref)-avg);
	for (list<PipelineComponentBase*> bucket : c) {
		mksp = calc_stage_proc(bucket, args, ref);
		var += (mksp-avg)*(mksp-avg);
	}
	var /= (c.size()+1);

	cout << "with mksp=" << max_mksp << endl;
	cout << "with var=" << var << endl;

	// // maybe double if?
	if (max_mksp < best_mksp || (max_mksp == best_mksp && var < best_var)) {
	#pragma omp critical
	{
	// updates best cut, if this is the case
	if (max_mksp < best_mksp || (max_mksp == best_mksp && var < best_var)) {
		best_mksp = max_mksp;
		best_var = var;
		list<PipelineComponentBase*> flat_c;
		for (list<PipelineComponentBase*> bucket : c)
			flat_c.insert(flat_c.begin(), bucket.begin(), bucket.end());
		best_cut = pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>>(c1, flat_c);
	}
	}
	}
}

list<list<PipelineComponentBase*>> montecarlo_recursive_cut(list<PipelineComponentBase*> rem, 
	const map<int, PipelineComponentBase*> &all_stages, int n, map<int, ArgumentBase*> &args, map<string, list<ArgumentBase*>> ref) {

	// list<pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>>> current_cuts;
	pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> best_cut;
	list<list<PipelineComponentBase*>> final_cut;
	float best_mksp = FLT_MAX;
	float best_var = FLT_MAX;
	int r_size = rem.size();

	cout << "[montecarlo_recursive_cut] n=" << n << endl;

	// stoping condition
	if (n == 1 || rem.size() == 1) {
		final_cut.emplace_back(rem);
		cout << "[montecarlo_recursive_cut] n=" << n << " retuned1" << endl;
		cout << "\tb0" << endl;
		for (PipelineComponentBase* s : rem)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
		return final_cut;
	}

	#pragma omp parallel
	{
	#pragma omp single
	{
	// attempt all possible cuts on this level
	list<PipelineComponentBase*> rest;
	while (rem.size() > 1) {
		// attempt a cut
		pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>> c; 
		c = get_cut(rem, all_stages, args, ref);

		// store the cut for the rest to be calculated later
		float w1 = calc_stage_proc(c.first, args, ref);
		float w2 = calc_stage_proc(c.second, args, ref);

		list<PipelineComponentBase*> c1 = w1>=w2?c.first:c.second;
		list<PipelineComponentBase*> c2 = w1>=w2?c.second:c.first;

		// add rest to c2
		c2.insert(c2.begin(), rest.begin(), rest.end());

		#pragma omp task
		montecarlo_dep(n, c1, c2, all_stages, args, ref, best_mksp, best_var, best_cut);

		cout << "[montecarlo_recursive_cut] n=" << n << " adding cut" << endl;
		cout << "\tc1:" << endl;
		for (PipelineComponentBase* s : c1)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
		cout << "\tc2:" << endl;
		for (PipelineComponentBase* s : c2)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;

		// update rem to get a new cut
		rem = c1;
		rest = c2;
	}
	}
	#pragma barrier
	}

	// cout << "---------------------------------------------------" << endl;

	// #pragma omp parallel
	// {
	// #pragma omp single
	// {
	// // runs all dependencies a la montecarlo
	// for (list<pair<list<PipelineComponentBase*>, list<PipelineComponentBase*>>>::iterator i=current_cuts.begin();
	// 		i!=current_cuts.end(); i++) {
	// 	// WARNING: best cut is shared between all montecarlo_dep runs
	// 	#pragma omp task
	// 	montecarlo_dep(n, i->first, i->second, all_stages, args, ref, best_mksp, best_var, best_cut);
	// }
	// }

	cout << "[montecarlo_recursive_cut] n=" << n << " best cut of the stage with mksp=" << best_mksp << endl;
		cout << "\tc1:" << endl;
		for (PipelineComponentBase* s : best_cut.first)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
		cout << "\tc2:" << endl;
		for (PipelineComponentBase* s : best_cut.second)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;

	// recursive call on the best rest
	final_cut = montecarlo_recursive_cut(best_cut.second, all_stages, n-1, args, ref);
	final_cut.emplace_back(best_cut.first);

	cout << "[montecarlo_recursive_cut] n=" << n << " final cut:" << endl;
	int i=0;
	for (list<PipelineComponentBase*> bucket : final_cut) {
		cout << "\tb:" << i++ << endl;
		for (PipelineComponentBase* s : bucket)
			cout << "\t\t" << s->getId() << ":" << s->getName() << endl;
	}

	return final_cut;
}

void merge_stages_fine_grain(const map<int, PipelineComponentBase*> &all_stages, 
	const map<int, PipelineComponentBase*> &stages_ref, map<int, PipelineComponentBase*> &merged_stages, 
	RegionTemplate* rt, map<int, ArgumentBase*> expanded_args, int max_bucket_size) {

	// attempt merging for each stage type
	for (map<int, PipelineComponentBase*>::const_iterator ref=stages_ref.cbegin(); ref!=stages_ref.cend(); ref++) {
		// get only the stages from the current stage_ref
		list<PipelineComponentBase*> current_stages;
		filter_stages(all_stages, ref->second->getName(), current_stages);

		cout << "[merge_stages_fine_grain] Generating tasks..." << endl;
		// generate all tasks
		for (list<PipelineComponentBase*>::iterator s=current_stages.begin(); s!=current_stages.end(); ) {
			// if the stage isn't composed of reusable tasks then 
			(*s)->tasks = task_generator(ref->second->tasksDesc, *s, rt, expanded_args);
			if ((*s)->tasks.size() == 0) {
				merged_stages[(*s)->getId()] = *s;
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

		list<list<PipelineComponentBase*>> solution = reuse_tree_merging(current_stages, all_stages, 
			max_bucket_size, expanded_args, ref->second->tasksDesc);

		// write merging solution
		ofstream solution_file;
		solution_file.open("fine-grain-merging-solution-b" + to_string(max_bucket_size), ios::trunc);
		cout << endl << "solution:" << endl;
		solution_file << endl << "solution:" << endl;
		for (list<PipelineComponentBase*> b : solution) {
			cout << "\tbucket with " << b.size() << " stages and cost "
				<< calc_stage_proc(b, expanded_args, ref->second->tasksDesc) << ":" << endl;
			solution_file << "\tbucket with " << b.size() << " stages and cost "
				<< calc_stage_proc(b, expanded_args, ref->second->tasksDesc) << ":" << endl;
			for (PipelineComponentBase* s : b) {
				cout << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
				// solution_file << "\t\tstage " << s->getId() << ":" << s->getName() << ":" << endl;
			}
		}
		solution_file.close();

		// merge all stages in each bucket, given that they are mergable
		for (list<PipelineComponentBase*> bucket : solution) {
			// cout << "bucket merging" << endl;
			list<PipelineComponentBase*> curr = merge_stages_full(bucket, expanded_args, ref->second->tasksDesc);
			// send rem stages to merged_stages
			for (PipelineComponentBase* s : curr) {
				// cout << "\tadding stage " << s->getId() << endl;
				merged_stages[s->getId()] = s;
			}
		}
	}
}

void generate_drs(RegionTemplate* rt, 
	const map<int, ArgumentBase*> &expanded_args) {

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

void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, 
	map<int, ArgumentBase*> &merged_arguments,
	RegionTemplate *rt) {

	int i=0;
	for (pair<int, PipelineComponentBase*> stage : merged_stages) {
		// add input arguments to stage, adding them as RT as needed
		for (int arg_id : stage.second->getInputs()) {
			ArgumentBase* new_arg = merged_arguments[arg_id]->clone();
			new_arg->setParent(merged_arguments[arg_id]->getParent());
			stage.second->addArgument(new_arg);
			if (new_arg->getType() == ArgumentBase::RT) {
				cout << "input RT : " << merged_arguments[arg_id]->getName() << endl;
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
				cout << "[before]Dependency: " << stage.second->getId() << ":" << stage.second->getName()
					<< " ->addDependency( " << parent << " )" << endl;
				if (merged_stages[merged_arguments[arg_id]->getParent()]->reused != NULL)
					parent = merged_stages[merged_arguments[arg_id]->getParent()]->reused->getId();
				cout << "Dependency: " << stage.second->getId() << ":" << stage.second->getName()
					<< " ->addDependency( " << parent << " )" << endl;
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
				cout << "output RT : " << merged_arguments[arg_id]->getName() << endl;
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
/************* Workflow parsing helper functions ***************/
/***************************************************************/

int get_line(char** line, FILE* f) {
	char* nline;
	size_t length=0;
	if (line_buffer.empty()) {
		if (getline(&nline, &length, f) == -1)
			return -1;
		string sline(nline);
		size_t pos=string::npos;
		while ((pos = sline.find("><")) != string::npos) {
			line_buffer.emplace_back(sline.substr(0,pos+1));
			sline = sline.substr(pos+1);
		}
		line_buffer.emplace_back(sline);
	}

	char* cline = (char*)malloc((line_buffer.front().length()+1)*sizeof(char*));
	memcpy(cline, line_buffer.front().c_str(), line_buffer.front().length()+1);
	*line = cline;
	line_buffer.pop_front();
	return strlen(*line);
}

string get_workflow_name(FILE* workflow) {
	return get_workflow_field(workflow, "name");
}

string get_workflow_field(FILE* workflow, string field) {
	char *line = NULL;
	size_t len = 0;
	
	// create field regex
	regex r ("<" + field + ">[\"\\:\\w {},.~\\/\\[\\]-]+<\\/" + field + ">");

	// get a new line until name is found
	while (get_line(&line, workflow) != -1) {
		smatch match;
		string s(line);
		regex_search(s, match, r);

		// cout << "line: " << s << endl;

		// if got a name match
		if (match.size() == 1) {
			// cout << "field match: " << line << endl;
			return s.substr(s.find("<" + field + ">")+field.length()+2, 
				s.find("</" + field + ">")-s.find("<" + field + ">")-field.length()-2);
		}
	}

	return nullptr;
}

void get_workflow_arguments(FILE* workflow, 
	list<ArgumentBase*> &output_arguments) {

	char *line = NULL;
	size_t len = 0;

	// initial ports section beginning and end
	string ie("<outputs>");
	string iee("</outputs>");
	
	// ports section beginning and end
	string e("<entry>");
	string ee("</entry>");

	// go to the initial entries beginning
	while (get_line(&line, workflow) != -1 && string(line).find(ie) == string::npos);
	// cout << "port init begin: " << line << endl;

	// keep getting ports until it reaches the end of initial ports
	while (get_line(&line, workflow) != -1 && string(line).find(iee) == string::npos) {
		// consumes the port beginning
		while (string(line).find(e) == string::npos && get_line(&line, workflow) != -1);
		// cout << "port begin: " << line << endl;

		// finds the name and field
		string name = get_workflow_field(workflow, "string");

		// generate an argument
		string type = get_workflow_field(workflow, "path");
		ArgumentBase* arg = new_typed_arg_base(type);
		arg->setName(name);
		arg->setId(new_uid());
		output_arguments.emplace_back(arg);

		// consumes the port ending
		while (get_line(&line, workflow) != -1 && string(line).find(ee) == string::npos);
		// 	cout << "not port end: " << line << endl;
		// cout << "port end: " << line << endl;
	}
	// cout << "port init end: " << line << endl;
}

vector<general_field_t> get_all_fields(FILE* workflow, string start, string end) {
	char *line = NULL;
	size_t len = 0;
	string type;
	string field;
	vector<general_field_t> fields;
	general_field_t general_field;

	// consumes the beginning
	while (get_line(&line, workflow) != -1 && string(line).find(start) == string::npos);

	// create general field regex
	regex r ("<[\\w]+>[\\w ]+<\\/[\\w]+>");
	
	// keep fiding fields until the end
	while (get_line(&line, workflow) != -1 && string(line).find(end) == string::npos) {
		smatch match;
		string s(line);
		regex_search(s, match, r);

		// if got a general field match
		if (match.size() == 1) {
			// cout << "general field match: " << line << endl;
			type = s.substr(s.find("<")+1, s.find(">")-s.find("<")-1);
			field = s.substr(s.find("<" + type + ">")+type.length()+2, s.find("</" + type + ">")-s.find("<" + type + ">")-type.length()-2);
			// cout << "type: " << type << ", field: " << field << endl;
			general_field.type = type;
			general_field.data = field;
			fields.push_back(general_field);
		}
	}
	return fields;
}

PipelineComponentBase* find_stage(map<int, PipelineComponentBase*> stages, string name) {
	for (pair<int, PipelineComponentBase*> p : stages)
		if (p.second->getName().compare(name) == 0)
			return p.second;
	return NULL;
}

int find_stage_id(map<int, PipelineComponentBase*> stages, string name) {
	for (pair<int, PipelineComponentBase*> p : stages)
		if (p.second->getName().compare(name) == 0)
			return p.first;
	return -1;
}

ArgumentBase* find_argument(const map<int, ArgumentBase*>& arguments, string name) {
	for (pair<int, ArgumentBase*> p : arguments)
		if (p.second->getName().compare(name) == 0)
			return p.second;
	return NULL;
}

ArgumentBase* find_argument(const list<ArgumentBase*>& arguments, int id) {
	for (ArgumentBase* a : arguments)
		if (a->getId() == id)
			return a;
	return NULL;
}

ArgumentBase* find_argument(const list<ArgumentBase*>& arguments, string name) {
	for (ArgumentBase* a : arguments)
		if (a->getName().compare(name) == 0)
			return a;
	return NULL;
}

// taken from: http://stackoverflow.com/questions/16388510/evaluate-a-string-with-a-switch-in-c
constexpr unsigned int str2int(const char* str, int h = 0) {
	return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

ArgumentBase* new_typed_arg_base(string type) {
	switch (str2int(type.c_str())) {
		case str2int("integer"):
			return new ArgumentInt();
		case str2int("float"):
			return new ArgumentFloat();
		case str2int("string"):
			return new ArgumentString();
		case str2int("floatarray"):
			return new ArgumentFloatArray();
		case str2int("rt"):
			return new ArgumentRT();
		default:
			return NULL;
	}
}

parsing::port_type_t get_port_type(string s) {
	switch (str2int(s.c_str())) {
		case str2int("integer"):
			return parsing::int_t;
		case str2int("float"):
			return parsing::float_t;
		case str2int("string"):
			return parsing::string_t;
		case str2int("floatarray"):
			return parsing::float_array_t;
		case str2int("rt"):
			return parsing::rt_t;
		default:
			return parsing::error;
	}
}
