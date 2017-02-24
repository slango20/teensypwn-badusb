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
}

Payload::Payload(const char* filename){
	this->filename=(char*)malloc(sizeof(filename));
	this->filename[strlen(filename)]='\0'; //null terminator
	strncpy(this->filename, filename, strlen(filename));
}

Payload::~Payload() {
	// TODO Auto-generated destructor stub
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

void Payload::fire(void){ //fire a payload to stage0
	File load = SD.open(this->filename); //it assumes readonly, which we're fine with
	int buffill; //length of data in buffer
	char loadbuf[65] = {0};//add extra null for print.
	buffill = load.read(loadbuf+2, 62); //fill the buffer with room for the length
	uint16_t flen = (uint16_t)load.size();
	memcpy(loadbuf,(const void*)flen,2); //prepend the size to the first packet so the stager doesn't preemptively fire
	while(buffill>0){ //loop again file if the last read returned any data
		if (buffill < 64){
			for (int i = buffill; i<64; i++){
				loadbuf[i]=0x00;//pad extra bytes
			}
		}
		RawHID.send(loadbuf, 20000); //pass buffer to keyboard, with length of data in buffer
		buffill = load.read(loadbuf, 64); //does the file have more data for us?
	}
	Keyboard.println(""); //append a last enter in case the file doesn't have one
	load.close(); //make sure we close the file so SdFat can be used again
}
