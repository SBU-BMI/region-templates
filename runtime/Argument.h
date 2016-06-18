/*
 * Argument.h
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#ifndef ARGUMENT_H_
#define ARGUMENT_H_

#include <string>
#include <iostream>
#include <string.h>
#include <vector>
#include <string>
 
class ArgumentBase {
protected:
	int type;
	std::string name;

	// this fields are used only on workflow generation
	int parent;
	int id;

public:
	ArgumentBase(){};
	ArgumentBase(int type);
	virtual ~ArgumentBase();

	virtual int serialize(char *buff);
	virtual int deserialize(char *buff);
	virtual int size();

	virtual ArgumentBase* clone() = 0;

	int getType() const;
	void setType(int type);

	void setName(std::string name) {this->name = name;};
	std::string getName(void) const {return name;};

	int getId() {return id;};
	void setId(int id) {this->id = id;};

	int getParent() {return parent;};
	void setParent(int parent) {this->parent = parent;};

	virtual std::string toString() = 0;

	static const int INPUT = 0;
	static const int OUTPUT = 0;
	static const int INPUT_OUTPUT = 0;

	//static const int CHAR = 1;
	static const int INT = 2;
	static const int STRING = 3;
	static const int FLOAT = 4;
	static const int RT = 5;
	static const int FLOAT_ARRAY = 8;

};

class ArgumentString: public ArgumentBase{
private:
	std::string arg_value;

public:
	ArgumentString();
	ArgumentString(std::string value);
	virtual ~ArgumentString();

	virtual ArgumentBase* clone();

	std::string toString() {return arg_value;};

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	std::string getArgValue() const;
	void setArgValue(std::string arg_value);
};

class ArgumentInt: public ArgumentBase{
private:
	int arg_value;
public:
	ArgumentInt();
	ArgumentInt(int value);
	virtual ~ArgumentInt();
	virtual ArgumentBase* clone();

	std::string toString();

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	int getArgValue() const;
	void setArgValue(int arg_value);
};

class ArgumentFloat: public ArgumentBase{
private:
	float arg_value;
public:
	ArgumentFloat();
	ArgumentFloat(float value);
	virtual ~ArgumentFloat();
	virtual ArgumentBase* clone();

	std::string toString();

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	float getArgValue() const;
	void setArgValue(float arg_value);
};

class ArgumentFloatArray: public ArgumentBase{
private:
	std::vector<ArgumentFloat> arg_value;
public:
	ArgumentFloatArray();
	ArgumentFloatArray(ArgumentFloat value);
	virtual ~ArgumentFloatArray();
	virtual ArgumentBase* clone();

	std::string toString();

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	ArgumentFloat getArgValue(int index);
	float* getArgValue();
	void addArgValue(ArgumentFloat arg_value);
	int getNumArguments();
};

class ArgumentRT: public ArgumentBase{
private:
	std::string name;

public:
	ArgumentRT();
	ArgumentRT(std::string name);
	virtual ~ArgumentRT();

	virtual ArgumentBase* clone();

	std::string toString() {return name;};

	bool isFileInput;

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	std::string getArgValue() const;
	void setArgValue(std::string name);
};


#endif /* ARGUMENT_H_ */
