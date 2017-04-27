#ifndef PARSING_HPP_
#define PARSING_HPP_

#include <string>
#include <map>
#include <list>
#include <vector>

#include <regex>
#include "json/json.h"

#include "Argument.h"
#include "PipelineComponentBase.h"
#include "fg_reuse/merging.hpp"

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

// Argument parsing functions
int find_arg_pos(string s, int argc, char** argv);

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

// Workflow parsing helper functions
static list<string> line_buffer;
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

#endif