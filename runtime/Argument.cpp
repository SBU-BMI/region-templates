/*
 * Argument.cpp
 *
 *  Created on: Feb 16, 2012
 *      Author: george
 */

#include "Argument.h"
#include <sstream>

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

ArgumentBase* ArgumentString::clone() {
	ArgumentString* retValue = new ArgumentString();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
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

ArgumentInt::ArgumentInt() : ArgumentBase(ArgumentBase::INT) {
	this->arg_value = 0;
}

ArgumentInt::ArgumentInt(int value) : ArgumentBase(ArgumentBase::INT) {
	this->arg_value = value;
}

ArgumentInt::~ArgumentInt() {
}

int ArgumentInt::size() {
	// space use by the ArgumentBase class
	int arg_size = ArgumentBase::size();
	arg_size+=sizeof(int);
	return arg_size;
}

int ArgumentInt::serialize(char* buff) {
	int serialized_bytes = 0;

	// serialize data from ArgumentBase class
	serialized_bytes+=ArgumentBase::serialize(buff);

	int dataItem = this->getArgValue();
	// pack data
	memcpy(buff+serialized_bytes, &dataItem, sizeof(int));
	serialized_bytes+=sizeof(int);
	return serialized_bytes;
}

int ArgumentInt::deserialize(char* buff) {
	int deserialized_bytes = 0;
	// fill up Argument class data
	deserialized_bytes +=ArgumentBase::deserialize(buff);

	int dataItem;
	memcpy(&dataItem, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes+= sizeof(int);

	this->setArgValue(dataItem);

	return deserialized_bytes;
}

int ArgumentInt::getArgValue() const {
	return this->arg_value;
}

ArgumentBase* ArgumentInt::clone() {
	ArgumentInt* retValue = new ArgumentInt();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

void ArgumentInt::setArgValue(int arg_value) {
	this->arg_value = arg_value;
}

std::string ArgumentInt::toString() {
	std::ostringstream convert;
	convert << arg_value;
	return convert.str();
}

ArgumentIntArray::ArgumentIntArray() : ArgumentBase(ArgumentBase::INT_ARRAY) {
	this->array_size = 0;
}

// Makes a hard-copy opf array
ArgumentIntArray::ArgumentIntArray(int* array, int size) : ArgumentBase(ArgumentBase::INT_ARRAY) {
	this->arg_value = new int[size];
	for (int i=0; i<size; i++)
		this->arg_value[i] = array[i];
	this->array_size = size;
}

ArgumentIntArray::~ArgumentIntArray() {
	delete[] this->arg_value;
}

ArgumentBase* ArgumentIntArray::clone() {
	ArgumentIntArray* retValue = new ArgumentIntArray(
		this->arg_value, this->array_size);
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

std::string ArgumentIntArray::toString() {
	std::string output;
	output = "[" + std::to_string(this->arg_value[0]);
	for (int i=1; i<this->array_size; i++)
		output += ", " + std::to_string(this->arg_value[i]);
	output += "]\n";
	return output;
}

int ArgumentIntArray::size() {
	int size = ArgumentBase::size();

	// array size variable
	size += sizeof(int);

	// array variables itself
	size += this->array_size * sizeof(int);

	return size;
}

int ArgumentIntArray::serialize(char *buff) {
	int serialized_bytes = ArgumentBase::serialize(buff);

	// array size variable
	memcpy(buff+serialized_bytes, &this->array_size, sizeof(int));
	serialized_bytes += sizeof(int);

	// array variables itself
	for (int i=0; i<this->array_size; i++) {
		memcpy(buff+serialized_bytes, this->arg_value+i, sizeof(int));
		serialized_bytes += sizeof(int);
	}

	return serialized_bytes;
}

int ArgumentIntArray::deserialize(char *buff) {
	int deserialized_bytes = ArgumentBase::deserialize(buff);

	// array size variable
	memcpy(&this->array_size, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes += sizeof(int);

	// array variables itself
	this->arg_value = new int[this->array_size];
	for (int i=0; i<this->array_size; i++) {
		memcpy(this->arg_value+i, buff+deserialized_bytes, sizeof(int));
		deserialized_bytes += sizeof(int);
	}

	return deserialized_bytes;
}

int ArgumentIntArray::getArgValue(int index) {
	return this->arg_value[index];
}

int* ArgumentIntArray::getArgValue() {
	return this->arg_value;
}

int ArgumentIntArray::getNumArguments() {
	return this->array_size;
}

ArgumentFloat::ArgumentFloat() : ArgumentBase(ArgumentBase::FLOAT){
	this->arg_value = 0.0;
}

ArgumentFloat::ArgumentFloat(float value) : ArgumentBase(ArgumentBase::FLOAT) {
	this->arg_value = value;
}

ArgumentFloat::~ArgumentFloat() {
}

int ArgumentFloat::size() {
	// space use by the ArgumentBase class
	int arg_size = ArgumentBase::size();
	arg_size+=sizeof(float);
	return arg_size;
}

int ArgumentFloat::serialize(char* buff) {
	int serialized_bytes = 0;

	// serialize data from ArgumentBase class
	serialized_bytes+=ArgumentBase::serialize(buff);

	float dataItem = this->getArgValue();
	// pack data
	memcpy(buff+serialized_bytes, &dataItem, sizeof(float));
	serialized_bytes+=sizeof(float);
	return serialized_bytes;
}

int ArgumentFloat::deserialize(char* buff) {
	int deserialized_bytes = 0;
	// fill up Argument class data
	deserialized_bytes +=ArgumentBase::deserialize(buff);

	float dataItem;
	memcpy(&dataItem, buff+deserialized_bytes, sizeof(float));
	deserialized_bytes+= sizeof(float);

	this->setArgValue(dataItem);

	return deserialized_bytes;
}

float ArgumentFloat::getArgValue() const {
	return this->arg_value;
}

ArgumentBase* ArgumentFloat::clone() {
	ArgumentFloat* retValue = new ArgumentFloat();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

void ArgumentFloat::setArgValue(float arg_value) {
	this->arg_value = arg_value;
}

std::string ArgumentFloat::toString() {
	std::ostringstream convert;
	convert << arg_value;
	return convert.str();
}

ArgumentFloatArray::ArgumentFloatArray() : ArgumentBase(ArgumentBase::FLOAT_ARRAY){
}

ArgumentFloatArray::ArgumentFloatArray(ArgumentFloat value) : ArgumentBase(ArgumentBase::FLOAT_ARRAY) {
	this->addArgValue(value);
}

ArgumentFloatArray::~ArgumentFloatArray() {
}

int ArgumentFloatArray::size() {
	// space use by the ArgumentBase class
	int arg_size = ArgumentBase::size();

	// number of arguments
	arg_size+=sizeof(int);

	// size of individual arguments
	for(int i = 0; i < this->getNumArguments(); i++){
		arg_size += this->getArgValue(i).size();
	}
	return arg_size;

}

int ArgumentFloatArray::serialize(char* buff) {
	int serialized_bytes = 0;

	// serialize data from ArgumentBase class
	serialized_bytes+=ArgumentBase::serialize(buff);

	// number of elements in the array
	int numArguments = this->getNumArguments();

	memcpy(buff+serialized_bytes, &numArguments, sizeof(int));
	serialized_bytes+=sizeof(int);
	for(int i = 0; i < this->getNumArguments(); i++){
		serialized_bytes += this->getArgValue(i).serialize(buff+serialized_bytes);
	}

	return serialized_bytes;
}

int ArgumentFloatArray::deserialize(char* buff) {
	int deserialized_bytes = 0;
	// fill up Argument class data
	deserialized_bytes +=ArgumentBase::deserialize(buff);


	int numElements = 0;
	memcpy(&numElements, buff+deserialized_bytes, sizeof(int));
	deserialized_bytes+= sizeof(int);

	for(int i = 0; i < numElements; i++){
		ArgumentFloat aux;
		deserialized_bytes += aux.deserialize(buff+deserialized_bytes);
		this->addArgValue(aux);
	}

	return deserialized_bytes;
}

ArgumentFloat ArgumentFloatArray::getArgValue(int index) {
	// number of elements in the array

	if(index < this->getNumArguments()){
		return this->arg_value[index];
	}
}

float* ArgumentFloatArray::getArgValue() {
	float* val = new float[this->arg_value.size()];
	for (int i=0; i<this->arg_value.size(); i++) {
		val[i] = this->arg_value[i].getArgValue();
	}
	return val;
}

void ArgumentFloatArray::addArgValue(ArgumentFloat arg_value) {
	this->arg_value.push_back(arg_value);
}

int ArgumentFloatArray::getNumArguments() {
	return this->arg_value.size();
}

ArgumentBase* ArgumentFloatArray::clone() {
	ArgumentFloatArray* retValue = new ArgumentFloatArray();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

std::string ArgumentFloatArray::toString() {
	std::string out;

	if (arg_value.size() == 0)
		return "";

	out += "[" + arg_value[0].toString();
	for (int i=1; i<arg_value.size(); i++) {
		out += ", " + arg_value[i].toString();
	}
	out += "]";
	return out;
}

ArgumentRT::ArgumentRT() : path(""), ArgumentBase(ArgumentBase::RT){}

ArgumentRT::ArgumentRT(std::string path) : ArgumentBase(ArgumentBase::RT) {
	this->setArgValue(path);
}

ArgumentRT::~ArgumentRT() {}


std::string ArgumentRT::getArgValue() const {
    return path;
}

void ArgumentRT::setArgValue(std::string path) {
    this->path = path;
}

int ArgumentRT::serialize(char *buff) {
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

int ArgumentRT::size() {
	// space use by the Argument class
	int arg_size = ArgumentBase::size();

	// used to store the size of the string stored
	arg_size+=sizeof(int);

	// the actual size of the string
	arg_size+=sizeof(char) * this->getArgValue().size();

	return arg_size;
}

ArgumentBase* ArgumentRT::clone() {
	ArgumentRT* retValue = new ArgumentRT();
	int size = this->size();
	char *buff = new char[size];
	this->serialize(buff);
	retValue->deserialize(buff);
	delete buff;
	return retValue;
}

int ArgumentRT::deserialize(char *buff) {
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
