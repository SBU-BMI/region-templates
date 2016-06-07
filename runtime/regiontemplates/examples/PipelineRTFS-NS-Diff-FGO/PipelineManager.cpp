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

using namespace std;

namespace parsing {
enum port_type_t {
	int_t, string_t, float_t, float_array_t, error
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
	for (map<int, ArgumentBase*>::iterator it = mapp.begin(); it != mapp.end(); ++it)
	// for (map<int, ArgumentBase*>::iterator i=map.begin(); i!=map.end();i++)
		cout << it->first << ":" << it->second->getName() << endl;
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

void listprint(list<ArgumentBase*> listt) {
	for (list<ArgumentBase*>::iterator i=listt.begin(); i!=listt.end(); i++)
		cout << (*i)->getName() << endl;
}

void listprint(list<int> listt) {
	for (list<int>::iterator i=listt.begin(); i!=listt.end(); i++)
		cout << (*i) << endl;
}

// BUG??? g++4.9 bad???
//
list<int>::iterator beg(list<int> l) {return l.begin();}
list<int>::iterator endd(list<int> l) {return l.end();}
list<int>::iterator inc(list<int>::iterator &i) {return i++;}
//
// for (map<int, PipelineComponentBase*>::iterator it = base_stages.begin(); it != base_stages.end(); ++it) {
// 	cout << "stage: " << it->second->getName() << endl;
// 	for (list<int>::iterator i = it->second->getOutputs().begin(); i != (it->second->getOutputs().end()); ++i) {
// 		list<int> l = it->second->getOutputs();
// 		// works
// 		listprint(l);
// 		// also works
// 		listprint(it->second->getOutputs());
// 		// this works too
// 		for (list<int>::iterator i=l.begin(); i!=l.end(); i++)
// 			cout << (*i) << endl;
// 		// not this
// 		for (list<int>::iterator i=it->second->getOutputs().begin(); i!=it->second->getOutputs().end(); i++)
// 			cout << (*i) << endl;
// 		// nor this
// 		for (list<int>::iterator i=beg(it->second->getOutputs()); i!=endd(it->second->getOutputs()); inc(i))
// 			cout << (*i) << endl;
// 		// this also works
// 		for (int i:it->second->getOutputs())
// 			cout << i << endl;
// 	}
// }



// Workflow parsing functions
void get_inputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_inputs, 
	map<int, list<ArgumentBase*>> &parameters_values);
void get_outputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_outputs);
void get_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments);
void connect_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> &base_stages, 
	map<int, ArgumentBase*> &interstage_arguments, map<int, ArgumentBase*> &input_arguments,
	map<int, list<int>> &deps);
list<map<int, ArgumentBase*>> expand_parameters_combinations(map<int, list<ArgumentBase*>> parameters_values, 
	map<int, ArgumentBase*> workflow_inputs);
void expand_arguments(map<int, ArgumentBase*> &ref_arguments, int copies, 
	list<map<int, ArgumentBase*>> &copy_arguments);
void expand_stages(map<int, PipelineComponentBase*> &base_stages, int copies, 
	list<map<int, PipelineComponentBase*>> &all_stages);
void iterative_full_merging(list<map<int, ArgumentBase*>> &all_inputs, list<map<int, ArgumentBase*>> &all_outputs, 
	list<map<int, PipelineComponentBase*>> &all_stages, list<map<int, ArgumentBase*>> &all_interstage_arguments, 
	map<int, ArgumentBase*> &interstage_arguments_ref, map<int, ArgumentBase*> &merged_arguments, 
	map<int, ArgumentBase*> &merged_outputs, map<int, PipelineComponentBase*> &merged_stages);
void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, map<int, ArgumentBase*> &merged_arguments, 
	map<int, list<int>> &deps, RegionTemplateCollection *rts);

// Workflow parsing helper functions
list<string> line_buffer;
int get_line(char** line, FILE* f);
string get_workflow_name(FILE* workflow);
string get_workflow_field(FILE* workflow, string field);
list<string> get_workflow_input_arguments(FILE* workflow, string entry_type);
list<ArgumentBase*> get_workflow_output_arguments(FILE* workflow, string entry_type);
void get_workflow_arguments(FILE* workflow, string entry_type, list<ArgumentBase*> &output_arguments, 
	list<string> &input_arguments, bool is_output);	
vector<general_field_t> get_all_fields(FILE* workflow, string start, string end);
PipelineComponentBase* find_stage(map<int, PipelineComponentBase*> stages, string name);
ArgumentBase* find_argument(map<int, ArgumentBase*> arguments, string name);
ArgumentBase* new_typed_arg_base(string type);
parsing::port_type_t get_port_type(string s);
map<int, ArgumentBase*> cpy_ab_map(map<int, ArgumentBase*> &ref);
template <class T>
T map_pop(map<int, T> &m);
int find_id_by_name(string name, map<int, ArgumentBase*> &ref);



int main(int argc, char* argv[]) {

	// Handler to the distributed execution system environment
	SysEnv sysEnv;

	// Tell the system which libraries should be used
	sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");


	FILE* workflow_descriptor = fopen("seg_example.t2flow", "r");

	//------------------------------------------------------------
	// Parse pipeline file
	//------------------------------------------------------------

	// get all workflow inputs without their values, returning also the parameters 
	// values (i.e list<ArgumentBase> values) on another map
	map<int, ArgumentBase*> workflow_inputs;
	map<int, list<ArgumentBase*>> parameters_values;
	get_inputs_from_file(workflow_descriptor, workflow_inputs, parameters_values);

	cout << "workflow_inputs:" << endl;
	for (pair<int, ArgumentBase*> p : workflow_inputs)
		cout << p.first << ":" << p.second->getName() << endl;

	cout << endl << "parameters_values:" << endl;
	for (pair<int, list<ArgumentBase*>> p : parameters_values) {
		cout << "argument " << p.first << ":" << workflow_inputs[p.first]->getName() << ":" << endl;
		for (ArgumentBase* a : p.second)
			cout << "\t" << a->toString() << endl;
	}

	// get all workflow outputs
	map<int, ArgumentBase*> workflow_outputs;
	get_outputs_from_file(workflow_descriptor, workflow_outputs);
	cout << endl << "workflow_outputs " << endl;
	for (pair<int, ArgumentBase*> p : workflow_outputs)
		cout << p.first << ":" << p.second->getName() << endl;

	// get all stages, also setting the uid from this context to Task (i.e Task::setId())
	// also returns the list of arguments used
	// the stages dependencies are also set here (i.e Task::addDependency())
	map<int, PipelineComponentBase*> base_stages;
	map<int, ArgumentBase*> interstage_arguments;
	get_stages_from_file(workflow_descriptor, base_stages, interstage_arguments);
	cout << endl << "base_stages:" << endl;
	for (pair<int, PipelineComponentBase*> p : base_stages) {
		cout << p.first << ":" << p.second->getName() << ", outputs: " << p.second->getOutputs().size() << endl << endl;
		for (int i : p.second->getOutputs())
			cout << "\t" << i << ":" << interstage_arguments[i]->getName() << endl;
	}

	cout << endl << "interstage_arguments:" << endl;
	for (pair<int, ArgumentBase*> p : interstage_arguments)
		cout << p.first << ":" << p.second->getName() << endl;

	// this map is a dependency structure: stage -> dependency_list
	map<int, list<int>> deps;
	
	// connect the stages inputs/outputs 
	connect_stages_from_file(workflow_descriptor, base_stages, interstage_arguments, workflow_inputs, deps);
	map<int, ArgumentBase*> all_argument(workflow_inputs);
	for (pair<int, ArgumentBase*> a : interstage_arguments)
		all_argument[a.first] = a.second;

	cout << endl << "all_arguments:" << endl;
	mapprint(all_argument);

	cout << endl << "deps:" << endl;
	for (pair<int, list<int>> p : deps)
		for (int d : p.second)
			cout << "\t" << base_stages[p.first]->getName() << " depends on " << base_stages[d]->getName() << endl;

// buggy buggggy
/*for (map<int, PipelineComponentBase*>::iterator it = base_stages.begin(); it != base_stages.end(); ++it) {
	cout << "stage: " << it->second->getName() << endl;
	for (list<int>::iterator i = it->second->getOutputs().begin(); i != (it->second->getOutputs().end()); ++i) {
		list<int> l = it->second->getOutputs();
		// works
		listprint(l);
		// also works
		listprint(it->second->getOutputs());
		// this works too
		for (list<int>::iterator i=l.begin(); i!=l.end(); i++)
			cout << (*i) << endl;
		// not this
		for (list<int>::iterator i=it->second->getOutputs().begin(); i!=it->second->getOutputs().end(); i++)
			cout << (*i) << endl;
		// nor this
		for (list<int>::iterator i=beg(it->second->getOutputs()); i!=endd(it->second->getOutputs()); inc(i))
			cout << (*i) << endl;
		// this also works
		for (int i:it->second->getOutputs())
			cout << i << endl;
	}
}*/


	cout << endl << "connected base_stages:" << endl;
	for (pair<int, PipelineComponentBase*> p : base_stages) {
		cout << p.first << ":" << p.second->getName() << endl;
		cout << "\tinputs: " << p.second->getInputs().size() << endl << endl;
		for (int i : p.second->getInputs())
			cout << "\t\t" << i << ":" << all_argument[i]->getName() << endl;
		cout << "\toutputs: " << p.second->getOutputs().size() << endl << endl;
		for (int i : p.second->getOutputs())
			cout << "\t\t" << i << ":" << all_argument[i]->getName() << endl;
	}

	//------------------------------------------------------------
	// Add create all combinarions of parameters
	//------------------------------------------------------------

	// expand parameter combinations returning a list of parameters sets each set contains 
	// exactly one value for every input parameter. the agrument objects are ready to be used 
	// on the workflow execution.
	list<map<int, ArgumentBase*>> expanded_parameters;
	expanded_parameters = expand_parameters_combinations(parameters_values, workflow_inputs);
	int i = 0;
	for (map<int, ArgumentBase*> parameter_set : expanded_parameters) {
		cout << "Parameter set " << i++ << endl;
		for (pair<int, ArgumentBase*> p : parameter_set) {
			cout << "\t" << p.first << ":" << p.second->getName() << " = " << p.second->toString() << endl;
		}
		cout << endl;
	}

	// replicate outputs
	list<map<int, ArgumentBase*>> all_outputs;
	expand_arguments(workflow_outputs, expanded_parameters.size(), all_outputs);
	i = 0;
	for (map<int, ArgumentBase*> parameter_set : all_outputs) {
		cout << "Output set " << i++ << endl;
		for (pair<int, ArgumentBase*> p : parameter_set) {
			cout << "\t" << p.first << ":" << p.second->getName() << endl;
		}
		cout << endl;
	}

	// replicate interstage arguments
	list<map<int, ArgumentBase*>> all_interstage_arguments;
	expand_arguments(interstage_arguments, expanded_parameters.size(), all_interstage_arguments);
	i = 0;
	for (map<int, ArgumentBase*> parameter_set : all_interstage_arguments) {
		cout << "Interstage arguments set " << i++ << endl;
		for (pair<int, ArgumentBase*> p : parameter_set) {
			cout << "\t" << p.first << ":" << p.second->getName() << endl;
		}
		cout << endl;
	}

	// replicate stages
	list<map<int, PipelineComponentBase*>> all_stages;
	expand_stages(base_stages, expanded_parameters.size(), all_stages);
	i = 0;
	for (map<int, PipelineComponentBase*> stage_set : all_stages) {
		cout << "Stage set " << i++ << endl;
		for (pair<int, PipelineComponentBase*> p : stage_set) {
			cout << "\t" << p.first << ":" << p.second->getName() << " with inputs:" << endl;
			for (int inp : p.second->getInputs()) {
				cout << "\t\t" << inp << ":" << all_argument[inp]->getName() << endl;
			}
			cout << "\tand outputs: " << endl;
			for (int out : p.second->getOutputs()) {
				cout << "\t\t" << out << ":" << all_argument[out]->getName() << endl;
			}
		}
		cout << endl;
	}

	//------------------------------------------------------------
	// Iterative merging of stages
	//------------------------------------------------------------

	// maybe try to merge the inputs, and then merge stages iteratively
	// again, updating the uids (Task and local id)
	map<int, ArgumentBase*> merged_arguments;
	map<int, ArgumentBase*> merged_outputs;
	map<int, PipelineComponentBase*> merged_stages;
	iterative_full_merging(expanded_parameters, all_outputs, all_stages, all_interstage_arguments, 
		interstage_arguments, merged_arguments, merged_outputs, merged_stages);

	cout << endl << "merged stages" << endl;
	for (pair<int, PipelineComponentBase*> p : merged_stages) {
		cout << "\t" << p.first << ":" << p.second->getName() << " with inputs:" << endl;
		for (int inp : p.second->getInputs()) {
			cout << "\t\t" << inp << ":" << merged_arguments[inp]->getName() << endl;
		}
		cout << "\tand outputs: " << endl;
		for (int out : p.second->getOutputs()) {
			cout << "\t\t" << out << ":" << merged_arguments[out]->getName() << endl;
		}
	}
	cout << endl;

	//------------------------------------------------------------
	// Add workflows to Manager to be executed
	//------------------------------------------------------------

	// Create region templates description without instantiating data
	RegionTemplateCollection *rts;
	// string inputFolderPath = "~/Desktop/images15"
	// rtCollection = RTFromFiles(inputFolderPath);

	// add arguments to each stage
	add_arguments_to_stages(merged_stages, merged_arguments, deps, rts);

	// add all stages to manager
	for (pair<int, PipelineComponentBase*> s : merged_stages)
		sysEnv.executeComponent(s.second);

	// execute workflows
	sysEnv.startupExecution();

	// get results
	// for each ArgumentBase output of merged_outputs do
	// 	cout << sysEnv.getComponentResultData(output.getId()) << endl;
	// end

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
			default:
				exit(-4);
		}

		// set inp_arg name and id
		inp_arg->setName(name);
		int arg_id = new_uid();
		inp_arg->setId(arg_id);

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
			default:
				exit(-4);
		}
		
		out_arg->setName(name);
		workflow_outputs[new_uid()] = out_arg;

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
		list<ArgumentBase*> outputs = get_workflow_output_arguments(workflow_descriptor, "outputs");
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
	map<int, list<int>> &deps) {

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
				// cout << "source from stage " << all_source_fields[0].data << " is " << arg->getId() << ":" << arg->getName() << endl;
			}

			// add the link to the sink stage
			sink_stg->addInput(arg->getId());
		} 
		// else {
		// 	cout << "workflow argument sink: " << all_sink_fields[0].data << endl;
		// }

		// consumes the datalink ending
		while (string(line).find(de) == string::npos && get_line(&line, workflow_descriptor) != -1);
		// cout << "datalink end" << line << endl;
	}
}

/***************************************************************/
/**************** Argument expansion functions *****************/
/***************************************************************/

// returns by value the list of expanded parameters sets. each parameter set
// an be sent to the workflow to be executed. a parameter set is a map of aguments with
// a possible value for that argument (parameter). 
list<map<int, ArgumentBase*>> expand_parameters_combinations(map<int, list<ArgumentBase*>> parameters_values, 
	map<int, ArgumentBase*> workflow_inputs) {

	list<map<int, ArgumentBase*>> output;

	list<ArgumentBase*> values_head = map_pop(parameters_values);
	ArgumentBase* input_head = map_pop(workflow_inputs);

	// halting condition
	if (parameters_values.size() == 0) {
		// cout << "halting on " << input_head->getName() << endl;
		for (ArgumentBase* a : values_head) {
			map<int, ArgumentBase*> inputs;
			ArgumentBase* input = a->clone();
			input->setName(input_head->getName());
			input->setId(new_uid());
			inputs[input->getId()] = input;
			output.emplace_back(inputs);
		}

		return output;
	}
	
	// cout << "recurring on " << endl;
	// mapprint(parameters_values);
	// cout << endl;
	list<map<int, ArgumentBase*>> next = expand_parameters_combinations(parameters_values, workflow_inputs);
	for (ArgumentBase* a : values_head) {
		// list<map<int, ArgumentBase*>> next_copy = copt(next);
		ArgumentBase* input_copy = a->clone();
		input_copy->setName(input_head->getName());
		input_copy->setId(new_uid());

		for (map<int, ArgumentBase*> p : next) {
			map<int, ArgumentBase*> updated_arguments = cpy_ab_map(p);
			updated_arguments[input_copy->getId()] = input_copy;
			output.emplace_back(updated_arguments);
		}
		
	}

	return output;
}

// returns by reference a list of copied outputs.
void expand_arguments(map<int, ArgumentBase*> &ref_arguments, int copies, 
	list<map<int, ArgumentBase*>> &copy_arguments) {

	for (int i=0; i<copies; i++) {
		map<int, ArgumentBase*> cpy = cpy_ab_map(ref_arguments);
		copy_arguments.emplace_back(cpy);
	}
}

void expand_stages(map<int, PipelineComponentBase*> &base_stages, int copies, 
	list<map<int, PipelineComponentBase*>> &all_stages) {

	for (int i=0; i<copies; i++) {
		map<int, PipelineComponentBase*> cpy;
		for (pair<int, PipelineComponentBase*> pcb : base_stages) {
			// clone basic info
			PipelineComponentBase* pcb_cpy = pcb.second->clone();
			
			// set name and id
			pcb_cpy->setName(pcb.second->getName());
			pcb_cpy->setId(new_uid());

			// copy input list
			for (int inp : pcb.second->getInputs())
				pcb_cpy->addInput(inp);

			// copy output list
			for (int out : pcb.second->getOutputs())
				pcb_cpy->addOutput(out);

			cpy[pcb_cpy->getId()] = pcb_cpy;
		}

		all_stages.emplace_back(cpy);
	}
}

/***************************************************************/
/********* Workflow merging and preparation functions **********/
/***************************************************************/

void iterative_full_merging(list<map<int, ArgumentBase*>> &all_inputs, 
	list<map<int, ArgumentBase*>> &all_outputs, 
	list<map<int, PipelineComponentBase*>> &all_stages, 
	list<map<int, ArgumentBase*>> &all_interstage_arguments, 
	map<int, ArgumentBase*> &interstage_arguments_ref, 
	map<int, ArgumentBase*> &merged_arguments, 
	map<int, ArgumentBase*> &merged_outputs,
	map<int, PipelineComponentBase*> &merged_stages) {

	list<map<int, ArgumentBase*>>::iterator inputs_it = all_inputs.begin();
	list<map<int, ArgumentBase*>>::iterator outputs_it = all_outputs.begin();
	list<map<int, ArgumentBase*>>::iterator interstage_arguments_it = all_interstage_arguments.begin();

	int i=0;
	for (map<int, PipelineComponentBase*> stages : all_stages) {
		// cout << "starting stage " << i << endl;
		int io_offset = inputs_it->begin()->first - 1;
		int input_lim = inputs_it->size();
		int output_lim = inputs_it->size() + outputs_it->size();

		// cout << "io_offset " << io_offset << endl;
		// cout << "input_lim " << input_lim << endl;
		// cout << "output_lim " << output_lim << endl;

		for (pair<int, PipelineComponentBase*> stage : stages) {
			// cout << "\tstage " << stage.first << ":" << stage.second->getName() << endl;
			// replace all inputs and outputs ids
			for (int inp : stage.second->getInputs()) {
				if (inp <= input_lim) {
					// cout << "\t\treg_input" << endl;
					stage.second->replaceInput(inp, inp+io_offset);
					// cout << "\t\tinput " << inp << " swaped with " << inp+io_offset << endl;
				} else {
					// cout << "\t\targ_input" << endl;
					int new_inp = find_id_by_name(interstage_arguments_ref[inp]->getName(), 
						*interstage_arguments_it);
					stage.second->replaceInput(inp, new_inp);
					// cout << "\t\tisa_input " << inp << new_inp << endl;
				}
			}
			for (int out : stage.second->getOutputs()) {
				if (out <= output_lim) {
					stage.second->replaceOutput(out, out+io_offset);
				} else {
					int new_inp = find_id_by_name(interstage_arguments_ref[out]->getName(), 
						*interstage_arguments_it);
					stage.second->replaceOutput(out, new_inp);
				}
			}
		}
		inputs_it++;
		outputs_it++;
		interstage_arguments_it++;
	}

	// merge all inputs, outputs and interstage arguments into one
	for (map<int, ArgumentBase*> m : all_inputs) 
		for (pair<int, ArgumentBase*> p : m)
			merged_arguments[p.first] = p.second;
	for (map<int, ArgumentBase*> m : all_outputs) {
		for (pair<int, ArgumentBase*> p : m) {
			merged_arguments[p.first] = p.second;
			merged_outputs[p.first] = p.second;
		}
	}
	for (map<int, ArgumentBase*> m : all_interstage_arguments) 
		for (pair<int, ArgumentBase*> p : m)
			merged_arguments[p.first] = p.second;


	// merged_arguments

	// bool changes = true;
	// while (changes) {
	// 	changes = false;

	// 	// 
	// }

	merged_stages = all_stages.front();
}

void add_arguments_to_stages(map<int, PipelineComponentBase*> &merged_stages, 
	map<int, ArgumentBase*> &merged_arguments,
	map<int, list<int>> &deps,
	RegionTemplateCollection *rts) {

	int i=0;
	for (pair<int, PipelineComponentBase*> stage : merged_stages) {
		// add arguments to stage, adding them as RT as needed
		for (int arg_id : stage.second->getInputs()) {
			stage.second->addArgument(merged_arguments[arg_id]);
			if (merged_arguments[arg_id]->getType() == ArgumentBase::STRING)
				cout << "RT : " << merged_arguments[arg_id]->getName() << endl;
				// stage.second->addRegionTemplateInstance(rts->getRT(i), rts->getRT(i++)->getName());
		}

		// add dependencies
		for (int d : deps[stage.second->getId()])
			((RTPipelineComponentBase*)stage.second)->addDependency(d);
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
	regex r ("<" + field + ">[\"\\:\\w {},.\\[\\]-]+<\\/" + field + ">");

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

list<string> get_workflow_input_arguments(FILE* workflow, string entry_type) {
	list<ArgumentBase*> null;
	list<string> inputs;
	get_workflow_arguments(workflow, entry_type, null, inputs, false);
	return inputs;
}

list<ArgumentBase*> get_workflow_output_arguments(FILE* workflow, string entry_type) {
	list<ArgumentBase*> outputs;
	list<string> null;
	get_workflow_arguments(workflow, entry_type, outputs, null, true);
	return outputs;
}

void get_workflow_arguments(FILE* workflow, 
	string entry_type, 
	list<ArgumentBase*> &output_arguments, 
	list<string> &input_arguments, 
	bool is_output) {

	char *line = NULL;
	size_t len = 0;

	// initial ports section beginning and end
	string ie("<" + entry_type + ">");
	string iee("</" + entry_type + ">");
	
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

		// generate an argument if it's an output
		if (is_output) {
			string type = get_workflow_field(workflow, "path");
			ArgumentBase* arg = new_typed_arg_base(type);
			arg->setName(name);
			arg->setId(new_uid());
			output_arguments.emplace_back(arg);
		} else
			input_arguments.emplace_back(name);

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

ArgumentBase* find_argument(map<int, ArgumentBase*> arguments, string name) {
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
		default:
			return parsing::error;
	}
}

map<int, ArgumentBase*> cpy_ab_map(map<int, ArgumentBase*> &ref) {
	map<int, ArgumentBase*> cpy;
	for (pair<int, ArgumentBase*> p : ref) {
		ArgumentBase* ab_cpy = p.second->clone();
		ab_cpy->setName(p.second->getName());
		ab_cpy->setId(new_uid());
		cpy[ab_cpy->getId()] = ab_cpy;
	}
	return cpy;
}

template <class T>
T map_pop(map<int, T> &m) {
	typename map<int, T>::iterator i = m.begin();
	T val = i->second;
	m.erase(i);
	return val;
}

int find_id_by_name(string name, map<int, ArgumentBase*> &ref) {
	for (pair<int, ArgumentBase*> p : ref) {
		if (name.compare(p.second->getName()) == 0)
			return p.first;
	}
	return 0;
}