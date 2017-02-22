/*
 * RawHidTransport.cpp
 *
 *  Created on: Jan 20, 2017
 *      Author: eroach
 */

//#include "RawHidTransport.h"
//
//struct {
//	void* func;
//	uint16_t port;
//} eventhandle;
//
//struct eventhandle* Events;
//uint16_t Eventc = 0; //keeps track of event array size
//
//RawHidTransport::RawHidTransport() {
//
//}
//
//
//RawHidTransport::~RawHidTransport() {
//	// TODO Auto-generated destructor stub
//}
//
//int RawHidTransport::RegisterEvent(const void* func, uint16_t port){
//	Eventc++; //increment event handle count;
//	realloc(Events, (sizeof(Events)+sizeof(eventhandle))); //expand event array by one
//	eventhandle newhandle = (eventhandle*)malloc(sizeof(eventhandle));
//	newhandle->func = func;
//	newhandle->port = port;
//	Events[Eventc-1]=newhandle; //new handle is already a pointer
//	return Eventc; //return event handle, can be used to remove an event
//}
//
//int RawHidTransport::_push(hid_raw_packet* pack){
//	//TODO: send the packet
//	return -1;
//}
//int RawHidTransport::_get(hid_raw_packet* pack){
//	//TODO: receive packet from USB subsystem
//	return -1; //USB subsystem not online
//}
//
//int RawHidTransport::write(hid_raw_packet* buf){
//	hid_raw_packet* pack = (hid_raw_packet*)malloc(64); //fine gcc, I allocated the struct for you
//	memcpy(pack, buf, 64);
//	int retval = this->_push(pack);
//	free(pack); //_push can't tell we want to free this after use
//	return retval;
//}
