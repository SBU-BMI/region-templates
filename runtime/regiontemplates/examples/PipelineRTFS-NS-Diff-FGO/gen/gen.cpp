#include <iostream>
#include <string>
#include <locale>
#include <fstream>

#include "json/json.h"

using namespace std;

string generate_header(Json::Value data);
string generate_source(Json::Value data);
string generate_tasks(Json::Value data, string &desc_decl, string &desc_def);
string getTypeCast(string type);
string getArgumentTypeCast(string type);
string getMatDRType(string type);
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

	// string header = generate_header(data);
	// std::ofstream header_file(string(name + ".hpp"));
	// header_file << header;
	// header_file.close();

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
	string stage_desc_decl;
	string stage_desc_def;
	string stage_input_dr;
	string stage_output_dr;

	// generate DataRegion io strings
	for (int i=0; i<data["dr_args"].size(); i++) {
		if (data["dr_args"][i]["io"].asString().compare("input") == 0) {
			stage_input_dr += "\tthis->addInputOutputDataRegion(\"tile\", \"" + 
				data["dr_args"][i]["name"].asString() + "\", RTPipelineComponentBase::INPUT);\n";
		} else {
			stage_output_dr += "\tthis->addInputOutputDataRegion(\"tile\", \"" + 
				data["dr_args"][i]["name"].asString() + "\", RTPipelineComponentBase::OUTPUT);\n";
		}
	}

	// generate the tasks
	string tasks = generate_tasks(data, stage_desc_decl, stage_desc_def);

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

	// $STAGE_DESC_DECL$
	replace_multiple_string(source, "$STAGE_DESC_DECL$", stage_desc_decl);

	// $STAGE_DESC_DEF$
	replace_multiple_string(source, "$STAGE_DESC_DEF$", stage_desc_def);

	// $STAGE_INPUT_DR$
	replace_multiple_string(source, "$STAGE_INPUT_DR$", stage_input_dr);

	// $STAGE_OUTPUT_DR$
	replace_multiple_string(source, "$STAGE_OUTPUT_DR$", stage_output_dr);	

	// $TASKS$
	replace_multiple_string(source, "$TASKS$", tasks);

	return source;
}

string generate_tasks(Json::Value data, string &desc_decl, string &desc_def) {

	string final_source;
	map<string, bool> fisrt_forward;

	// go through all tasks
	for (int i=0; i<data["tasks"].size(); i++) {
		string name = data["name"].asString() + to_string(i);
		string name_prev = data["name"].asString() + to_string(i-1);
		string type_check = "";
		string task_args;
		string input_dr_delete;
		string input_mat_dr;
		string output_mat_dr;
		string intertask_mat;
		string intertask_inst;
		string cmd = data["tasks"][i]["call"].asString() + "(";
		string call_args;
		string output_dr_return;
		string update_mat_dr;
		string update_ints_args;
		string resolve_deps;
		string reusable_cond;
		string task_size = "\t\tsizeof(int) + sizeof(int) +\n";
		string task_serialize;
		string task_deserialize;
		string task_print;

		if (i != data["tasks"].size()-1) {
			type_check = "\tif (typeid(t) != typeid(Task" + data["name"].asString() + to_string(i+1) + 
				")) {\n\t\tstd::cout << \"[Task" + data["name"].asString() + to_string(i+1) + 
				"] \" << __FILE__ << \":\" << __LINE__ <<\" incompatible tasks.\" << std::endl;\n\t}\n";
		}

		desc_decl += "\tlist<ArgumentBase*> task_" + name + "_args;\n";

		// go through all args
		for (int j=0; j<data["tasks"][i]["args"].size(); j++) {
			desc_def += "\t" + getArgumentTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + "* " + 
				data["tasks"][i]["args"][j]["name"].asString() + to_string(i) + " = new " + 
				getArgumentTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + "();\n\t" + 
				data["tasks"][i]["args"][j]["name"].asString() + to_string(i) + "->setName(\"" + 
				data["tasks"][i]["args"][j]["name"].asString() + "\");\n\ttask_" + 
				name + "_args.emplace_back(" + 
				data["tasks"][i]["args"][j]["name"].asString() + to_string(i) + ");\n";

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
					data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName() &&\n";

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
					call_args += data["tasks"][i]["args"][j]["name"].asString() + ", ";

					input_dr_delete += "\tif(" + data["tasks"][i]["args"][j]["name"].asString() + "_temp != NULL) delete " + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp;\n";

					input_mat_dr += "\tcv::Mat " + data["tasks"][i]["args"][j]["name"].asString() + " = this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getData();\n";

					update_mat_dr += "\t" + data["tasks"][i]["args"][j]["name"].asString() + 
						"_temp = dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getName(),\n\t\tthis->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId(), 0, stoi(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp->getId())));\n";
				} else {
					call_args += "&" + data["tasks"][i]["args"][j]["name"].asString() + ", ";

					output_mat_dr += "\tcv::Mat " + data["tasks"][i]["args"][j]["name"].asString() + ";\n";

					output_dr_return += "\tthis->" + data["tasks"][i]["args"][j]["name"].asString() + 
						"_temp->setData(" + data["tasks"][i]["args"][j]["name"].asString() + ");\n";

					update_mat_dr += "rt->insertDataRegion(this->" + 
						data["tasks"][i]["args"][j]["name"].asString() + "_temp);\n";
				}
			} else {
				call_args += data["tasks"][i]["args"][j]["name"].asString() + ", ";

				task_args += "\t\tif (a->getName().compare(\"" + 
					data["tasks"][i]["args"][j]["name"].asString() + "\") == 0) {\n\t\t\tthis->" + 
					data["tasks"][i]["args"][j]["name"].asString() + " = (" + 
					getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + ")((" + 
					getArgumentTypeCast(data["tasks"][i]["args"][j]["type"].asString()) + 
					"*)a)->getArgValue();\n\t\t\tset_cout++;\n\t\t}\n\n";

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

				task_print += "\tcout << \"" + data["tasks"][i]["args"][j]["name"].asString() + ": \" << " + 
					data["tasks"][i]["args"][j]["name"].asString() + " << endl;\n";
			}
		}

		// go through all interstage args
		for (int j=0; j<data["tasks"][i]["interstage_args"].size(); j++) {

			if (data["tasks"][i]["interstage_args"][j]["io"].asString().compare("input") == 0) {
				update_ints_args += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
					" = ((Task$NAME$*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";

				// check if this is the a forwarded argument
				if (fisrt_forward[data["tasks"][i]["interstage_args"][j]["name"].asString()] == true) {
					resolve_deps += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
						" = ((Task" + name_prev + "*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + "_fw;\n";
				} else {
					resolve_deps += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
						" = ((Task" + name_prev + "*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";
				}

				call_args += data["tasks"][i]["interstage_args"][j]["name"].asString() + ", ";
			} else if (data["tasks"][i]["interstage_args"][j]["io"].asString().compare("output") == 0) {
				call_args += data["tasks"][i]["interstage_args"][j]["name"].asString() + ", ";
				fisrt_forward[data["tasks"][i]["interstage_args"][j]["name"].asString()] = false;

				intertask_mat += "\t" + getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) + " " + 
					data["tasks"][i]["interstage_args"][j]["name"].asString() + "_temp;\n";

				intertask_inst += "\t" + data["tasks"][i]["interstage_args"][j]["name"].asString() +
					" = new " + getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) + ";\n";

			} else {
				update_ints_args += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
					"_fw = ((Task$NAME$*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + "_fw;\n";

				// check if this is the first forward
				if (fisrt_forward[data["tasks"][i]["interstage_args"][j]["name"].asString()] == false) {
					fisrt_forward[data["tasks"][i]["interstage_args"][j]["name"].asString()] = true;
					resolve_deps += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
						"_fw = ((Task" + name_prev + "*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";
				} else {
					resolve_deps += "\tthis->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + 
						"_fw = ((Task" + name_prev + "*)t)->" + data["tasks"][i]["interstage_args"][j]["name"].asString() + "_fw;\n";
				}
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

		// $INTERTASK_MAT$
		replace_multiple_string(source, "$INTERTASK_MAT$", intertask_mat);

		// $INTERTASK_INST$
		replace_multiple_string(source, "$INTERTASK_INST$", intertask_inst);

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

		desc_def += "\tthis->tasksDesc[\"Task" + name + "\"] = task_" + name + "_args;\n\n";
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
	else if (type.compare("dr") == 0)
		return "ArgumentRT";
	else
		return nullptr;
}

string getArgumentTypeCast(string type) {
	if (type.compare("uchar") == 0)
		return "ArgumentInt";
	else if (type.compare("int") == 0)
		return "ArgumentInt";
	else if (type.compare("double") == 0)
		return "ArgumentFloat";
	else if (type.compare("float_array") == 0)
		return "ArgumentFloatArray";
	else if (type.compare("dr") == 0)
		return "ArgumentRT";
	else
		return nullptr;
}

string getMatDRType(string type) {
	if (type.compare("mat") == 0)
		return "cv::Mat";
	else if (type.compare("mat_vect") == 0)
		return "std::vector<cv::Mat>";
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