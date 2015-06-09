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
    virtual void* getArgPtr(){return NULL;};

    int getType() const;
    void setType(int type);

	static const int INPUT = 0;
	static const int OUTPUT = 0;
	static const int INPUT_OUTPUT = 0;

	static const int CHAR = 1;
	static const int INT = 2;
	static const int STRING = 3;

};

class ArgumentString: public ArgumentBase{
private:
	std::string arg_value;

public:
	ArgumentString();
	ArgumentString(std::string value);
	virtual ~ArgumentString();

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
    std::string getArgValue() const;
    void* getArgPtr(){return &arg_value;};
    void setArgValue(std::string arg_value);
};

template <class T>
class Argument: public ArgumentBase{
private:
	T arg_value;
public:
	Argument();
	Argument(T value);
	virtual ~Argument();

	int size();
	int serialize(char *buff);
	int deserialize(char *buff);
    T getArgValue() const;
    void setArgValue(T arg_value);
};


//template class Argument<int>;
//template class Argument<float>;
//template class Argument<double>;

#endif /* ARGUMENT_H_ */
