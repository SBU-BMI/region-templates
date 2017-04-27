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

	string header = generate_header(data);
	std::ofstream header_file(string(name + ".hpp"));
	header_file << header;
	header_file.close();

	string source = generate_source(data);
	std::ofstream source_file(string(name + ".cpp"));
	source_file << source;
	source_file.close();

	// cout << source << endl;
}

string generate_header(Json::Value data) {
	// get name
	string name = data["name"].asString();

	// get includes string
	string includes = data["includes"].asString();

	// get all tasks
	string tasks;
	for (int i=0; i<data["tasks"].size(); i++) {
		string task_friend;
		if (i != data["tasks"].size()-1)
			task_friend = "\tfriend class Task" + name + to_string(i+1) + ";";
		
		// set regular args
		string args;
		string dr_args;
		for (int j=0; j<data["tasks"][i]["args"].size(); j++) {
			if (data["tasks"][i]["args"][j]["type"].asString().compare("dr") != 0) {
				args += "\t" + getTypeCast(data["tasks"][i]["args"][j]["type"].asString()) +
					" " + data["tasks"][i]["args"][j]["name"].asString() + ";\n";
			} else {
				dr_args += "\tstd::shared_ptr<DenseDataRegion2D*> " + 
					data["tasks"][i]["args"][j]["name"].asString() + "_temp;\n";
			}
		}

		// set intertask arguments
		string intertask_args;
		for (int j=0; j<data["tasks"][i]["interstage_args"].size(); j++) {
			if (data["tasks"][i]["interstage_args"][j]["io"].asString().compare("forward") == 0) {
				intertask_args += "\tstd::shared_ptr<" + 
					getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) +
					"> " + data["tasks"][i]["interstage_args"][j]["name"].asString() + "_fw;\n";
			} else {
				intertask_args += "\tstd::shared_ptr<" + 
					getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) +
					"> " + data["tasks"][i]["interstage_args"][j]["name"].asString() + ";\n";
			}
		}

		// open task_header_template file
		char* line;
		size_t length=0;
		FILE* task_header_template = fopen("header_task_template", "r");

		// concat all task_header_template lines
		string task_header;
		while(getline(&line, &length, task_header_template) != -1)
			task_header += string(line);

		replace_multiple_string(task_header, "$NAME$", name+to_string(i));
		replace_multiple_string(task_header, "$FRIEND_TASK$", task_friend);
		replace_multiple_string(task_header, "$ARGS$", args);
		replace_multiple_string(task_header, "$INTERTASK_ARGS$", intertask_args);
		replace_multiple_string(task_header, "$DR_ARGS$", dr_args);

		tasks += task_header;

	}

	// open header file
	char* line;
	size_t length=0;
	FILE* header_template = fopen("header_template", "r");

	// concat all header lines
	string header;
	while(getline(&line, &length, header_template) != -1)
		header += string(line);


	// add includes
	replace_multiple_string(header, "$INCLUDES$", includes);

	// add filename
	replace_multiple_string(header, "$NAME$", name);

	// add DataRegions
	replace_multiple_string(header, "$TASKS$", tasks);
	

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
		string task_destr;
		string task_print;

		if (i != 0) {
			type_check = "\tif (dynamic_cast<Task" + data["name"].asString() + 
				to_string(i-1) + "*>(t) == NULL) {\n\t\tstd::cout << \"[Task" + 
				data["name"].asString() + to_string(i) + 
				"] \" << __FILE__ << \":\" << __LINE__ <<\" incompatible tasks: " + 
				"needed \" << typeid(this).name() << \" and got \" << " + 
				"typeid(t).name() << std::endl;\n\t\treturn;\n\t}\n";
		}

		desc_decl += "\tlist<ArgumentBase*> task_" + name + "_args;\n";

		// go through all args
		for (int j=0; j<data["tasks"][i]["args"].size(); j++) {
			string arg = data["tasks"][i]["args"][j]["name"].asString();
			string arg_t = data["tasks"][i]["args"][j]["type"].asString();
			desc_def += "\t" + getArgumentTypeCast(arg_t) + "* " + 
				arg + to_string(i) + " = new " + 
				getArgumentTypeCast(arg_t) + "();\n\t" + 
				arg + to_string(i) + "->setName(\"" + 
				arg + "\");\n\ttask_" + 
				name + "_args.emplace_back(" + 
				arg + to_string(i) + ");\n";

			if (arg_t.compare("dr") == 0) {
				task_args += "\t\tif (a->getName().compare(\"" + arg + 
					"\") == 0) {\n\t\t\tArgumentRT* " + arg + "_arg;\n\t\t\t" + 
					arg + "_arg = (ArgumentRT*)a;\n\t\t\tthis->" + arg + 
					"_temp = std::make_shared<DenseDataRegion2D*>" + 
					"(new DenseDataRegion2D());\n\t\t\t(*this->" + arg + 
					"_temp)->setName(" + arg + "_arg->getName());\n\t\t\t(*this->" + 
					arg + "_temp)->setId(std::to_string(" + arg + 
					"_arg->getId()));\n\t\t\t(*this->" + arg + "_temp)->setVersion(" + 
					arg + "_arg->getId());\n\t\t\tset_cout++;\n\t\t}\n\n";

				reusable_cond += "\t\t(*this->" + arg + "_temp)->getName() == (*t->" + 
					arg + "_temp)->getName() &&\n";

				task_size += "\t\tsizeof(int) + (*this->" + arg + 
					"_temp)->getName().length()*sizeof(char) + sizeof(int) +\n";

				task_serialize += "\t// copy " + arg + " id\n\tint " + arg + 
					"_id = stoi((*" + arg + "_temp)->getId());\n\tmemcpy(" + 
					"buff+serialized_bytes, &" + arg + "_id, sizeof(int));" + 
					"\n\tserialized_bytes+=sizeof(int);\n\n\t// copy " + arg + 
					" name size\n\tint " + arg + "_name_size = (*" + arg + 
					"_temp)->getName().length();\n\tmemcpy(buff+serialized_bytes, &" + 
					arg + "_name_size, sizeof(int));\n\tserialized_bytes+=sizeof(int);" + 
					"\n\n\t// copy " + arg + " name\n\tmemcpy(buff+serialized_bytes, (*" + 
					arg + "_temp)->getName().c_str(), " + arg + 
					"_name_size*sizeof(char));\n\tserialized_bytes+=" + arg + 
					"_name_size*sizeof(char);\n\n";

				task_deserialize += "\t// create the " + arg + "\n\tthis->" + 
					arg + 
					"_temp = std::make_shared<DenseDataRegion2D*>" + 
					"(new DenseDataRegion2D());\n\n\t// extract " + arg + " id\n\tint " + 
					arg + "_id = ((int*)(buff+deserialized_bytes))[0];\n\t(*this->" + 
					arg + "_temp)->setId(to_string(" + arg + "_id));\n\t(*this->" + 
					arg + "_temp)->setVersion(" + arg + "_id);\n\t" + 
					"deserialized_bytes += sizeof(int);\n\n\t// extract " + arg + 
					" name size\n\tint " + arg + "_name_size = ((int*)" + 
					"(buff+deserialized_bytes))[0];\n\tdeserialized_bytes" + 
					" += sizeof(int);\n\n\t// copy " + arg + " name\n\tchar " + 
					arg + "_name[" + arg + "_name_size+1];\n\t" + arg + "_name[" + 
					arg + "_name_size] = \'\\0\';\n\tmemcpy(" + arg + 
					"_name, buff+deserialized_bytes, sizeof(char)*" + arg + 
					"_name_size);\n\tdeserialized_bytes += sizeof(char)*" + 
					arg + "_name_size;\n\t(*this->" + arg + "_temp)->setName(" + 
					arg + "_name);\n\n";

				task_destr += "\tif (" + arg + "_temp.unique() && mock)\n\t\tdelete *" + 
					arg + "_temp;";

				if (data["tasks"][i]["args"][j]["io"].asString().compare("input") == 0) {
					call_args += arg + ", ";

					input_mat_dr += "\tcv::Mat " + arg + " = (*this->" + 
						arg + "_temp)->getData();\n";

					update_mat_dr += "\tstring name_" + arg + "_temp = (*this->" + arg + 
						"_temp)->getName();\n\tstring sid_" + arg + "_temp = (*this->" +
						arg + "_temp)->getId();\n\tint id_" + arg + "_temp = " +
						"stoi((*this->" + arg + "_temp)->getId());\n\tif (" + arg + 
						"_temp != NULL)\n\t\tdelete *" + arg + "_temp;\n\t" + arg +
						"_temp = std::make_shared<DenseDataRegion2D*>(" + 
						"dynamic_cast<DenseDataRegion2D*>(rt->getDataRegion(name_" + 
						arg + "_temp, sid_" + arg + "_temp, 0, id_" + arg + 
						"_temp)));";

				} else {
					call_args += "&" + arg + ", ";

					output_mat_dr += "\tcv::Mat " + arg + ";\n";

					output_dr_return += "\t(*this->" + arg + 
						"_temp)->setData(" + arg + ");\n";

					update_mat_dr += "rt->insertDataRegion(*this->" + 
						arg + "_temp);\n";
				}
			} else {
				call_args += arg + ", ";

				task_args += "\t\tif (a->getName().compare(\"" + 
					arg + "\") == 0) {\n\t\t\tthis->" + 
					arg + " = (" + 
					getTypeCast(arg_t) + ")((" + 
					getArgumentTypeCast(arg_t) + 
					"*)a)->getArgValue();\n\t\t\tset_cout++;\n\t\t}\n\n";

				reusable_cond += "\t\tthis->" + arg + " == t->" + 
					arg + " &&\n";

				task_size += "\t\tsizeof(" + getTypeCast(arg_t) + 
					") +\n";

				task_serialize += "\t// copy field " + arg + 
					"\n\tmemcpy(buff+serialized_bytes, &" + 
					arg + ", sizeof(" + 
					getTypeCast(arg_t) + "));\n\tserialized_bytes+=sizeof(" + 
					getTypeCast(arg_t) + ");\n\n";

				task_deserialize += "\t// extract field " + arg + 
					"\n\tthis->" + arg + " = ((" + 
					getTypeCast(arg_t) + 
					"*)(buff+deserialized_bytes))[0];\n\tdeserialized_bytes += sizeof(" + 
					getTypeCast(arg_t) + ");\n\n";

				task_print += "\tcout << \"" + arg + ": \" << " + 
					arg + " << endl;\n";
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

				call_args += "&*" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ", ";
			} else if (data["tasks"][i]["interstage_args"][j]["io"].asString().compare("output") == 0) {
				call_args += "&*" + data["tasks"][i]["interstage_args"][j]["name"].asString() + ", ";
				fisrt_forward[data["tasks"][i]["interstage_args"][j]["name"].asString()] = false;

				intertask_mat += "\t" + getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) + " " + 
					data["tasks"][i]["interstage_args"][j]["name"].asString() + "_temp;\n";

				intertask_inst += "\t" + data["tasks"][i]["interstage_args"][j]["name"].asString() +
					" = std::shared_ptr<" + getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) +
					">(new " + getMatDRType(data["tasks"][i]["interstage_args"][j]["type"].asString()) + ");\n";

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

		// $TASK_DESTR$
		replace_multiple_string(source, "$TASK_DESTR$", task_destr);

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