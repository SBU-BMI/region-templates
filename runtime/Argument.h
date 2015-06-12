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
class ArgumentBase {
protected:
	int type;

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

	static const int INPUT = 0;
	static const int OUTPUT = 0;
	static const int INPUT_OUTPUT = 0;

	//static const int CHAR = 1;
	static const int INT = 2;
	static const int STRING = 3;
	static const int FLOAT = 4;
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

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
    ArgumentFloat getArgValue(int index);
    void addArgValue(ArgumentFloat arg_value);
    int getNumArguments();
};

#endif /* ARGUMENT_H_ */
