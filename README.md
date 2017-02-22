This is the BadUSB subset of my TeensyPWN project. This is designed to run on a teensy 3.5/6 using the internal microSD. The project uses a modified version of the USaBUSe transport using RawHID.
The modificiations include the addition of a sequence field and flags.

```
struct hid_packet { //USB handles CRC/packet loss, so all we need is size and out of order correction
    char magic; //magic byte to make sure it's the actual payload
    char seq; //sequence number of existing packet, when sent with a NACK means "restart from this packet"
    struct flags; //flags, 1 byte
    char len; //packet len, so trailing garbage/padding is ignored
    char data[60]; //data, 60 bytes
}
struct flags {
    bool syn:1; //tell GCC that each variable takes 1 bit
    bool ack:1;
    bool nack:1; //NACK bit, used to signal out-of-order reception
    bool cmd:1; //this is a command to the teensy from the payload, 0 for data
    bool unused:4; //tell GCC this takes all 4 remaining bits
}
```
