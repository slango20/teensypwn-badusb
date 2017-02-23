/*
 * payload.cpp
 *
 *  Created on: Jan 14, 2017
 *      Author: eroach
 */

#include "payload.h"
#include <SD.h>

Payload::Payload() {
	//other constructors init the variables, and this is private
}
Payload::Payload(const char* filename, const char* comport){
	this->filename=(char*)malloc(sizeof(filename));
	this->filename[strlen(filename)]='\0'; //null terminator
	strncpy(this->filename, filename, strlen(filename));
	this->comport=(char*)malloc(sizeof(comport));
	this->comport[strlen(comport)]='\0'; //null terminator
	strncpy(this->comport, filename, strlen(comport));
	this->extension="";
}

Payload::Payload(const char* filename){
	this->filename=(char*)malloc(sizeof(filename));
	this->filename[strlen(filename)]='\0'; //null terminator
	strncpy(this->filename, filename, strlen(filename));
	this->comport=(char*)malloc(6); //COM ports are at most 5 characters, so 6 would be null byte
	this->comport[6]='\0'; //add endstop to catch strcpy
	this->comport[0]='\0'; //make string 0-length
}

Payload::~Payload() {
	// TODO Auto-generated destructor stub
}

void Payload::setComPort(const char* comport){ //if you need to set the COM port after creation
	strncpy(this->comport, comport, 5); //space was alreay allocated for COM port ID, so just copy it there
}

char* Payload::getComPort(){ //allow other stuff to read the COM port;
	return this->comport;
}

void Payload::setmagic(const char* magic){
	this->magic = (char*)magic;
}
const char* Payload::getmagic(){
	return this->magic;
}
void Payload::setext(const char* ext){
	this->extension = (char*)ext;
}
const char* Payload::getext(){
	return this->extension;
}
bool Payload::doesreturn(){
	return this->returns;
}
void Payload::doesreturn(bool returns){
	this->returns=returns;
}

void Payload::fire(void){
	if (comport[0] != '\0'){ //if we have a COM port,
		Keyboard.printf("$comport=%s\n", comport); //feed PS the COM port
	}
	File load = SD.open(this->filename); //it assumes readonly, which we're fine with
	int buffill; //length of data in buffer
	char loadbuf[101];//100 byte buffer with null endstop byte, gcc discards this after function exit
	loadbuf[100]='\0'; //make an endstop so print doesn't overrun the buffer accidentally
	buffill = load.read(&loadbuf, 100); //fill the buffer
	while(buffill>0){ //loop again file if the last read returned any data
		Keyboard.write(loadbuf, buffill); //pass buffer to keyboard, with length of data in buffer
		buffill = load.read(&loadbuf, 100); //does the file have more data for us?
	}
	Keyboard.println(""); //append a last enter in case the file doesn't
	load.close(); //make sure we close the file so SdFat can be used again
}
