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
	int id;

	// this fields are used only on workflow generation
	int parent;

public:
	enum io_type {input, output};

	ArgumentBase() : io(ArgumentBase::input) {};
	ArgumentBase(int type);
	virtual ~ArgumentBase();

	virtual int serialize(char *buff);
	virtual int deserialize(char *buff);
	virtual int size();

	virtual ArgumentBase* clone() = 0;

	int getType() const;
	void setType(int type);

	void setName(std::string name);
	std::string getName(void) const;

	int getId();
	void setId(int id);

	io_type getIo();
	void setIo(io_type io);

	int getParent();
	void setParent(int parent);

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


private:
	// this field is here only because forward enum declaraion isn't allowed before c++0x
	// otherwise, it would be below id.
	// this is necessary since io_type must be a public type
	io_type io;

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
	std::string path;

public:
	ArgumentRT();
	ArgumentRT(std::string path);
	virtual ~ArgumentRT();

	virtual ArgumentBase* clone();

	std::string toString() {return path;};

	bool isFileInput;

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
	std::string getArgValue() const;
	void setArgValue(std::string path);
};


#endif /* ARGUMENT_H_ */
