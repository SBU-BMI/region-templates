#include <iostream>
#include <string>
#include <locale>
#include <fstream>

#include "json/json.h"

using namespace std;

string generate_header(Json::Value data);
string generate_source(Json::Value data);
string generate_tasks(Json::Value data, string &exec_task);
string getTypeCast(string type);
string getArgumentTypeCast(string type);
void replace_multiple_string(string& str, string to_find, string to_replace);
string uppercase(string s);

int main(int argc, char** argv) {
	
	if (argc != 3) {
		cout << "usage: gen -d <descriptor>" << endl;
		return 0;
	}

	string filename = "";
	for (int i = 0; i < argc-1; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'd') {
			filename = argv[i+1];
		}
	}
	if (filename.length() == 0) {
		cout << "usage: gen -d <descriptor>" << endl;
		return 0;
	}

	// open descriptor
	FILE* desc_file = fopen(filename.c_str(), "r");

	// concat all descriptors lines
	string description;
	char* line;
	size_t length=0;
	while(getline(&line, &length, desc_file) != -1)
		description += " " + string(line);

	Json::Reader reader;
	bool wellFormed;
	Json::Value data;

	wellFormed = reader.parse(description, data, false);
	if(!wellFormed) {
		cout << "Failed to parse JSON: " << description << endl << reader.getFormattedErrorMessages() << endl;
		exit(-3);
	}

	fclose(desc_file);

	string name = data["name"].asString();

	string header = generate_header(data);
	std::ofstream header_file(string(name + ".hpp"));
	header_file << header;
	header_file.close();

	string source = generate_source(data);
	std::ofstream source_file(string(name + ".cpp"));
	source_file << source;
	source_file.close();

	// TODO: get descriptor from argv

	// cout << source << endl;
}

string generate_header(Json::Value data) {
	// get name
	string name = data["name"].asString();

	// get includes string
	string includes = data["includes"].asString();

	// generate DataRegion variables string
	string dataRegionVariables;
	string dataRegionVariablesNames;
	for (int i=0; i<data["args"].size(); i++)
		if (data["args"][i]["type"].asString().compare("dr") == 0) {
			dataRegionVariables += "\tDenseDataRegion2D* " + 
				data["args"][i]["name"].asString() + "_temp;\n";
			dataRegionVariablesNames += "DenseDataRegion2D* " + 
				data["args"][i]["name"].asString() + "_temp, ";
		}

	// generate all other variables string
	string commonVariables;
	string commonVariablesNames;
	for (int i=0; i<data["args"].size(); i++)
		if (data["args"][i]["type"].asString().compare("dr") != 0) {
			commonVariables += "\t" + getTypeCast(data["args"][i]["type"].asString()) +
				" " + data["args"][i]["name"].asString() + ";\n";
			commonVariablesNames += getTypeCast(data["args"][i]["type"].asString()) +
				" " + data["args"][i]["name"].asString() + ", ";
		}

	// remove last comma from commonVariablesNames
	size_t pos;
	commonVariablesNames.erase(commonVariablesNames.length()-2, 2);

	// open header file
	char* line;
	size_t length=0;
	FILE* header_template = fopen("header_template", "r");

	// concat all header lines
	string header;
	while(getline(&line, &length, header_template) != -1)
		header += string(line);


	// add includes
	pos = header.find("$INCLUDES$");
	header.erase(pos, 10);
	header.insert(pos, includes);

	// add filename
	replace_multiple_string(header, "$NAME$", name);

	// add DataRegions
	replace_multiple_string(header, "$DR_VARS$", dataRegionVariables);
	replace_multiple_string(header, "$DR_VARS_NAMES$", dataRegionVariablesNames);

	// add other variables
	replace_multiple_string(header, "$COMMON_VARS$", commonVariables);
	replace_multiple_string(header, "$COMMON_VARS_NAMES$", commonVariablesNames);
	

	return header;
}

string generate_source(Json::Value data) {
	// get name
	string name = data["name"].asString();

	string dataRegionOutputCreate;	// $OUTPUT_DR_CREATE$
	string dataRegionInputCreate;	// $INPUT_DR_CREATE$
	string dataRegionOutputCast;	// $OUTPUT_CAST_DR$
	string dataRegionInputCast;		// $INPUT_CAST_DR$
	string dataRegionOutputDecl;	// $OUTPUT_DECL_DR$
	string dataRegionInputDecl;		// $INPUT_DECL_DR$
	string commonArgsDec;			// $PCB_ARGS$
	string commonArgsLoop = 		// $PCB_ARGS$
		"\tint set_cout = 0;\n\tfor(int i=0; i<this->getArgumentsSize(); i++){\n";
	string pcb_task_params;			// $PCB_TASK_PARAMS$
	string task_args;				// $TASK_ARGS$
	string exec_task;

	// generate DataRegion strings
	for (int i=0; i<data["args"].size(); i++) {
		if (data["args"][i]["type"].asString().compare("dr") == 0) {
			string dr_name = data["args"][i]["name"].asString();

			// Generate input strings
			if (data["args"][i]["io"].asString().compare("input") == 0) {
				// generate input dr name declaration - $PCB_ARGS$
				commonArgsDec += "\tArgumentRT* " + dr_name + "_arg;\n";

				// generate input dr name conditional assignment - $PCB_ARGS$
				commonArgsLoop += "\t\tif (this->getArgument(i)->getName().compare(\"" +
					dr_name + "\") == 0) {\n\t\t\t" + dr_name + "_arg = (ArgumentRT*)" + 
					"this->getArgument(i);\n\t\t\tset_cout++;\n\t\t}\n\n";

				// generate DataRegion IOs - $INPUT_DR_CREATE$
				dataRegionInputCreate += "\tthis->addInputOutputDataRegion(\"tile\", " +
					dr_name + "_arg->getName(), RTPipelineComponentBase::INPUT);\n";

				// generate DataRegion input declaration - $INPUT_DECL_DR$
				dataRegionInputDecl += "\t\tDenseDataRegion2D *" + 
					dr_name + " = NULL;\n";

				// generate DataRegion input cast - $INPUT_CAST_DR$
				dataRegionInputCast += "\t\t\t" + dr_name + 
					" = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(" +
					dr_name + "_arg->getName(), " +
					"std::to_string(" + dr_name + "_arg->getId()), 0, " + dr_name + 
					"_arg->getId()));\n";

			} 
			// generate output string
			else if (data["args"][i]["io"].asString().compare("output") == 0) {
				// generate input dr name declaration - $PCB_ARGS$
				commonArgsDec += "\tArgumentRT* " + dr_name + "_arg;\n";

				// generate input dr name conditional assignment - $PCB_ARGS$
				commonArgsLoop += "\t\tif (this->getArgument(i)->getName().compare(\"" +
					dr_name + "\") == 0) {\n\t\t\t" + dr_name + "_arg = (ArgumentRT*)" + 
					"this->getArgument(i);\n\t\t\tset_cout++;\n\t\t}\n\n";

				// generate DataRegion IOs - $OUTPUT_DR_CREATE$
				dataRegionOutputCreate += "\tthis->addInputOutputDataRegion(\"tile\", " +
					dr_name + "_arg->getName(), RTPipelineComponentBase::OUTPUT);\n";

				// generate DataRegion output declaration - $OUTPUT_DECL_DR$
				dataRegionOutputDecl += "\t\tDenseDataRegion2D *" + 
					dr_name + " = NULL;\n";

				// generate DataRegion output cast - $OUTPUT_CAST_DR$
				dataRegionOutputCast += "\t\t\t" + dr_name + 
					" = new DenseDataRegion2D();\n\t\t\t" + dr_name + "->setName(" + 
					dr_name + "_arg->getName());\n\t\t\t" + dr_name + "->setId(std::to_string(" + 
					dr_name + "_arg->getId()));\n\t\t\t" + dr_name + "->setVersion(" + 
					dr_name + "_arg->getId());\n\t\t\tinputRt->insertDataRegion(" + 
					dr_name + ");\n";

			} else {
				cout << "Malformed descriptor." << endl;
				exit(-1);
			}

			// generate DR args $PCB_TASK_PARAMS$
			pcb_task_params += dr_name + ", ";

			// generate DR args $TASK_ARGS$
			task_args += "\tthis->" + dr_name + "_temp = " + dr_name + "_temp;\n";
		}
	}

	// generate all common args string
	for (int i=0; i<data["args"].size(); i++) {
		if (data["args"][i]["type"].asString().compare("dr") != 0) {
			string arg_name = data["args"][i]["name"].asString();
			string arg_type = data["args"][i]["type"].asString();
			
			// generate arg declaration - $PCB_ARGS$
			commonArgsDec += "\t" + getTypeCast(arg_type) + " " + arg_name + ";\n";

			// generate arg conditional assignment - $PCB_ARGS$
			commonArgsLoop += "\t\tif (this->getArgument(i)->getName().compare(\"" +
				arg_name + "\") == 0) {\n\t\t\t" + arg_name + " = (" + getTypeCast(arg_type) + 
				")(" + getArgumentTypeCast(arg_type) + "this->getArgument(i))->getArgValue();\n" + 
				"\t\t\tset_cout++;\n\t\t}\n\n";

			// generate common args $PCB_TASK_PARAMS$
			pcb_task_params += arg_name + ", ";
		}
	}

	// finish generating $PCB_ARGS$
	string commonArgs = commonArgsDec + "\n" + commonArgsLoop + "\t}\n\n" + 
		"\tif (set_cout < this->getArgumentsSize())\n\t\tstd::cout " + 
		"<< __FILE__ << \":\" << __LINE__ <<\" Missing common arguments on " + 
		name + "\" << std::endl;";

	// remove final comma from pcb_task_params
	pcb_task_params.erase(pcb_task_params.length()-2, 2);

	// generate the tasks
	string tasks = generate_tasks(data, exec_task);

	/********************************************************/
	/***************** Generate File String *****************/
	/********************************************************/

	// open source_template file
	char* line;
	size_t length=0;
	FILE* source_template = fopen("source_template", "r");

	// concat all source_template lines
	string source;
	while(getline(&line, &length, source_template) != -1)
		source += string(line);


	// $NAME$
	replace_multiple_string(source, "$NAME$", name);

	// $OUTPUT_DR_CREATE$
	replace_multiple_string(source, "$OUTPUT_DR_CREATE$", dataRegionOutputCreate);

	// $INPUT_DR_CREATE$
	replace_multiple_string(source, "$INPUT_DR_CREATE$", dataRegionInputCreate);

	// $PCB_ARGS$
	replace_multiple_string(source, "$PCB_ARGS$", commonArgs);

	// $OUTPUT_CAST_DR$
	replace_multiple_string(source, "$OUTPUT_CAST_DR$", dataRegionOutputCast);

	// $INPUT_CAST_DR$
	replace_multiple_string(source, "$INPUT_CAST_DR$", dataRegionInputCast);

	// $OUTPUT_DECL_DR$
	replace_multiple_string(source, "$OUTPUT_DECL_DR$", dataRegionOutputDecl);

	// $INPUT_DECL_DR$
	replace_multiple_string(source, "$INPUT_DECL_DR$", dataRegionInputDecl);

	// $PCB_TASK_PARAMS$
	replace_multiple_string(source, "$PCB_TASK_PARAMS$", pcb_task_params);

	// $EXEC_TASK$
	replace_multiple_string(source, "$EXEC_TASK$", exec_task);

	// $TASKS$
	replace_multiple_string(source, "$TASKS$", tasks);

	return source;
}

string generate_tasks(Json::Value data, string &exec_task) {

	string final_source;

	// go through all tasks
	for (int i=0; i<data["tasks"].size(); i++) {
		string name = data["name"].asString() + to_string(i);
		string type_check = "";
		string task_args;
		string input_dr_delete;
		string input_mat_dr;
		string output_mat_dr;
		string cmd = data["tasks"][i]["call"].asString() + "(";
		string call_args;
		string output_dr_return;
		string update_mat_dr;
		string update_ints_args;
		string resolve_deps;
		string reusable_cond;
		string task_size;
		string task_serialize;
		string task_deserialize;
		string task_print;

		if (i != data["tasks"].size()-1) {
			type_check = "\tif (typeid(t) != typeid(Task" + data["name"].asString() + to_string(i+1) + 
				")) {\n\t\tstd::cout << \"[Task" + data["name"].asString() + to_string(i+1) + 
				"] \" << __FILE__ << \":\" << __LINE__ <<\" incompatible tasks.\" << std::endl;\n\t}\n";
		}

		// go through all args
		for (int j=0; j<data["tasks"][i]["args"].size(); j++) {
			call_args += data["tasks"][i]["args"][j]["name"].asString() + ", ";

			if (data["tasks"][i]["args"][j]["type"].asString().compare("dr") == 0) {
				task_args += "\t\tif (a->getName().compare(\"" + 
					data["tasks"][i]["args"][j]["name"].asString() + "\") == 0) {\n\t\t\tArgumentRT* " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_arg;\n\t\t\t" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_arg = (ArgumentRT*)a;\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp = new DenseDataRegion2D();\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setName(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_arg->getName());\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setId(std::to_string(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_arg->getId()));\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setVersion(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_arg->getId());\n\t\t\tset_cout++;\n\t\t}\n\n";

				reusable_cond += "\t\tthis->" + data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName() == t->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName() &&\n\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId() == t->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId() &&\n\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getVersion() == t->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getVersion() &&\n";

				task_size += "\t\tsizeof(int) + " + data["tasks"][i]["args"][j]["name"].asString() + 
					"_temp->getName().length()*sizeof(char) + sizeof(int) +\n";

				task_serialize += "\t// copy " + data["tasks"][i]["args"][j]["name"].asString() + " id\n\tint " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_id = stoi(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId());\n\tmemcpy(buff+serialized_bytes, &" + 
					data["tasks"][i]["args"][j]["name"].asString() + 
					"_id, sizeof(int));\n\tserialized_bytes+=sizeof(int);\n\n\t// copy " + 
					data["tasks"][i]["args"][j]["name"].asString() + " name size\n\tint " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size = " + 
					data["tasks"][i]["args"][j]["name"].asString() + 
					"_temp->getName().length();\n\tmemcpy(buff+serialized_bytes, &" + 
					data["tasks"][i]["args"][j]["name"].asString() + 
					"_name_size, sizeof(int));\n\tserialized_bytes+=sizeof(int);\n\n\t// copy " + 
					data["tasks"][i]["args"][j]["name"].asString() + " name\n\tmemcpy(buff+serialized_bytes, " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName().c_str(), " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size*sizeof(char));\n\tserialized_bytes+=" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size*sizeof(char);\n\n";

				task_deserialize += "\t// create the " + data["tasks"][i]["args"][j]["name"].asString() + "\n\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp = new DenseDataRegion2D();\n\n\t// extract " + 
					data["tasks"][i]["args"][j]["name"].asString() + " id\n\tint " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_id = ((int*)(buff+deserialized_bytes))[0];\n\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setId(to_string(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_id));\n\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setVersion(" + 
					data["tasks"][i]["args"][j]["name"].asString() + 
					"_id);\n\tdeserialized_bytes += sizeof(int);\n\n\t// extract " + 
					data["tasks"][i]["args"][j]["name"].asString() + " name size\n\tint " + 
					data["tasks"][i]["args"][j]["name"].asString() + 
					"_name_size = ((int*)(buff+deserialized_bytes))[0];" + 
					"\n\tdeserialized_bytes += sizeof(int);\n\n\t// copy " + 
					data["tasks"][i]["args"][j]["name"].asString() + " name\n\tchar " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name[" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size+1];\n\t" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name[" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size] = \'\\0\';\n\tmemcpy(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name, buff+deserialized_bytes, sizeof(char)*" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size);\n\tdeserialized_bytes += sizeof(char)*" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name_size;\n\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->setName(" + 
					data["tasks"][i]["args"][j]["name"].asString() + "_name);\n\n";

				if (data["tasks"][i]["args"][j]["io"].asString().compare("input") == 0) {
					input_dr_delete += "\tif(" + data["tasks"][i]["args"][j]["name"].asString() + "_temp != NULL) delete " + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp;\n";

					input_mat_dr += "\tcv::Mat " + data["tasks"][i]["args"][j]["name"].asString() + ";\n";

					update_mat_dr += "\t" + data["tasks"][i]["args"][j]["name"].asString() + 
						"_temp = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName(),\n\t\tthis->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId(), 0, stoi(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId())));\n";
				} else {
					output_mat_dr += "\tcv::Mat " + data["tasks"][i]["args"][j]["name"].asString() + " = this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getData();\n";

					output_dr_return += "\t" + data["tasks"][i]["args"][j]["name"].asString() + 
						" = new cv::Mat(" + data["tasks"][i]["args"][j]["name"].asString() + "_temp);\n";

					update_mat_dr += "rt->insertDataRegion(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp);\n";
				}
			} else {
				task_args += "\t\tif (a->getName().compare(\"" + 
					data["tasks"][i]["args"][j]["name"].asString() + "\") == 0) {\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + " = (" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + ")(" + 
					getArgumentTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + 
					"a)->getArgValue();\n\t\t\tset_cout++;\n\t\t}\n\n";

				reusable_cond += "\t\tthis->" + data["tasks"][i]["args"][j]["name"].asString() + " == t->" + 
					data["tasks"][i]["args"][j]["name"].asString() + " &&\n";

				task_size += "\t\tsizeof(" + getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + 
					") +\n";

				task_serialize += "\t// copy field " + data["tasks"][i]["args"][j]["name"].asString() + 
					"\n\tmemcpy(buff+serialized_bytes, &" + 
					data["tasks"][i]["args"][j]["name"].asString() + ", sizeof(" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + "));\n\tserialized_bytes+=sizeof(" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + ");\n\n";

				task_deserialize += "\t// extract field " + data["tasks"][i]["args"][j]["name"].asString() + 
					"\n\tthis->" + data["tasks"][i]["args"][j]["name"].asString() + " = ((" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + 
					"*)(buff+deserialized_bytes))[0];\n\tdeserialized_bytes += sizeof(" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + ");\n\n";

				task_print += "\tcout << \"\t\t\tminSizePl: \" << " + 
					data["tasks"][i]["args"][j]["name"].asString() + " << endl;\n";
			}
		}

		// go through all interstage args
		for (int j=0; j<data["tasks"][i]["interstage_args"].size(); j++) {
			call_args += data["tasks"][i]["interstage_args"][j]["name"].asString() + ", ";

			if (data["tasks"][i]["interstage_args"][j]["io"].asString().compare("input") == 0) {
				update_ints_args += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
					" = ((Task$NAME$*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";

				resolve_deps += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
					" = &((Task$NAME$*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";
			}
		}

		// remove cmd last comma and add a parentesis
		call_args.erase(call_args.length()-2, 2);
		cmd += call_args;
		cmd += ");";

		// open source_template file
		char* line;
		size_t length=0;
		FILE* source_template = fopen("source_task_template", "r");

		// concat all source_template lines
		string source;
		while(getline(&line, &length, source_template) != -1)
			source += string(line);

		// $TASK_ARGS$
		replace_multiple_string(source, "$TASK_ARGS$", task_args);

		// $INPUT_DR_DELETE$
		replace_multiple_string(source, "$INPUT_DR_DELETE$", input_dr_delete);

		// $INPUT_MAT_DR$
		replace_multiple_string(source, "$INPUT_MAT_DR$", input_mat_dr);

		// $OUTPUT_MAT_DR$
		replace_multiple_string(source, "$OUTPUT_MAT_DR$", output_mat_dr);

		// $CMD$
		replace_multiple_string(source, "$CMD$", cmd);

		// $OUTPUT_DR_RETURN$
		replace_multiple_string(source, "$OUTPUT_DR_RETURN$", output_dr_return);

		// $UPDATE_MAT_DR$
		replace_multiple_string(source, "$UPDATE_MAT_DR$", update_mat_dr);

		// $UPDATE_INTS_ARGS$
		replace_multiple_string(source, "$UPDATE_INTS_ARGS$", update_ints_args);

		// $RESOLVE_DEPS$
		replace_multiple_string(source, "$RESOLVE_DEPS$", resolve_deps);

		// $REUSABLE_COND$
		replace_multiple_string(source, "$REUSABLE_COND$", reusable_cond);

		// $TASK_SIZE$
		replace_multiple_string(source, "$TASK_SIZE$", task_size);

		// $TASK_SERIALIZE$
		replace_multiple_string(source, "$TASK_SERIALIZE$", task_serialize);

		// $TASK_DESERIALIZE$
		replace_multiple_string(source, "$TASK_DESERIALIZE$", task_deserialize);

		// $TASK_PRINT$
		replace_multiple_string(source, "$TASK_PRINT$", task_print);		

		// $NAME$
		replace_multiple_string(source, "$NAME$", name);

		// $TYPE_CHECK$
		replace_multiple_string(source, "$TYPE_CHECK$", type_check);

		final_source += source;

		exec_task += "\t\tTask" + name + " * task" + to_string(i) + " = new Task" + name + 
			"(" + call_args + ");\n\t\tthis->executeTask(task" + to_string(i) + ");\n";
	}

	return final_source;
}

string getTypeCast(string type) {
	if (type.compare("uchar") == 0)
		return "unsigned char";
	else if (type.compare("int") == 0)
		return "int";
	else if (type.compare("double") == 0)
		return "double";
	else if (type.compare("float_array") == 0)
		return "float*";
	else
		return nullptr;
}

string getArgumentTypeCast(string type) {
	if (type.compare("uchar") == 0)
		return "(ArgumentInt*)";
	else if (type.compare("int") == 0)
		return "(ArgumentInt*)";
	else if (type.compare("double") == 0)
		return "(ArgumentFloat*)";
	else if (type.compare("float_array") == 0)
		return "(ArgumentFloatArray*)";
	else
		return nullptr;
}

void replace_multiple_string(string& str, string to_find, string to_replace) {
	size_t pos;
	while ((pos = str.find(to_find)) != string::npos) {
		str.erase(pos, to_find.length());
		str.insert(pos, to_replace);
	}
}

string uppercase(string s) {
	for(int i=0;s[i]!='\0';i++){
		s[i]=toupper(s[i]);
	}
	return s;
}