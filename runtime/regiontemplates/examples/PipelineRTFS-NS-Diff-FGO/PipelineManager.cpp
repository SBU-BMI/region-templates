
#include "SysEnv.h"
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include "FileUtils.h"
#include "RegionTemplate.h"
#include "RegionTemplateCollection.h"

#include "NormalizationComp.h"
#include "Segmentation.h"
#include "FeatureExtraction.h"
#include "DiffMaskComp.h"
#include "ParameterSet.h"

#include <regex>
#include <list>
#include <vector>

#include <string.h>
#include <stdio.h>

#include "json/json.h"

using namespace std;

typedef struct port_t {
	string name;
	string pipeline_port;
	list<struct task_t*> interfaces;
} port_t;

typedef struct task_t {
	string name;
	list<port_t> inputs;
	list<port_t> outputs;
	string command;
} task_t;

typedef struct {
	string type;
	string data;
} general_field_t;

namespace parsing {

enum port_type_t {
	int_t, string_t, float_t, array_t, error
};

}

string tasks2string(list<task_t> tasks);
string ports2string(list<port_t> ports);
string get_workflow_field(FILE* workflow, string field);
string get_workflow_name(FILE* workflow);
list<string> get_workflow_ports(FILE* workflow, string port_t);
list<ArgumentBase*> get_workflow_inputs(FILE* workflow);
list<string> get_workflow_outputs(FILE* workflow);
list<task_t> get_workflow_tasks(FILE* workflow);
void update_tasks_dependencies(FILE* workflow, list<task_t>* tasks);
vector<general_field_t> get_all_fields(FILE* workflow, string start, string end);
task_t* find_task(list<task_t>* tasks, string name);
port_t* find_port(list<port_t>* ports, string name);
parsing::port_type_t get_port_type(string s);

int get_line(char** line, FILE* f);
list<string> line_buffer;

int main() {

	FILE* workflow = fopen("seg_example.t2flow", "r");

	//------------------------------------------------------------
	// Parse pipeline file
	//------------------------------------------------------------

	string name = get_workflow_name(workflow);
	// cout << "name: " << name << endl << endl;
	list<ArgumentBase*> workflow_inputs = get_workflow_inputs(workflow);
	cout << "inputs:" << endl;
	for(list<ArgumentBase*>::iterator i=workflow_inputs.begin(); i!=workflow_inputs.end(); i++)
		cout << (*i)->getName() << ": " << (*i)->toString() << endl;
	cout << endl;
	list<string> workflow_outputs = get_workflow_outputs(workflow);
	// cout << "outputs:" << endl;
	// for(list<string>::iterator i=workflow_outputs.begin(); i!=workflow_outputs.end(); i++)
	// 	cout << *i << endl;
	// cout << endl;
	list<task_t> all_tasks = get_workflow_tasks(workflow);
	// cout << "get wf" << endl << tasks2string(all_tasks) << endl << endl;
	update_tasks_dependencies(workflow, &all_tasks);
	// cout << "update wf" << endl << tasks2string(all_tasks) << endl << endl;

	fclose(workflow);

	// print all tasks
	cout << tasks2string(all_tasks);

	//------------------------------------------------------------
	// Add tasks to Manager to be executed
	//------------------------------------------------------------



	//------------------------------------------------------------
	// Run pipeline
	//------------------------------------------------------------

	return 0;
}

string tasks2string(list<task_t> tasks) {
	string out = "";
	for(list<task_t>::iterator i = tasks.begin(); i != tasks.end(); i++) {
		out += "Task " + i->name + "\n";
		out += "\tcmd: " + i->command + "\n";
		out += "\tinputs:\n" + ports2string(i->inputs) + "\n";
		out += "\toutputs:\n" + ports2string(i->outputs) + "\n";
	}
	return out;
}

string ports2string(list<port_t> ports) {
	string out = "";
	for(list<port_t>::iterator i = ports.begin(); i != ports.end(); i++) {
		if (i->pipeline_port.length() > 0)
			out += "\t\t" + i->name + " connected to pipeline port " + i->pipeline_port + "\n";
		else {
			out += "\t\t" + i->name + " connected to: \n";
			for(list<task_t*>::iterator j = i->interfaces.begin(); j != i->interfaces.end(); j++) {
				out += "\t\t\t" + (*j)->name + "\n";
			}
		}
	}
	return out;
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

list<ArgumentBase*> get_workflow_inputs(FILE* workflow) {
	list<ArgumentBase*> ports;

	char *line = NULL;
	size_t len = 0;
	string port = "inputPorts";

	// initial ports section beginning and end
	string ip("<" + port + ">");
	string ipe("</" + port + ">");
	
	// ports section beginning and end
	string p("<port>");
	string pe("</port>");

	// argument name, stored before to be consumed when the ArgumentBase type can be set
	string name;

	// go to the initial ports beginning
	while (get_line(&line, workflow) != -1 && string(line).find(ip) == string::npos);
	// cout << "port init begin: " << line << endl;

	// keep getting ports until it reaches the end of initial ports
	while (get_line(&line, workflow) != -1 && string(line).find(ipe) == string::npos) {
		// consumes the port beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow) != -1);
		// cout << "port begin: " << line << endl;

		// finds the name field
		name = get_workflow_name(workflow);
		// cout << "name: " << name << endl;

		// finds the description field
		string description = get_workflow_field(workflow, "text");
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

		// create the propper Argument object for each type case
		ArgumentBase* inp_val;
		switch (get_port_type(data["type"].asString())) {
			case parsing::int_t:
				inp_val = new ArgumentInt(data["value"].asInt());
				break;
			case parsing::string_t:
				inp_val = new ArgumentString(data["value"].asString());
				break;
			case parsing::float_t:
				inp_val = new ArgumentFloat(data["value"].asFloat());
				break;
			case parsing::array_t:
				inp_val = new ArgumentFloatArray();
				for (int i=0; i<data["value"]["value"].size(); i++) {
					ArgumentFloat float_val(data["value"]["value"][i].asFloat()); 
					((ArgumentFloatArray*)inp_val)->addArgValue(float_val);
				}
				break;
			default:
				exit(-4);
		}

		// set inp_val name
		inp_val->setName(name);

		// add input to list
		ports.push_back(inp_val);

		// consumes the port ending
		while (get_line(&line, workflow) != -1 && string(line).find(pe) == string::npos);
			// cout << "not port end: " << line << endl;
		// cout << "port end: " << line << endl;
	}
	return ports;
}	

list<string> get_workflow_outputs(FILE* workflow) {
	return get_workflow_ports(workflow, "outputPorts");
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

list<port_t> get_task_ports(FILE* workflow, string port_type) {
	list<string> ports_names = get_workflow_ports(workflow, port_type);
	list<port_t> ports;
	port_t port;

	// convert the list of port names to a list of port_t
	for (list<string>::iterator i = ports_names.begin(); i != ports_names.end(); i++) {
		port.name = *i;
		ports.push_back(port);
	}
	return ports;
}

list<task_t> get_workflow_tasks(FILE* workflow) {
	list<task_t> tasks;

	char *line = NULL;
	size_t len = 0;

	string ps("<processors>");
	string pse("</processors>");

	string p("<processor>");
	string pe("</processor>");

	// go to the processors beginning
	while (get_line(&line, workflow) != -1 && string(line).find(ps) == string::npos);
		// cout << "not processor init begin: " << line << endl;
	 // cout << "processor init begin: " << line << endl;

	// keep getting single processors until it reaches the end of all processors
	while (get_line(&line, workflow) != -1 && string(line).find(pse) == string::npos) {
		// consumes the processor beginning
		while (string(line).find(p) == string::npos && get_line(&line, workflow) != -1);
		// cout << "processor begin: " << line << endl;

		task_t new_task;
		new_task.name = get_workflow_name(workflow);
		new_task.inputs = get_task_ports(workflow, "inputPorts");
		new_task.outputs = get_task_ports(workflow, "outputPorts");
		
		// get task command
		new_task.command = get_workflow_field(workflow, "command");

		tasks.push_back(new_task);

		// consumes the processor ending
		while (get_line(&line, workflow) != -1 && string(line).find(pe) == string::npos);
			// cout << "not processor end: " << line << endl;
		// cout << "processor end: " << line << endl;
	}

	return tasks;
}

void update_tasks_dependencies(FILE* workflow, list<task_t>* tasks) {
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
	while (get_line(&line, workflow) != -1 && string(line).find(ds) == string::npos);
		// cout << "not datalink init begin" << line << endl;
	// cout << "datalink init begin" << line << endl;

	// keep getting single datalinks until it reaches the end of all datalinks
	while (get_line(&line, workflow) != -1 && string(line).find(dse) == string::npos) {
		// consumes the datalink beginning
		while (string(line).find(d) == string::npos && get_line(&line, workflow) != -1);
		// cout << "datalink begin" << line << endl;

		// get sink field
		vector<general_field_t> all_sink_fields = get_all_fields(workflow, sink, sinke);
		
		// check if it's a pipeine sink instead of a task sink
		bool pipeline_sink = false;
		task_t* sink_task;
		string sink_name;
		string sink_port_name;
		if (all_sink_fields.size() == 1) {
			// cout << "pipeline sink" << endl;
			pipeline_sink = true;
			sink_name = all_sink_fields[0].data;
		} else {
			// cout << "task sink" << endl;
			sink_name = all_sink_fields[0].data;
			sink_port_name = all_sink_fields[1].data;
			sink_task = find_task(tasks, sink_name);
		}

		// get source field
		vector<general_field_t> all_source_fields = get_all_fields(workflow, source, sourcee);

		// check if it's a pipeine source instead of a task source
		bool pipeline_source = false;
		task_t* source_task;
		string source_name;
		string source_port_name;
		if (all_source_fields.size() == 1) {
			// cout << "pipeline source" << endl;
			source_name = all_source_fields[0].data;
			pipeline_source = true;
		} else {
			// cout << "task source" << endl;
			source_name = all_source_fields[0].data;
			source_port_name = all_source_fields[1].data;
			source_task = find_task(tasks, source_name);
		}

		// update source reference if there is one
		port_t* source_port;
		if (!pipeline_source) {
			source_port = find_port(&(source_task->outputs), source_port_name);
			if (pipeline_sink)
				source_port->pipeline_port = sink_name;
			else {
				source_port->pipeline_port = "";
				source_port->interfaces.push_back(sink_task);
			}
		}

		// update input reference if there is one
		port_t* sink_port;
		if (!pipeline_sink) {
			sink_port = find_port(&(sink_task->inputs), sink_port_name);
			if (pipeline_source)
				sink_port->pipeline_port = source_name;
			else {
				sink_port->pipeline_port = "";
				sink_port->interfaces.push_back(source_task);
			}
		}

		// consumes the datalink ending
		while (string(line).find(de) == string::npos && get_line(&line, workflow) != -1);
		// cout << "datalink end" << line << endl;
		
	}

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

task_t* find_task(list<task_t>* tasks, string name) {
	for (list<task_t>::iterator i = tasks->begin(); i != tasks->end(); i++)
		if (i->name.compare(name) == 0)
			return &(*i);

	return NULL;
}

port_t* find_port(list<port_t>* ports, string name) {
	for (list<port_t>::iterator i = ports->begin(); i != ports->end(); i++)
		if (i->name.compare(name) == 0)
			return &(*i);

	return NULL;
}

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

size_t size(list<string> s) {return s.size();}

void print(list<string> s) {
	for(list<string>::iterator i=s.begin();i!=s.end();i++)
		cout << *i << endl;
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
		case str2int("array"):
			return parsing::array_t;
		default:
			return parsing::error;
	}
}

// namespace patch{
// 	template < typename T > std::string to_string(const T& n)
// 	{
// 		std::ostringstream stm;
// 		stm << n;
// 		return stm.str();
// 	}
// }
// void parseInputArguments(int argc, char**argv, std::string &inputFolder){
// 	// Used for parameters parsing
// 	for(int i = 0; i < argc-1; i++){
// 		if(argv[i][0] == '-' && argv[i][1] == 'i'){
// 			inputFolder = argv[i+1];
// 		}
// 	}
// }


// RegionTemplateCollection* RTFromFiles(std::string inputFolderPath){
// 	// Search for input files in folder path
// 	FileUtils fileUtils(".mask.png");
// 	std::vector<std::string> fileList;
// 	fileUtils.traverseDirectoryRecursive(inputFolderPath, fileList);
// 	RegionTemplateCollection* rtCollection = new RegionTemplateCollection();
// 	rtCollection->setName("inputimage");

// 	std::cout << "Input Folder: "<< inputFolderPath <<std::endl;

// 	std::string temp;
// 	// Create one region template instance for each input data file
// 	// (creates representations without instantiating them)
// 	for(int i = 0; i < fileList.size(); i++){

// 		// Create input mask data region
// 		DenseDataRegion2D *ddr2d = new DenseDataRegion2D();
// 		ddr2d->setName("RAW");
// 		std::ostringstream oss;
// 		oss << i;
// 		ddr2d->setId(oss.str());
// 		ddr2d->setInputType(DataSourceType::FILE_SYSTEM);
// 		ddr2d->setIsAppInput(true);
// 		ddr2d->setOutputType(DataSourceType::FILE_SYSTEM);
// 		std::string inputFileName = fileUtils.replaceExt(fileList[i], ".mask.png", ".tiff");
// 		ddr2d->setInputFileName(inputFileName);

// 		// Create reference mask data region
// 		DenseDataRegion2D *ddr2dRefMask = new DenseDataRegion2D();
// 		ddr2dRefMask->setName("REF_MASK");
// 		ddr2dRefMask->setId(oss.str());
// 		ddr2dRefMask->setInputType(DataSourceType::FILE_SYSTEM);
// 		ddr2dRefMask->setIsAppInput(true);
// 		ddr2dRefMask->setOutputType(DataSourceType::FILE_SYSTEM);
// 		ddr2dRefMask->setInputFileName(fileList[i]);

// 	/*	// Create reference mask data region
// 		DenseDataRegion2D *ddr2dRefNorm = new DenseDataRegion2D();
// 		ddr2dRefNorm->setName("NORM");
// 		ddr2dRefNorm->setId(oss.str());
// 		ddr2dRefNorm->setInputType(DataSourceType::FILE_SYSTEM);
// 		ddr2dRefNorm->setIsAppInput(true);
// 		ddr2dRefNorm->setOutputType(DataSourceType::FILE_SYSTEM);
// 		ddr2dRefNorm->setInputFileName("/home/george/workspace/nscale-normalization/build/bin/normalized.tiff");*/

// 		// Adding data regions to region template
// 		RegionTemplate *rt = new RegionTemplate();
// 		rt->setName("tile");
// 		rt->insertDataRegion(ddr2d);
// 		rt->insertDataRegion(ddr2dRefMask);
// 		//rt->insertDataRegion(ddr2dRefNorm);

// 		// Adding region template instance to collection
// 		rtCollection->addRT(rt);
// 	}

// 	return rtCollection;
// }
// void buildParameterSet(ParameterSet &normalization, ParameterSet &segmentation){

// 	std::vector<ArgumentBase*> targetMeanOptions;

// 	// add normalization parameters.
// 	ArgumentFloatArray *targetMeanAux = new ArgumentFloatArray(ArgumentFloat(-0.632356));
// 	targetMeanAux->addArgValue(ArgumentFloat(-0.0516004));
// 	targetMeanAux->addArgValue(ArgumentFloat(0.0376543));
// 	targetMeanOptions.push_back(targetMeanAux);

// 	/*ArgumentFloatArray *targetMeanAux2 = new ArgumentFloatArray(ArgumentFloat(-0.376));
// 	targetMeanAux2->addArgValue(ArgumentFloat(-0.0133));
// 	targetMeanAux2->addArgValue(ArgumentFloat(0.0243));
// 	targetMeanOptions.push_back(targetMeanAux2);*/

// 	normalization.addArguments(targetMeanOptions);

// 	// Blue channel
//     segmentation.addArgument(new ArgumentInt(220));
//     //segmentation.addRangeArguments(200, 240, 50);

// 	// Green channel
//     segmentation.addArgument(new ArgumentInt(220));
//     //segmentation.addRangeArguments(200, 240, 50);
// 	// Red channel
//     segmentation.addArgument(new ArgumentInt(220));
//     //segmentation.addRangeArguments(200, 240, 50);

// 	// T1, T2  Red blood cell detection thresholds
// 	segmentation.addArgument(new ArgumentFloat(5.0));// T1
// 	segmentation.addArgument(new ArgumentFloat(4.0));// T2
// 	/*std::vector<ArgumentBase*> T1, T2;
// 	for(float i = 2.0; i <= 5; i+=0.5){
// 		T1.push_back(new ArgumentFloat(i+0.5));
// 		T2.push_back(new ArgumentFloat(i));
// 	}
// 	parSet.addArguments(T1);
// 	parSet.addArguments(T2);*/

// 	// G1 = 80, G2 = 45	-> Thresholds used to identify peak values after reconstruction. Those peaks
// 	// are preliminary areas in the calculation of the nuclei candidate set
// 	segmentation.addArgument(new ArgumentInt(80));

// 	segmentation.addRangeArguments(45, 70, 40);


// 	// minSize=11, maxSize=1000	-> Thresholds used to filter out preliminary nuclei areas that are not within a given size range  after peak identification
// 	segmentation.addArgument(new ArgumentInt(11));
// 	//parSet.addRangeArguments(10, 30, 5);
// 	segmentation.addArgument(new ArgumentInt(1000));
// 	//parSet.addRangeArguments(900, 1500, 50);


// 	// int minSizePl=30 -> Filter out objects smaller than this value after overlapping objects are separate (watershed+other few operations)
// 	segmentation.addArgument(new ArgumentInt(30));

// 	//int minSizeSeg=21, int maxSizeSeg=1000 -> Perform final threshold on object sizes after objects are identified
// 	segmentation.addArgument(new ArgumentInt(21));
// 	//parSet.addRangeArguments(10, 30, 5);
// 	segmentation.addArgument(new ArgumentInt(1000));
// 	//parSet.addRangeArguments(900, 1500, 50);

// 	// fill holes element 4-8
// 	segmentation.addArgument(new ArgumentInt(4));
// 	//parSet.addRangeArguments(4, 8, 4);

// 	// recon element 4-8
// 	segmentation.addArgument(new ArgumentInt(8));
// 	//parSet.addRangeArguments(4, 8, 4);

// 	// watershed element 4-8
// 	segmentation.addArgument(new ArgumentInt(8));
// 	//parSet.addRangeArguments(4, 8, 4);

// 	return;
// }

// int main (int argc, char **argv){
// 	// Folder when input data images are stored
// 	std::string inputFolderPath;
// 	std::vector<RegionTemplate *> inputRegionTemplates;
// 	RegionTemplateCollection *rtCollection;
// 	std::vector<int> diffComponentIds;
// 	std::map<std::string, double> perfDataBase;


// 	parseInputArguments(argc, argv, inputFolderPath);

// 	// Handler to the distributed execution system environment
// 	SysEnv sysEnv;

// 	// Tell the system which libraries should be used
// 	sysEnv.startupSystem(argc, argv, "libcomponentnsdifffgo.so");

// 	// Create region templates description without instantiating data
// 	rtCollection = RTFromFiles(inputFolderPath);

// 	ParameterSet parSetNormalization, parSetSegmentation;
// 	buildParameterSet(parSetNormalization, parSetSegmentation);

// 	int segCount = 0;
// 	// Build application dependency graph
// 	// Instantiate application dependency graph
// 	for(int i = 0; i < rtCollection->getNumRTs(); i++){

// 		int previousSegCompId = 0;

// 		// each tile computed with a different parameter combination results
// 		// into a region template w/ a different version value
// 		int versionNorm = 0;

// 		parSetNormalization.resetIterator();
// 		parSetSegmentation.resetIterator();

// 		// walk through parameter set and instantiate the computation pipeline for each parameter combination
// 		std::vector<ArgumentBase*> argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();

// 		// while it has not finished w/ all parameter combinations
// 		while((argSetInstanceNorm).size() != 0){
// 			int versionSeg = 0;

// 			DiffMaskComp *diff = NULL;
// 			NormalizationComp *norm = new NormalizationComp();

// 			// normalization parameters
// 			norm->addArgument(new ArgumentInt(versionNorm));
// 			norm->addArgument(argSetInstanceNorm[0]);
// 			norm->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
// 			sysEnv.executeComponent(norm);

// 			std::vector<ArgumentBase*> argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();

// 			while((argSetInstanceSegmentation).size() != 0){

// 				Segmentation *seg = new Segmentation();

// 				// version of the data region read. Each parameter instance in norm creates a output w/ different version
// 				seg->addArgument(new ArgumentInt(versionNorm));

// 				// version of the data region outputed by the segmentation stage
// 				seg->addArgument(new ArgumentInt(versionSeg));

// 				// add remaining (application specific) parameters from the argSegInstance
// 				for(int j = 0; j < 15; j++)
// 					seg->addArgument(argSetInstanceSegmentation[j]);

// 				seg->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
// 				seg->addDependency(norm->getId());

// 				std::cout<< "Creating DiffMask" << std::endl;
//                 //TODO change here
// 				diff = new DiffMaskComp();
//                 //diff->setTask(new PixelCompare());

// 				// version of the data region that will be read. It is created during the segmentation.
// 				diff->addArgument(new ArgumentInt(versionSeg));

// 					// region template name
// 				diff->addRegionTemplateInstance(rtCollection->getRT(i), rtCollection->getRT(i)->getName());
// 				diff->addDependency(previousSegCompId);
// 				diff->addDependency(seg->getId());
// 				// add to the list of diff component ids.
// 				diffComponentIds.push_back(diff->getId());

// 				sysEnv.executeComponent(seg);
// 				sysEnv.executeComponent(diff);

// 				std::cout << "Manager CompId: " << diff->getId() << " fileName: " << rtCollection->getRT(i)->getDataRegion(0)->getInputFileName() << std::endl;
// 				argSetInstanceSegmentation = parSetSegmentation.getNextArgumentSetInstance();
// 				segCount++;
// 				versionSeg++;
// 			}
// 			versionNorm++;
// 			parSetSegmentation.resetIterator();
// 			argSetInstanceNorm = parSetNormalization.getNextArgumentSetInstance();
// 		}
// 	}

// 	// End Creating Dependency Graph
// 	sysEnv.startupExecution();

//     float diff = 0;
//     float secondaryMetric = 0;
// 	for(int i = 0; i < diffComponentIds.size(); i++){
//         char *resultData = sysEnv.getComponentResultData(diffComponentIds[i]);
//         std::cout << "Diff Id: " << diffComponentIds[i] << " resultData -  ";
// 		if(resultData != NULL){
//             std::cout << "size: " << ((int *) resultData)[0] << " Diff: " << ((float *) resultData)[1] <<
//             " Secondary Metric: " << ((float *) resultData)[2] << std::endl;
//             diff += ((float *) resultData)[1];
//             secondaryMetric += ((float *) resultData)[2];
// 		}else{
// 			std::cout << "NULL" << std::endl;
// 		}
// 	}
//     std::cout << "Total diff: " << diff << " Secondary Metric: " << secondaryMetric << std::endl;

// 	// Finalize all processes running and end execution
// 	sysEnv.finalizeSystem();

// 	delete rtCollection;

// 	return 0;
// }



