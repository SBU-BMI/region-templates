#include <stdio.h>

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <regex>

#include "json/json.h"

#include "SysEnv.h"
#include "Argument.h"
#include "PipelineComponentBase.h"
#include "RTPipelineComponentBase.h"
#include "RegionTemplateCollection.h"
#include "ReusableTask.hpp"

using namespace std;

namespace parsing {
enum port_type_t {
	int_t, string_t, float_t, float_array_t, rt_t, error
};
}

// global uid for any local(this file) entity
int uid=1;
int new_uid() {return uid++;}

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
	const map<int, PipelineComponentBase*> &stages_ref,	map<int, PipelineComponentBase*> &merged_stages, 
	RegionTemplate* rt, map<int, ArgumentBase*> expanded_args);
void generate_drs(RegionTemplate* rt, const map<int, ArgumentBase*> &expanded_args);
void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, 
	map<int, ArgumentBase*> &merged_arguments,
	RegionTemplate *rt);

// Workflow parsing helper functions
list<string> line_buffer;
int get_line(char** line, FILE* f);
string get_workflow_name(FILE* workflow);
string get_workflow_field(FILE* workflow, string field);
void get_workflow_arguments(FILE* workflow, list<ArgumentBase*> &output_arguments);
vector<general_field_t> get_all_fields(FILE* workflow, string start, string end);
PipelineComponentBase* find_stage(map<int, PipelineComponentBase*> stages, string name);
int find_stage_id(map<int, PipelineComponentBase*> stages, string name);
ArgumentBase* find_argument(const map<int, ArgumentBase*> arguments, string name);
ArgumentBase* new_typed_arg_base(string type);
parsing::port_type_t get_port_type(string s);

int main(int argc, char* argv[]) {

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");

	// region template used by all stages
	RegionTemplate *rt = new RegionTemplate();
	rt->setName("tile");

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
	cout << endl << "base_stages:" << endl;
	for (pair<int, PipelineComponentBase*> p : base_stages) {
		cout << p.first << ":" << p.second->getName() << endl;
		cout << "\toutputs: " << p.second->getOutputs().size() << endl << endl;
		for (int i : p.second->getOutputs())
			cout << "\t\t" << i << ":" << interstage_arguments[i]->getName() << endl;
		cout << "\t task descriptors:" << endl;
		for (pair<string, list<ArgumentBase*>> d : p.second->tasksDesc) {
			cout << "\t\ttask: " << d.first << endl;
			for (ArgumentBase* a : d.second)
				cout << "\t\t\t" << a->getName() << endl;
		}
	}


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
	// 	cout << "\tinputs: " << p.second->getInputs().size() << endl << endl;
	// 	for (int i : p.second->getInputs())
	// 		cout << "\t\t" << i << ":" << all_argument[i]->getName() << endl;
	// 	cout << "\toutputs: " << p.second->getOutputs().size() << endl << endl;
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

	expand_stages(args, parameters_values, expanded_args, 
		base_stages, expanded_stages, workflow_outputs);

	cout << endl<< "merged: " << endl;
	for (pair<int, PipelineComponentBase*> p : expanded_stages) {
		cout << "stage " << p.second->getId() << ":" << p.second->getName() << endl;
		cout << "\tinputs: " << endl;
		for (int i : p.second->getInputs())
			cout << "\t\t" << i << ":" << expanded_args[i]->getName() << endl;
		cout << "\toutputs: " << endl;
		for (int i : p.second->getOutputs())
			cout << "\t\t" << i << ":" << expanded_args[i]->getName() << endl;
	}

	cout << endl << "merged args" << endl;
	for (pair<int, ArgumentBase*> p : expanded_args)
		cout << "\t" << p.first << ":" << p.second->getName() << " = " 
			<< p.second->toString() << " sized: " << p.second->size() << endl;

	map<int, PipelineComponentBase*> merged_stages;
	merge_stages_fine_grain(expanded_stages, base_stages, merged_stages, rt, expanded_args);

	cout << endl<< "merged-fine: " << endl;
	for (pair<int, PipelineComponentBase*> p : merged_stages) {
		string s = p.second->reused!=NULL?" - reused":"";
		cout << "stage " << p.second->getId() << ":" << p.second->getName() << s << endl;
		cout << "\ttasks: " << endl;
		for (ReusableTask* t : p.second->tasks) {
			cout << "\t\t" << t->getId() << ":" << t->getTaskName() << endl;
			cout << "\t\t\tparent_task: " << t->parentTask << endl;
			t->print();
		}
	}

	//------------------------------------------------------------
	// Add workflows to Manager to be executed
	//------------------------------------------------------------

	string inputFolderPath = "~/Desktop/images15";
	cout << endl << "generate_drs" << endl;
	generate_drs(rt, expanded_args);

	// RegionTemplateCollection* rts;
	// rts = RTFromFiles(inputFolderPath);

	// add arguments to each stage
	cout << endl << "add_arguments_to_stages" << endl;
	add_arguments_to_stages(merged_stages, expanded_args, rt);

	// add all stages to manager
	cout << endl << "executeComponent" << endl;
	for (pair<int, PipelineComponentBase*> s : merged_stages) {
		if (s.second->reused == NULL) {
			cout << "sent component " << s.second->getId() << ":" 
				<< s.second->getName() << " to execute with args:" << endl;
			cout << "\tinputs: " << endl;
			for (int i : s.second->getInputs())
				cout << "\t\t" << i << ":" << expanded_args[i]->getName() << " = " 
					<< expanded_args[i]->toString() << " parent " << expanded_args[i]->getParent() << endl;
			cout << "\toutputs: " << endl;
			for (int i : s.second->getOutputs())
				cout << "\t\t" << i << ":" << expanded_args[i]->getName() << " = " 
					<< expanded_args[i]->toString() << endl;
			((Task*)s.second)->setId(s.second->getId());
			sysEnv.executeComponent(s.second);
		}
	}

	// execute workflows
	cout << endl << "startupExecution" << endl;
	sysEnv.startupExecution();

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

	// cout << endl << "arg_values:" << endl;
	for (pair<int, list<ArgumentBase*>> p : args_values) {
		// cout << "base argument " << p.first << ":" << args.at(p.first)->getName() << endl;
		for (ArgumentBase* a : p.second) {
			a->setId(new_uid());
			a->setName(args.at(p.first)->getName());
			// cout << "\t" << a->getId() << ":" << a->getName() << " = " << a->toString() << endl;
		}
	}

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

ArgumentBase* find_argument(PipelineComponentBase* p, string name, map<int, ArgumentBase*> expanded_args) {
	for(int i : p->getInputs()){
		if (expanded_args[i]->getName().compare(name) == 0) {
			return expanded_args[i];
		}
	}
	for(int i : p->getOutputs()){
		if (expanded_args[i]->getName().compare(name) == 0) {
			return expanded_args[i];
		}
	}

	return NULL;
}

// This function assumes that the merged PCB has at least, but not limited to one ReusableTask on tasks
// of type task_name. Also, to_merge must have exactly one ReusableTask of type task_name. These
// conditions are not checked.
bool exists_reusable_task(PipelineComponentBase* merged, PipelineComponentBase* to_merge, string task_name) {
	// get the only task of to_merge that has the type task_name
	ReusableTask* to_merge_task = NULL;
	for (ReusableTask* t : to_merge->tasks)
		if (t->getTaskName().compare(task_name) == 0)
			to_merge_task = t;

	// attempt to find the same task on merged
	for (ReusableTask* t : merged->tasks)
		if (t->getTaskName().compare(task_name) == 0 && 
				to_merge_task->reusable(t))
			return true;

	return false;
}

// for the meantime the mearging will happen whenever at least the first task is reusable
bool merging_condition(PipelineComponentBase* merged, PipelineComponentBase* to_merge, map<string, list<ArgumentBase*>> ref) {
	
	// compatibility with stages that dont implement task reuse
	if (ref.size() == 0)
		return false;

	// verify if the first task is reusable
	if (exists_reusable_task(merged, to_merge, ref.begin()->first))
		return true;
	else
		return false;
}

// filters all the stages from an input map by the stage's name
void filter_stages(const map<int, PipelineComponentBase*> &all_stages, 
	string stage_name, list<PipelineComponentBase*> &filtered_stages) {

	for (pair<int, PipelineComponentBase*> p : all_stages)
		if (p.second->getName().compare(stage_name) == 0)
			filtered_stages.emplace_back(p.second);
}

list<ReusableTask*> task_generator(map<string, list<ArgumentBase*>> &tasks_desc, 
	PipelineComponentBase* p, RegionTemplate* rt, map<int, ArgumentBase*> expanded_args) {

	list<ReusableTask*> tasks;
	ReusableTask* prev_task = NULL;

	// traverse the map on reverse order to set dependencies
	for (map<string, list<ArgumentBase*>>::reverse_iterator t=tasks_desc.rbegin(); t!=tasks_desc.rend(); t++) {
		// get task args
		list<ArgumentBase*> args;
		for (ArgumentBase* a : t->second) {
			ArgumentBase* aa = find_argument(p, a->getName(), expanded_args);
			if (aa != NULL)
				args.emplace_back(aa);
		}

		// call constructor
		int uid = new_uid();
		ReusableTask* n_task = ReusableTask::ReusableTaskFactory::getTaskFromName(t->first, args, rt);
		n_task->setId(uid);
		n_task->setTaskName(t->first);
		// set prevoius task dependency if this isn't the first task generated
		if (t != tasks_desc.rbegin()) {
			prev_task->parentTask = n_task->getId();
		}
		prev_task = n_task;
		tasks.emplace_back(n_task);
		// cout << "[task_generator] new task " << uid << ":" << t->first << " with size " << n_task->size() << endl;
	}

	return tasks;
}

ReusableTask* find_task(list<ReusableTask*> l, string name) {
	for (ReusableTask* t : l)
		if (t->getTaskName().compare(name) == 0)
			return t;
	return NULL;
}

list<ReusableTask*> find_tasks(list<ReusableTask*> l, string name) {
	list<ReusableTask*> tasks;
	for (ReusableTask* t : l)
		if (t->getTaskName().compare(name) == 0)
			tasks.emplace_back(t);
	return tasks;
}

void merge_stages(PipelineComponentBase* current, PipelineComponentBase* s, map<string, list<ArgumentBase*>> ref) {
	
	s->reused = current;

	ReusableTask* current_frontier_reusable_tasks;
	map<std::string, std::list<ArgumentBase*>>::iterator p=ref.begin();
	for (; p!=ref.end(); p++) {
		// verify if this is the first reusable task
		ReusableTask* t_s = find_task(s->tasks, p->first);
		list<ReusableTask*> t_cur = find_tasks(current->tasks, p->first);

		// check all of the same tasks of current
		bool reusable = false;
		for (ReusableTask* t : t_cur) {
			if (t->reusable(t_s)) {
				reusable = true;
				current_frontier_reusable_tasks = t;
				break;
			}
		}
		if (!reusable) {
			break;
		}
	}

	// updates the first non-reusable task dependency
	ReusableTask* frontier = find_task(s->tasks, p->first);
	frontier->parentTask = current_frontier_reusable_tasks->getId();
	current->tasks.emplace_front(frontier);
	p++;

	// adds the remaining non-reusable tasks to current
	for (; p!=ref.end(); p++) {
		current->tasks.emplace_front(find_task(s->tasks, p->first));
	}
}

void merge_stages_fine_grain(const map<int, PipelineComponentBase*> &all_stages, 
	const map<int, PipelineComponentBase*> &stages_ref, map<int, PipelineComponentBase*> &merged_stages, 
	RegionTemplate* rt, map<int, ArgumentBase*> expanded_args) {

	// attempt merging for each stage type
	for (map<int, PipelineComponentBase*>::const_iterator ref=stages_ref.cbegin(); ref!=stages_ref.cend(); ref++) {
		// get only the stages from the current stage_ref
		list<PipelineComponentBase*> current_stages;
		filter_stages(all_stages, ref->second->getName(), current_stages);

		// attempt to merge all current stages
		while (!current_stages.empty()) {
			// get one stage to start merging
			PipelineComponentBase* current = current_stages.front();
			current_stages.pop_front();

			// move forward if this stage was merged with another one
			if (current->reused == NULL) {				

				// start by generating all tasks
				if (current->tasks.size() == 0)
					current->tasks = task_generator(ref->second->tasksDesc, current, rt, expanded_args);

				// attempt to merge all stages to the current merged stages
				list<PipelineComponentBase*>::iterator i = current_stages.begin();
				while (i != current_stages.end()) {
					// add stage to merged stages if the merging condition is true
					PipelineComponentBase* s = *i;

					// generate tasks if it they weren't generated yet
					if (s->tasks.size() == 0)
						s->tasks = task_generator(ref->second->tasksDesc, s, rt, expanded_args);

					if (merging_condition(s, current, ref->second->tasksDesc)) {
						cout << "[merge_stages_fine_grain] mearging " << current->getId() << " with " << s->getId() << endl;
						merge_stages(current, s, ref->second->tasksDesc);
					}
					i++;
				}
			}

			// add merged stages to final map as one stage
			merged_stages[((PipelineComponentBase*)current)->getId()] = (PipelineComponentBase*)current;
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
			stage.second->addArgument(new_arg);
			if (merged_arguments[arg_id]->getType() == ArgumentBase::RT) {
				// cout << "input RT : " << merged_arguments[arg_id]->getName() << endl;
				// insert the region template on the parent stage if the argument is a DR and if the RT wasn't already added
				if (((RTPipelineComponentBase*)stage.second)->
						getRegionTemplateInstance(rt->getName()) == NULL)
					((RTPipelineComponentBase*)stage.second)->addRegionTemplateInstance(rt, rt->getName());
				((RTPipelineComponentBase*)stage.second)->addInputOutputDataRegion(
					rt->getName(), merged_arguments[arg_id]->getName(), RTPipelineComponentBase::INPUT);
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
			stage.second->addArgument(new_arg);
			if (merged_arguments[arg_id]->getType() == ArgumentBase::RT) {
				// cout << "output RT : " << merged_arguments[arg_id]->getName() << endl;
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

ArgumentBase* find_argument(const map<int, ArgumentBase*> arguments, string name) {
	for (pair<int, ArgumentBase*> p : arguments)
		if (p.second->getName().compare(name) == 0)
			return p.second;
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
