/*
 * Argument.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "Argument.h"

ArgumentBase::ArgumentBase(int type) {
	this->type=type;
}

int ArgumentBase::size()
{
	return (sizeof(int));
}

int ArgumentBase::serialize(char *buff)
{
	((int*)buff)[0] = this->getType();
	return ArgumentBase::size();
}

int ArgumentBase::deserialize(char *buff)
{
	this->setType(((int*)buff)[0]);
	return ArgumentBase::size();
}

void ArgumentBase::setType(int type)
{
    this->type = type;
}

int ArgumentBase::getType() const
{
    return type;
}


ArgumentBase::~ArgumentBase() {

}

ArgumentString::ArgumentString(): ArgumentBase(ArgumentBase::STRING)
{

}

ArgumentString::ArgumentString(std::string value): ArgumentBase(ArgumentBase::STRING)
{
	this->setArgValue(value);
}

ArgumentString::~ArgumentString()
{
}


std::string ArgumentString::getArgValue() const
{
    return arg_value;
}

void ArgumentString::setArgValue(std::string arg_value)
{
    this->arg_value = arg_value;
}

int ArgumentString::serialize(char *buff)
{
	int serialized_bytes = 0;

	// serialize data from ArgumentBase class
	serialized_bytes+=ArgumentBase::serialize(buff);

	// pack the size of the string
	int string_size = this->getArgValue().size();
	memcpy(buff+serialized_bytes, &string_size, sizeof(int));
	serialized_bytes+=sizeof(int);

	// serialize the string itself
	memcpy(buff+serialized_bytes, this->getArgValue().c_str(), this->getArgValue().size()*sizeof(char));
	serialized_bytes+=this->getArgValue().size()*sizeof(char);

	return serialized_bytes;
}

int ArgumentString::size()
{
	// space use by the Argument class
	int arg_size = ArgumentBase::size();

	// used to store the size of the string stored
	arg_size+=sizeof(int);

	// the actual size of the string
	arg_size+=sizeof(char) * this->getArgValue().size();

	return arg_size;
}

int ArgumentString::deserialize(char *buff)
{
	int deserialized_bytes = 0;
	// fill up Argument class data
	deserialized_bytes +=ArgumentBase::deserialize(buff);

	// get Size of the string
	int string_size;
	memcpy(&string_size, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes+= sizeof(int);

	// create string to extract data from memory buffer
	char string_value[string_size+1];
	string_value[string_size] = '\0';

	// copy string from message buffer to local variable holding string terminator
	memcpy(string_value, buff+deserialized_bytes, sizeof(char)*string_size);
	deserialized_bytes+=sizeof(char)*string_size;

	// init argument value from string extracted
	this->setArgValue(string_value);

	// return total number of bytes extracted from message
	return deserialized_bytes;
}

template <class T>
T Argument<T>::getArgValue() const
{
	return this->arg_value;
}

template<class T> inline Argument<T>::Argument()
{
}

template<class T> inline Argument<T>::Argument(T value)
{
	this->setArgValue(value);
}

template <class T>
void Argument<T>::setArgValue(T arg_value)
{
	this->arg_value = arg_value;
}

template <class T>
int Argument<T>::size()
{
	// space use by the ArgumentBase class
	int arg_size = ArgumentBase::size();
	arg_size+=sizeof(T);
	return arg_size;
}

template <class T>
int Argument<T>::serialize(char *buff)
{
	int serialized_bytes = 0;

	// serialize data from ArgumentBase class
	serialized_bytes+=ArgumentBase::serialize(buff);

	T dataItem = this->getArgValue();
	// pack data
	memcpy(buff+serialized_bytes, &dataItem, sizeof(T));
	serialized_bytes+=sizeof(T);
	return serialized_bytes;
}

template <class T>
int Argument<T>::deserialize(char *buff)
{
	int deserialized_bytes = 0;
	// fill up Argument class data
	deserialized_bytes +=ArgumentBase::deserialize(buff);

	T dataItem;
	memcpy(&dataItem, buff+deserialized_bytes, sizeof(T));
	deserialized_bytes+= sizeof(T);

	this->setArgValue(dataItem);

	return deserialized_bytes;
}






