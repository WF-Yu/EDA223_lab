/*
 *  User Guide ******************
 * 1. after load the program, type 'go' to start the application
 * 2. the music player will automatically start to play brother johns
 * 3. with default volume 4, key 0, tempo 120bpm
 * 4. could press 'u' to increase volume, 'd' to decrease volume
 * 5. could press 'm' to mute;
 * 6. to change the key: input the integer value and press 'k' as delimeter
 * 7. to change the tempo: input the integer value and press 't' as delimeter
 * 
 * Note: background load and calculate median functions are disabled in this version
 * 
 * To sumarize: 
 * 	e	: delimeter of integer input
 * 	f, F: clear sumary 
 *  m	: mute toggle button
 * 	u, d: up, down volume
 * 	h, l: higher, lower background load
 *  s: sound and background ddl enable toggle button
 *  k: delimeter of integer key input
 *  t: delimeter of integer tempo input
 *  c: enter conductor mode start play
 *  v: enter musician mode start play
 *  p: show current mode
 *  q: to stop play

 * */
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define init_can_msg()	{0,0,0,{0}}
#define init_freq_index() {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0}
#define init_period() {2024,1911,1803,1702,1607,1516,1431,1351,1275,1203,1136,1072,1012,955,901,851,803,758,715,675,637,601,568,536,506}
#define init_note_length() {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 4, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 4, 2, 2, 4}
                         //{a, a, a, a, a, a, a, a, a, a, b, a, a, b, c, c, c, c, a, a, c, c, c, c, a, a, a, a, b, a, a, b}
	
#define CAN_PORT1   (CAN_TypeDef *)(CAN2)

typedef struct {
	Object super;
	char buff[64];
	int volume;
	int mute;		
	int sound_freq;
	int ddl_en; // deadline_enable
	int ddl;
	int silent_flag;  // 1 means silence
	int _ON;		 // on switch, 1 means on , 0 means off
} ObjSound;

typedef struct {
    Object super;
    int count;
    char c;
	int buffsize;
	int i; 		/* the index of buffer*/
	char buff[64];  /*buf to store the integer chars*/
	int num[3];    /*integer input from uart*/
	int wrcnt ; 	/*number of integer write into*/
	int rdcnt;		/*number of integer can be read*/
	CANMsg msg;		// CAN msg to send
	int mode; 		// 0 conductor, 1 musician, 3 default
} App;
	
typedef struct {
    Object super;
	char buff[64]; 
	int freq_index[32]; // tone of the music
	int period[25]; // period look up table
	int note_len[32]; // length of each note in freq_index[]
	int key;
	int tempo; // in BPM
	int index; // current tone to play
	int totalTones; // number of tones in our music	
	int _ON;		 // on switch, 1 means on , 0 means off
} Controller;

typedef struct {
	Object super;
	char buff[64]; 
	int background_loop_range;
	int interval;
	int ddl_en; // deadline_enable
	int ddl;
} Background_load;

//------------------App methods
void reader(App*, int);
void receiver(App*, int);
void receiver_2(App*, int);
void read_integer(App*, int);
void parse_can_msg(App*, int);
void reset_mode(App*, int);	

//-------------- tone generator
void play_sound(ObjSound*, int);
void set_volume(ObjSound* self, int c);		
void set_ddl_sound(ObjSound*, int);
void set_freq(ObjSound*, int freq);
void make_silence(ObjSound* self, int unused);
void stop_play_sound(ObjSound* self, int _OFF);

// -------------- music player controller
void go_play(Controller* self, int c);
void set_key(Controller* self, int _key);
void set_tempo(Controller* self, int _tempo);
void display_period(Controller* self, int unused);
void stop_go_play(Controller* self, int _OFF);


//  --------------- Background load
void bg_loops(Background_load*, int);
void set_bg_load(Background_load* self, int c);
void set_ddl_bg(Background_load*, int);


App app = { initObject(), 0, 'X', 20, 0, {0}, {0}, 0, 0, init_can_msg(), 3};
ObjSound sound_0 = {initObject(), {0}, 4, 1, 1000, 1, 900, 0, 0};
Controller ctrl_obj = {initObject(), {0}, init_freq_index(), init_period(), init_note_length(), 0, 120, 0, 32, 0};
Background_load background_load = {initObject(), {0}, 0, 1300, 0, 1300};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);     

// app.c-----------------------------------------------------------------------------------------------------------
void reset_mode(App* self, int c){
	self->mode = c;
}

void receiver(App *self, int unused) {
    CAN_RECEIVE(&can0, &self->msg);
//    SCI_WRITE(&sci0, "\nCan1 msg received: \n");
//    SCI_WRITE(&sci0, self->msg.buff);
	ASYNC(self,parse_can_msg,0);
}

void parse_can_msg(App *self, int unused) {
	// only parse the msg when it is in musician mode
	if(self->mode == 1 ){
		SCI_WRITE(&sci0, "\nEnter parse CAN msg\n");
		uchar c = 0, sign_c;
		int  can_input = 0;

		can_input = self->msg.buff[0]; 
		c = self->msg.buff[1]; 
		sign_c = self->msg.buff[2];  // the sign bit 
		
		if(sign_c == 'n'){
			can_input = 0-can_input;
		}
		
//		snprintf(self->buff, sizeof(self->buff), "\nInput integer is: %d\n", can_input);
//		SCI_WRITE(&sci0, self->buff);
		snprintf(self->buff, sizeof(self->buff), "\nInput cmd is: %c\n", c);
		SCI_WRITE(&sci0, self->buff);
		
		switch(c){
			case 'c':
				self->mode = 0; 
				break;
			case 'u':
			case 'd':
			case 'm':
				// set volume 
				ASYNC(&sound_0, set_volume, c);
				break;
			case 's':
				ASYNC(&sound_0, set_ddl_sound, c);
				break;	
			case 'k':
				// set the key
				ASYNC(&ctrl_obj, set_key, can_input);
				break;
			case 't':
				// set the key
				ASYNC(&ctrl_obj, set_tempo, can_input);
				break;	
				
			default :;
		}
		// only carry out execution in conductor mode
		if(c == 'q'){
			// stop the controller
			ASYNC(&ctrl_obj, stop_go_play, 0); 
			// stop the tone generator
			ASYNC(&sound_0, stop_play_sound, 0);
			// reset the mode
			ASYNC(self, reset_mode, 3);
		}
	}
	
}

void reader(App *self, int c) {
	CANMsg msg;
    SCI_WRITE(&sci0, "\nRcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	switch(c){
		// conductor mode
		case 'c':
			if(self->mode == 3){
				// only start start the call when power on
				// reset the controller
				SYNC(&ctrl_obj, stop_go_play, 1); 
				// reset the tone generator
				SYNC(&sound_0, stop_play_sound, 1);
				// start music controller
				ASYNC(&ctrl_obj, go_play, 0); 
				// start the tone generator
				ASYNC(&sound_0, play_sound, 0);
				self->mode = 0; 
				SCI_WRITE(&sci0, "\nEnter Conductor mode\n");
			}			
			break;
			
		// musician mode	
		case 'v':		// go_play and play_sound will start in parse_can_msg()
			if(self->mode == 3){
				// only start the call once
				// reset the controller
				SYNC(&ctrl_obj, stop_go_play, 1); 
				// reset the tone generator
				SYNC(&sound_0, stop_play_sound, 1);
				// start music controller
				ASYNC(&ctrl_obj, go_play, 0); 
				// start the tone generator
				ASYNC(&sound_0, play_sound, 0);
			}
			// set the musician mode
			self->mode = 1; 
			SCI_WRITE(&sci0, "\nEnter musician mode\n");
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'v';
			CAN_SEND(&can0, &msg);
			break;
			
		// show current mode
		case 'p':
			if(self->mode == 1){
				SCI_WRITE(&sci0, "\nIn musician mode\n");
			}
			if(self->mode == 0){
				SCI_WRITE(&sci0, "\nIn conductor mode\n");
			}
			if(self->mode == 3){
				SCI_WRITE(&sci0, "\nIn Idle mode\n");
			}
			break;
			
		default:;	
	}
	ASYNC(self,read_integer,c);	
}

/* output integer input form uart */
void read_integer(App *self, int c) {
	CANMsg msg;
	int tmp =0;
	switch(c){
		case 'f': 
			self->rdcnt = 0;
			self->wrcnt = 0;
			self->i = 0;
			self->num[0] = 0;
			self->num[1] = 0;
			self->num[2] = 0;
			SCI_WRITE(&sci0, "The history has been erased!\n");
			break;
			
		case 'e':
			self->buff[self->i] = '\0';
			self->num[self->wrcnt] = atoi(self->buff);
			self->i = 0;
			snprintf(self->buff, sizeof(self->buff), "\nInput integer is: %d\n", self->num[self->wrcnt]);
			SCI_WRITE(&sci0, self->buff);
			/*update the write conuter*/
			if (self->wrcnt <2){
				self->wrcnt++;
			}
			else {
				self->wrcnt =0;
			}
			break;
		
		case 'c':		
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'c';
			CAN_SEND(&can0, &msg);
			break;

		case 'u':
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'u';
			CAN_SEND(&can0, &msg);
			break;
			
		case 'd':
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'd';
			CAN_SEND(&can0, &msg);
			break;	
			
		case 'm':
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'm';
			CAN_SEND(&can0, &msg);
			break;
			
		case 's':
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 's';
			CAN_SEND(&can0, &msg);
			break;	
		
		// stop the player
		case 'q':
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 2;
			msg.buff[0] = 10;	// unused int only to occupy the position
			msg.buff[1] = 'q';
			CAN_SEND(&can0, &msg);
			break;
			
		case 'k':
			self->buff[self->i] = '\0';
			self->num[self->wrcnt] = atoi(self->buff);
			self->i = 0;
			
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 3;			
			if(self->num[self->wrcnt]<0){
				tmp = abs(self->num[self->wrcnt]);
				msg.buff[2] = 'n';		// negative
			}
			else{
				tmp = self->num[self->wrcnt];
				msg.buff[2] = 'p';	//positive
			}			
			msg.buff[0] = tmp;
			msg.buff[1] = 'k';
			CAN_SEND(&can0, &msg);
			// only carry out execution in conductor mode
			if (self->mode == 0){
				// set the key
				ASYNC(&ctrl_obj, set_key, self->num[self->wrcnt]);
			}
			/*update the write conuter*/
			if (self->wrcnt <2){
				self->wrcnt++;
			}
			else {
				self->wrcnt =0;
			}
			break;
			
		case 't':
			self->buff[self->i] = '\0';
			self->num[self->wrcnt] = atoi(self->buff);
			self->i = 0;
			// prepare and send CAN msg
			msg.msgId = 1;
			msg.nodeId = 1;
			msg.length = 3;			
			if(self->num[self->wrcnt]<0){
				tmp = abs(self->num[self->wrcnt]);
				msg.buff[2] = 'n';	//negative
			}
			else{
				tmp = self->num[self->wrcnt];
				msg.buff[2] = 'p';	//positive
			}			
			msg.buff[0] = tmp;
			msg.buff[1] = 't';
			
			CAN_SEND(&can0, &msg);
			if (self->mode == 0){
				// set the key
				ASYNC(&ctrl_obj, set_tempo, self->num[self->wrcnt]);
			}
			/*update the write conuter*/
			if (self->wrcnt <2){
				self->wrcnt++;
			}
			else {
				self->wrcnt =0;
			}
			break;

		case '0' :
		case '1' :
		case '2' :
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
		case '8' :
		case '9' :
		case '-' :
			if (self->i < (self->buffsize-1) ){
				self->buff[self->i++] = c;
			}
			else {
				 SCI_WRITE(&sci0, "Warning: Int Buffer reaches the max legnth!\n");
			}
			
		default :;
	}
	if (self->mode == 0){
		ASYNC(&sound_0, set_ddl_sound, c);	
		ASYNC(&sound_0, set_volume, c);
		// only carry out execution in conductor mode
		if(c == 'q'){
			// stop the controller
			ASYNC(&ctrl_obj, stop_go_play, 0); 
			// stop the tone generator
			ASYNC(&sound_0, stop_play_sound, 0);
			// reset the mode
			ASYNC(self, reset_mode, 3);
		}
	}
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");

    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);

}
//main.c  ------------------------------------------------------------------------------------------------------
int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}

//  sound generator part soudn.c
//sound.c -------------------------------------------------------------------------------------------------------
void stop_play_sound(ObjSound* self, int _OFF){
	self->_ON = _OFF;
}

void play_sound(ObjSound* self, int ON){
	int* p = (int*)0x4000741C;		// device address
	
	if (ON == 1){
		if (self->silent_flag){
			*(p) = 0;
		}
		else{
			*(p) = self-> volume * self->mute;	// write the volume to the device addr
		}
	}
	if (ON == 0){
		*(p) = 0;							// write 0 to the device addr
	}
	
	if (self->_ON){
		if (self->ddl_en) {					// enable deadline
			SEND(USEC(500000 / self->sound_freq), USEC(self->ddl), self, play_sound, (ON + 1) % 2);
		}
		else {
			AFTER(USEC(500000 / self->sound_freq), self, play_sound, (ON + 1) % 2);		// disable deadline
		}
	}
}

void set_volume(ObjSound* self, int c) {
	switch (c) {
		case 'u':				// volume up
			if (self->volume < 20) {
				self->volume += 1;
			}
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume);
			SCI_WRITE(&sci0, self->buff);			
		break;
		
		case 'd':				//volume down
			if (self->volume > 0) {
				self->volume -= 1;
			}
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		case 'm':				// mute
			self->mute = self->mute==0?1:0;
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume * self->mute);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		default:;
	}
}

void make_silence(ObjSound* self, int _silence) {
	self->silent_flag = _silence; 
}

void set_ddl_sound(ObjSound* self, int c) {
	if (c == 's') {
		self->ddl_en = self->ddl_en == 0 ? 1 : 0;
		snprintf(self->buff, sizeof(self->buff), "\nSound generator deadline enable: %d\n", self->ddl_en);
		SCI_WRITE(&sci0, self->buff);
	}
}

void set_freq(ObjSound* self, int _freq){
	self->sound_freq = _freq;	// 1 is set, 0 is clear
}

// --Controller.c --------------------------------------------------------------------------------------------------------------------------------
void stop_go_play(Controller* self, int _OFF){
	self->_ON = _OFF;
	// reset the note index
	self->index = 0;
}

void go_play(Controller* self, int unused) {
	int cur_beat_length = 0;
	int _silent_time = 0;
	// calculate the frequency index according to key
	int _freq_index = self->freq_index[self->index]+self->key;
	int _period_index = _freq_index + 10;
	int current_freq ;
	// calculate the current frequency
	current_freq = 1000000/(2*self->period[_period_index]);
	// set the frequency, highest priority
	SYNC(&sound_0, set_freq, current_freq);
	// calculate the beat length
	cur_beat_length = self->note_len[self->index] * 30000 / self->tempo;
	// dynamic slience time
	if(self->tempo>=60 && self->tempo < 100){
		_silent_time = 100;
	}
	if(self->tempo>=100 && self->tempo < 140){
		_silent_time = 80;
	}
	if(self->tempo>=140 && self->tempo < 180){
		_silent_time = 60;
	}
	if(self->tempo>=180 && self->tempo <= 240){
		_silent_time = 50;
	}
	// leave 60 ms for silence
	SEND(MSEC(cur_beat_length - _silent_time), USEC(100), &sound_0, make_silence, 1);
	// reset the silent flag
	SEND(MSEC(cur_beat_length-1), USEC(100),&sound_0, make_silence, 0);
	
	if(self->_ON){
		// call itself to achieve the periodic play
		SEND(MSEC(cur_beat_length), USEC(100), self, go_play, 0);
	}
	// loop within the music segments
	if (self->index < (self->totalTones -1)) { 
		self->index++; 
	}
	else {self->index = 0;}	
}

void set_key(Controller* self, int _key) {
	if (_key>=-5 && _key <=5){
		self->key = _key;
		snprintf(self->buff, sizeof(self->buff), "\nCurrent key is: %d\n", self->key);
		SCI_WRITE(&sci0, self->buff);
	}
	else{
		SCI_WRITE(&sci0, "\nAttention: Key should be between [-5,5]\n");
	}
}

//   display the new period and freq according to new key 
void display_period(Controller* self, int unused) {
	int x = self->key + 10;
	int new_freq_index[32] = {0};
	int new_period[32] = {0};
	for (int i = 0; i < 32; i++) {
		new_freq_index[i] = self->freq_index[i] + x;	
		new_period[i] = self->period[new_freq_index[i]];
	}
	
	SCI_WRITE(&sci0, "\nfrequency:\n");
	for (int i = 0; i < 32; i++) {
		snprintf(self->buff, sizeof(self->buff), "%d ", new_freq_index[i] - 10);
		SCI_WRITE(&sci0, self->buff);
	}
	
	SCI_WRITE(&sci0, "\nperiod:\n");
	for (int i = 0; i < 32; i++) {
		snprintf(self->buff, sizeof(self->buff), "%d ", new_period[i]);
		SCI_WRITE(&sci0, self->buff);
	}
}

void set_tempo(Controller* self, int _tempo) {
	if(_tempo <=240 && _tempo>=60){
		self->tempo = _tempo;
		snprintf(self->buff, sizeof(self->buff), "\nCurrent tempo is: %d\n", self->tempo);
		SCI_WRITE(&sci0, self->buff);
	}
	else{
		SCI_WRITE(&sci0, "\nAttention: Tempo should be between [60,240] bpm\n");
	}
	
}

// background load .c-----------------------------------------------------------------
void bg_loops(Background_load* self, int unused) {
	int cnt = self->background_loop_range;
	while(cnt--);
	if (self->ddl_en) {
		SEND(USEC(self->interval), USEC(self->ddl), self, bg_loops, 0);
	}
	else {
		AFTER(USEC(self->interval),self, bg_loops, 0);
	}
}
void set_bg_load(Background_load* self, int c) {
	switch (c) {
		case 'h':
//			if (self->background_loop_range < 8000) {
				self->background_loop_range += 500;
//			}
			snprintf(self->buff, sizeof(self->buff), "\nBackground load is: %d\n", self->background_loop_range);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		case 'l':
			if (self->background_loop_range > 0) {
				self->background_loop_range -= 500;
			}
			snprintf(self->buff, sizeof(self->buff), "\nBackground load is: %d\n", self->background_loop_range);
			SCI_WRITE(&sci0, self->buff);
		break;
	}
}
void set_ddl_bg(Background_load* self, int c) {
	if (c == 's') {
		self->ddl_en = self->ddl_en == 0 ? 1 : 0;
		snprintf(self->buff, sizeof(self->buff), "\nBackground load deadline enable: %d\n", self->ddl_en);
		SCI_WRITE(&sci0, self->buff);
	}
}