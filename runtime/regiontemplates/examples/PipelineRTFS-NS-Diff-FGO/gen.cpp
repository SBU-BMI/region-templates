#include <iostream>
#include <string>
#include <locale>
#include <fstream>

#include "json/json.h"

using namespace std;

string generate_header(Json::Value data);
string generate_source(Json::Value data);
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
			commonVariables += "\t" + data["args"][i]["type"].asString() +
				" " + data["args"][i]["name"].asString() + ";\n";
			commonVariablesNames += data["args"][i]["type"].asString() +
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

	string dataRegionIOs;		// $IO_DR$
	string dataRegionInputCast;	// $INPUT_CAST_DR$
	string dataRegionInputDecl;	// $INPUT_DECL_DR$
	string dataRegionOutput;	// $OUTPUT_DR$
	string commonArgsDec;		// $PCB_ARGS$
	string commonArgsLoop = 	// $PCB_ARGS$
		"\tint set_cout = 0;\n\tfor(int i=0; i<this->getArgumentsSize(); i++){\n";
	string pcb_task_params;		// $PCB_TASK_PARAMS$
	string task_params;			// $TASK_PARAMS$
	string task_args;			// $TASK_ARGS$
	string dr_delete;			// $INPUT_DR_DELETE$
	string input_mat_dr;		// $INPUT_MAT_DR$
	string output_mat_dr;		// $OUTPUT_MAT_DR$
	string output_assign;		// $OUTPUT_DR_ASSIGN$

	// generate DataRegion strings
	for (int i=0; i<data["args"].size(); i++) {
		if (data["args"][i]["type"].asString().compare("dr") == 0) {
			string dr_name = data["args"][i]["name"].asString();

			// generate DataRegion IOs - $IO_DR$
			dataRegionIOs += "\tthis->addInputOutputDataRegion(\"tile\", \"" +
				dr_name + "\", RTPipelineComponentBase::" + 
				uppercase(data["args"][i]["io"].asString()) + ");\n";

			// Generate input strings
			if (data["args"][i]["io"].asString().compare("input") == 0) {
				// generate DataRegion input declaration - $INPUT_DECL_DR$
				dataRegionInputDecl += "\t\tDenseDataRegion2D *" + 
					dr_name + " = NULL;\n";

				// generate DataRegion input cast - $INPUT_CAST_DR$
				dataRegionInputCast += "\t\t\t" + dr_name + 
					" = dynamic_cast<DenseDataRegion2D*>(inputRt->getDataRegion(\"" + 
					dr_name + "\", \"\", 0, dr_id));\n";

				// generate DR cleaning for the destructor - $INPUT_DR_DELETE$
				dr_delete += "\tif(" + dr_name + "_temp != NULL) delete " + dr_name + "_temp;\n";

				// generate cv::Mat variables to hold input DR - $INPUT_MAT_DR$
				input_mat_dr += "\tcv::Mat " + dr_name + " = this->" + 
					dr_name + "_temp->getData();";
			} 
			// generate output string
			else if (data["args"][i]["io"].asString().compare("output") == 0) {
				// generate DataRegion output - $OUTPUT_DR$
				dataRegionOutput += "DenseDataRegion2D *" + dr_name + " = new " + 
					"DenseDataRegion2D();\n\t\t" + dr_name + "->setName(\"" + dr_name + 
					"\");\n\t\t" + dr_name + "->setId(dr_id_c);\n\t\t" + dr_name + 
					"->setVersion(0);\n\t\tinputRt->insertDataRegion(" + dr_name + ");";

				// generate cv::Mat variables to hold input DR - $OUTPUT_MAT_DR$
				output_mat_dr += "\tcv::Mat " + dr_name + " = this->" + 
					dr_name + "_temp->getData();";

				// generate mat assignment to output DR string - $OUTPUT_DR_ASSIGN$
				output_assign += "\tthis->" + dr_name + "_temp->setData(" + dr_name + ");";

			} else {
				cout << "Malformed descriptor." << endl;
				exit(-1);
			}

			// generate DR args $PCB_TASK_PARAMS$
			pcb_task_params += dr_name + ", ";

			// generate DR args $TASK_PARAMS$
			task_params += "DenseDataRegion2D* " + dr_name + "_temp, ";

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

			// generate common args $TASK_PARAMS$
			task_params += getTypeCast(arg_type) + " " + arg_name + ", ";

			// generate DR args $TASK_ARGS$
			task_args += "\tthis->" + arg_name + " = " + arg_name + ";\n";
		}
	}

	// finish generating $PCB_ARGS$
	string commonArgs = commonArgsDec + "\n" + commonArgsLoop + "\t}\n\n" + 
		"\tif (set_cout < this->getArgumentsSize())\n\t\tstd::cout " + 
		"<< __FILE__ << \":\" << __LINE__ <<\" Missing common arguments on " + 
		name + "\" << std::endl;";

	// remove final comma from pcb_task_params
	pcb_task_params.erase(pcb_task_params.length()-2, 2);

	// remove final comma from task_params
	task_params.erase(task_params.length()-2, 2);

	// generate the command invocation string
	string cmd = data["call"].asString();

	/*******************************************************/
	/*******************************************************/
	/*******************************************************/

	// open source_template file
	char* line;
	size_t length=0;
	FILE* source_template = fopen("source_template", "r");

	// concat all source_template lines
	string source;
	while(getline(&line, &length, source_template) != -1)
		source += string(line);

	size_t pos;


	// $NAME$
	replace_multiple_string(source, "$NAME$", name);

	// $IO_DR$
	replace_multiple_string(source, "$IO_DR$", dataRegionIOs);

	// $PCB_ARGS$
	replace_multiple_string(source, "$PCB_ARGS$", commonArgs);

	// $INPUT_CAST_DR$
	replace_multiple_string(source, "$INPUT_CAST_DR$", dataRegionInputCast);

	// $INPUT_DECL_DR$
	replace_multiple_string(source, "$INPUT_DECL_DR$", dataRegionInputDecl);

	// $OUTPUT_DR$
	replace_multiple_string(source, "$OUTPUT_DR$", dataRegionOutput);

	// $PCB_TASK_PARAMS$
	replace_multiple_string(source, "$PCB_TASK_PARAMS$", pcb_task_params);

	// $TASK_PARAMS$
	replace_multiple_string(source, "$TASK_PARAMS$", task_params);

	// $TASK_ARGS$
	replace_multiple_string(source, "$TASK_ARGS$", task_args);

	// $INPUT_DR_DELETE$
	replace_multiple_string(source, "$INPUT_DR_DELETE$", dr_delete);

	// $INPUT_MAT_DR$
	replace_multiple_string(source, "$INPUT_MAT_DR$", input_mat_dr);

	// $OUTPUT_MAT_DR$
	replace_multiple_string(source, "$OUTPUT_MAT_DR$", output_mat_dr);

	// $CMD$
	replace_multiple_string(source, "$CMD$", cmd);

	// $OUTPUT_DR_ASSIGN$
	replace_multiple_string(source, "$OUTPUT_DR_ASSIGN$", output_assign);

	return source;
}

string getTypeCast(string type) {
	if (type.compare("uchar") == 0)
		return "unsigned char";
	else if (type.compare("int") == 0)
		return "int";
	else if (type.compare("double") == 0)
		return "double";
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