/*
 * payload.h
 *
 *  Created on: Jan 14, 2017
 *      Author: eroach
 */

#ifndef SRC_PAYLOAD_H_
#define SRC_PAYLOAD_H_
#include <SD.h>
class Payload {
private:
	char* filename;
	char* magic;
	char* extension;
	bool returns;
	Payload(); //only used internally
public:

	virtual ~Payload();
	Payload(const char* filename);
	Payload(const char* filename, const char* comport);
	void setmagic(const char* magic);
	const char* getmagic();
	void setext(const char* ext);
	const char* getext();
	void fire();
	static Payload open(const char* filename);
	bool doesreturn();
	void doesreturn(bool returns); //set whether the payload returns data
};

#endif /* SRC_PAYLOAD_H_ */
