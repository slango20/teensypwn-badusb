/*
 * RawHidTransport.h
 *
 *  Created on: Jan 20, 2017
 *      Author: eroach
 */

#ifndef SRC_RAWHIDTRANSPORT_H_
#define SRC_RAWHIDTRANSPORT_H_
#include <stdint.h>
#include <stdlib.h>

typedef struct { //64 bytes
	char magic[2]; //two bytes are magic
	char buf[60]; //60 bytes of data
	char port[2]; //tells system what "port" was used
} hid_raw_packet;

class RawHidTransport {
public:
	int write(hid_raw_packet* buf);
	int write(char* buf, int len, char magic[2], char port[2]);
	RawHidTransport();
	virtual ~RawHidTransport();
	int RegisterEvent(const void* func, uint16_t port); //register for events when packets with port are recieved
private:
	int _push(hid_raw_packet* pack);
	int _get(hid_raw_packet* pack);
};

extern RawHidTransport hid_trans; //expose a global instance of the HID transport, much like serial
#endif /* SRC_RAWHIDTRANSPORT_H_ */
