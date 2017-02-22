
#define HID_BUF_SIZE 16
struct hid_packet { //USB handles CRC/packet loss, so all we need is size and out of order correction
    char magic; //magic byte to make sure it's the actual payload
    char seq; //sequence number of existing packet, when sent with a NACK means "restart from this packet"
    struct flags; //flags, 1 byte     
    char len; //packet len, so trailing garbage/padding is ignored
    char data[60]; //data, 60 bytes
};
struct flags {
    bool syn:1; //tell GCC that each variable takes 1 bit
    bool ack:1;   
    bool nack:1; //NACK bit, used to signal out-of-order reception
    bool cmd:1; //this is a command to the teensy from the payload, 0 for data
    bool unused:4; //tell GCC this takes all 4 remaining bits
};
class HIDTransport {
    private:
        bool buf_overflow;
        hid_packet* hid_buffer;
        int buf_head;
        int buf_tail;
    public:
        bool pop_packet(hid_packet*);
        bool push_packet(hid_packet*);
        bool peek_packet(hid_packet*);
        bool available();
        bool overflow();
        HIDTransport();
};

extern HIDTransport hid_transport;
