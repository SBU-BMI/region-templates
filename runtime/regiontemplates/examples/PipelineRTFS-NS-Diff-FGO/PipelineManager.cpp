#include <stdio.h>

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <regex>

#include "json/json.h"

#include "Argument.h"
#include "RTPipelineComponentBase.h"

using namespace std;

namespace parsing {

enum port_type_t {
	int_t, string_t, float_t, float_array_t, error
};

}

// global uid for any local(this file) entity
int uid=1;
int new_uid() {return uid++;};

// Workflow parsing functions
void get_inputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_inputs, map<int, list<ArgumentBase*>> &parameters_values);
void get_outputs_from_file(FILE* workflow_descriptor, map<int, ArgumentBase*> &workflow_outputs);
void get_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> base_stages);

// Workflow parsing helper functions
list<string> line_buffer;
int get_line(char** line, FILE* f);
string get_workflow_name(FILE* workflow);
string get_workflow_field(FILE* workflow, string field);
list<string> get_workflow_ports(FILE* workflow, string port);
parsing::port_type_t get_port_type(string s);


int main() {

	// Handler to the distributed execution system environment
	// SysEnv sysEnv;

	// // Tell the system which libraries should be used
	// sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");


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
	map<int, PipelineComponentBase*> base_stages;
	get_stages_from_file(workflow_descriptor, base_stages);

	// connect the stages inputs/outputs and, returning the list of arguments used
	// the stages dependencies are also set here (i.e Task::addDependency())
	// map<int, ArgumentBase> interstage_arguments = connect_stages_from_file(base_stages, workflow_descriptor);

	//------------------------------------------------------------
	// Add create all combinarions of parameters
	//------------------------------------------------------------

	// expand parameter combinations returning a list of parameters sets
	// each set contains exactly one value for every input parameter
	// list<map<int, ArgumentBase>> expanded_parameters = expand_parameters_combinations(parameters_values);

	// generate all inputs copies with the parameters' values
	// list<map<int, ArgumentBase>> all_inputs = expand_inputs(workflow_inputs, expanded_parameters);
	
	// generate outputs, stages and interstage arguments copies with updated uids and correct workflow_id's
	// list<map<int, ArgumentBase>> all_outputs = expand_outputs(workflow_outputs, expanded_parameters.size());
	// OBS: the tasks ids should also be updated (i.e Task::setId())
	// list<map<int, RTPipelineComponentBase>> all_stages = expand_stages(base_stages, expanded_parameters.size());
	// list<map<int, ArgumentBase>> all_interstage_arguments = expand_arguments(interstage_arguments, expanded_parameters.size());

	//------------------------------------------------------------
	// Iterative merging of stages
	//------------------------------------------------------------

	// maybe try to merge the inputs, and then merge stages iteratively
	// again, updating the uids (Task and local id)
	// {map<int, RTPipelineComponentBase> merged_stages,
	// 	map<int, ArgumentBase> merged_outputs,
	// 	map<int, ArgumentBase> merged_arguments} = iterative_full_merging(all_inputs, all_outputs, all_stages);

	//------------------------------------------------------------
	// Add workflows to Manager to be executed
	//------------------------------------------------------------

	// add arguments to each stage
	// list<RTPipelineComponentBase> ready_stages = add_arguments_to_stages(merged_stages, merged_arguments);

	// add all stages to manager
	// for each RTPipelineComponentBase s of ready_stages do
	// 	sysEnv.executeComponent(s);
	// end

	// execute workflows
	// sysEnv.startupExecution();

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
					inp_values.push_back(val);
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
					inp_values.push_back(val);
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
					inp_values.push_back(val);
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
					inp_values.push_back(val);
				}
				// cout << endl;
				break;
			default:
				exit(-4);
		}

		// set inp_arg name
		inp_arg->setName(name);

		// add input argument to map
		int arg_id = new_uid();
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

// returns by reference the map of stages on an uid 'base_stages'
void get_stages_from_file(FILE* workflow_descriptor, map<int, PipelineComponentBase*> base_stages) {
	// list<task_t> tasks;

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
		cout << "name: " << name << endl;

		// get stage command
		string command = get_workflow_field(workflow_descriptor, "command");
		cout << "command: " << command << endl;

		PipelineComponentBase* stage = PipelineComponentBase::ComponentFactory::getComponentFactory(command)();
		stage->setName(name);

		int stg_id = new_uid();
		// setting task id
		stage->setId(stg_id);
		base_stages[stg_id] = stage;

		// consumes the processor ending
		while (get_line(&line, workflow_descriptor) != -1 && string(line).find(pe) == string::npos);
			// cout << "not processor end: " << line << endl;
		// cout << "processor end: " << line << endl;
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
			line_buffer.push_back(sline.substr(0,pos+1));
			sline = sline.substr(pos+1);
		}
		line_buffer.push_back(sline);
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
			return s.substr(s.find("<" + field + ">")+field.length()+2, s.find("</" + field + ">")-s.find("<" + field + ">")-field.length()-2);
		}
	}

	return nullptr;
}

list<string> get_workflow_ports(FILE* workflow, string port) {
	list<string> ports;

	char *line = NULL;
	size_t len = 0;

	// initial ports section beginning and end
	string ip("<" + port + ">");
	string ipe("</" + port + ">");
	
	// ports section beginning and end
	string p("<port>");
	string pe("</port>");

	// go to the initial ports beginning
	while (get_line(&line, workflow) != -1 && string(line).find(ip) == string::npos);
	// cout << "port init begin: " << line << endl;

	// keep getting ports until it reaches the end of initial ports
	while (get_line(&line, workflow) != -1 && string(line).find(ipe) == string::npos) {
		// consumes the port beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow) != -1);
		// cout << "port begin: " << line << endl;

		// finds the name field
		ports.push_back(get_workflow_name(workflow));

		// consumes the port ending
		while (get_line(&line, workflow) != -1 && string(line).find(pe) == string::npos);
			// get_line << "not port end: " << line << endl;
		// cout << "port end: " << line << endl;
	}
	// cout << "port init end: " << line << endl;
	return ports;
}

// taken from: http://stackoverflow.com/questions/16388510/evaluate-a-string-with-a-switch-in-c
constexpr unsigned int str2int(const char* str, int h = 0) {
	return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
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
