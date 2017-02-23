//TODO: impliment more functionality using USaBUSe code
//TODO: clean indentation using eclipse
//TODO: add packet handler to fire during IRQ, and throw NACKs
//TODO: add proper init.
//TODO: figure out how to bind the protocol to the transport layer
#include "rawtransport.h"
#include <string.h>
using namespace std;


        HIDTransport::HIDTransport(){
            this->buf_head=0; 
            this->buf_tail=0;
            this->hid_buffer = new hid_packet[HID_BUF_SIZE]; //usually set to 16 (1k bytes)
            this->buf_overflow=false;
        }
        bool HIDTransport::push_packet(hid_packet* pack) { //head is next uninitialized data
            if ((buf_head+1) == buf_tail){ //we're about to eat the tail!
                buf_overflow=true; //we don't care if head==tail, as the buffer's empty anyways
                return false;
            }
            memcpy(&hid_buffer[buf_head], pack, 64);
            if (buf_head == (HID_BUF_SIZE-1)){ //did we just fill the last slot in the strip?
                buf_head = 0; //reset buf_head
            }
            else{ //we don't want to increment when we reset
                buf_head++; //increment head
            }
            return true;
        }
        bool HIDTransport::pop_packet(hid_packet* dest) { //we don't want to return a pointer in the buffer, or use malloc, so let the user allocate space
            if(buf_tail != buf_head){ //prevent tail going past the head if there is no data
                memcpy(dest, &hid_buffer[buf_tail], 64); //again, fixed size to buffer
                if (buf_tail == (HID_BUF_SIZE-1)){ //did we just pop the last value in the buffer?
                    buf_tail = 0; //reset tail to start of buffer
                }else {
                    buf_tail++; //we dont care if this now equals head, as there's no data anyways.
                }
                return true;
            }
            return false;
        }
        bool HIDTransport::peek_packet(hid_packet* dest){ //peek at FIFO without popping data
            if(available()){
                memcpy(dest, &hid_buffer[buf_tail], 64);
                return true;
            } else { //no data to return
                return false;
            }
        }
        bool HIDTransport::available(){
            return (buf_head != buf_tail); //if they are different, there is data.
        }
        bool HIDTransport::overflow(){ //resets the overflow flag, so be sore to store it somewhere if needed
            bool retval = buf_overflow;
            buf_overflow = false;
            return retval;
        }
