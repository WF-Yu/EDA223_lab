/*
 *  User Guide ******************
 * 1. after load the program, type 'go' to start the application
 * 2. the application can save up to three intergers less than 20 digits each 
 * 3. to input the integer , you first type the negative sign (fo negative bumber), 
 *    and then type each digit, after all digits are typed, type 'e' as the delimiter
 * 4. to clear the history of numbers typer 'f' or 'F'
 * 5. If you accidentally typed characteres other than numbers, you should use 'f' to clear the buffer and reset
 * 
 * To sumarize: 
 * 	f, F: clear sumary 
 * 	u, d: up, down volume
 * 	h, l: higher, lower background load
 *  s: ddl enable
 * */
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

#define init_freq_index() {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0}
#define init_period() {2024,1911,1803,1702,1607,1516,1431,1351,1275,1203,1136,1072,1012,955,901,851,803,758,715,675,637,601,568,536,506}

typedef struct {
	Object super;
	char buff[64];
	int freq_index[32];
	int period[25];
	int key;
	int volume;
	int mute;		
	int sound_freq;
	int ddl_en; // deadline_enable
	int ddl;
	Time totalTime;
	Time maxTime;
	int cnt;
	Time endtime;
	Time starttime;
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
	int sum;
	int median;
} App;

typedef struct {
	Object super;
	char buff[64]; 
	int background_loop_range;
	int interval;
	int ddl_en; // deadline_enable
	int ddl;
	Time totalTime;
	Time maxTime;
	int cnt;
} Background_load;



void reader(App*, int);
void receiver(App*, int);
void read_integer(App*, int);
void find_median(App* , int );
void print_info(App*, int);

void play_sound(ObjSound*, int);
void set_key(ObjSound* self, int _key);
void set_volume(ObjSound* self, int c);		
void calc_period(ObjSound* self, int unused);
void set_ddl_sound(ObjSound*, int);

void bg_loops(Background_load*, int);
void set_bg_load(Background_load* self, int c);
void set_ddl_bg(Background_load*, int);


App app = { initObject(), 0, 'X', 20, 0, {0}, {0}, 0, 0, 0, 0};
ObjSound sound_0 = {initObject(), {0}, init_freq_index(), init_period(),0,5,0,1000, 0, 100, 0, 0, 0, 0, 0};
Background_load background_load = {initObject(), {0}, 21500, 1300, 0, 1300, 0, 0, 0};

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

// app.c-----------------------------------------------------------------------------------------------------------
void receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) {
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
	ASYNC(self,read_integer,c);	
}


void find_median(App *self, int unused){
	/*deal with the situation with only 1 or 2 int*/
	if (self->rdcnt < 2){
		self->median = self->sum / (self->rdcnt +1) ;
	}
	/*find median for 3 ints*/
	if(self->rdcnt == 2) {
		if (self->num[0] > self->num[1]){
			if(self->num[0]> self-> num[2]){
				if(self->num[1]>self->num[2]){
					self->median = self->num[1];
				}
				else {
					self->median  = self->num[2];
				}
			}
			else {
				self->median  = self->num[0];
			}
		}	
		else {
			if(self->num[1]> self-> num[2]){
				if(self->num[0]>self->num[2]){
					self->median  = self->num[0];
				}
				else {
					self->median  = self->num[2];
				}
			}
			else {
				self->median  = self->num[1];
			}
		}
	}
	
	/*increase the read counter*/
	if (self->rdcnt <2){
		self->rdcnt++;
	}
	/*print the sum and median info*/
	// ASYNC(self, print_info, 0);
}

void print_info(App* self, int unused){
	SCI_WRITE(&sci0, "Entered Integer: \'");
	SCI_WRITE(&sci0, self->buff);
	SCI_WRITE(&sci0, ",\' Sum = \'");
	snprintf(self->buff, sizeof(self->buff), "%d", self->sum);
	SCI_WRITE(&sci0, self->buff);
	SCI_WRITE(&sci0, ",\' Median = \'");
	snprintf(self->buff, sizeof(self->buff), "%d", self->median);
	SCI_WRITE(&sci0, self->buff);
	SCI_WRITE(&sci0, "\'\n");
}

/* output integer input form uart */
void read_integer(App *self, int c) {
	int k;
	if ( (c == 'f') | (c == 'F')){
		self->rdcnt = 0;
		self->wrcnt = 0;
		self->i = 0;
		self->num[0] = 0;
		self->num[1] = 0;
		self->num[2] = 0;
		self->sum = 0;
		SCI_WRITE(&sci0, "The 3-history has been erased!\n");
	}else if (c != 'e' ) {
		if (self->i < (self->buffsize-1) ){
		self->buff[self->i++] = c;
		}
		else {
			 //SCI_WRITE(&sci0, "Warning: Buffer reaches the max legnth!\n");
		}
	}
	else{
		self->buff[self->i] = '\0';
		self->num[self->wrcnt] = atoi(self->buff);
		
		// self->key = atoi(self->buff);
		ASYNC(&sound_0, set_key, atoi(self->buff));
		
		ASYNC(&sound_0, calc_period, 0);
		
		self->i = 0;
		self->sum = 0;
		for (k=0; k<= self->rdcnt; k++){
			self->sum += self->num[k];
		}		
		/*update the write conuter*/
		if (self->wrcnt <2){
			self->wrcnt++;
		}
		else {
			self->wrcnt =0;
		}
		
		/*find the mdian value*/
		ASYNC(self, find_median, 0);
		
	}
	
	ASYNC(&background_load, set_ddl_bg, c);
	ASYNC(&background_load, set_bg_load, c);
	
	ASYNC(&sound_0, set_ddl_sound, c);	
	ASYNC(&sound_0, set_volume, c);
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
	ASYNC(&sound_0, play_sound, 1);
	// ASYNC(&background_load, bg_loops, 0);
}
//main.c  ------------------------------------------------------------------------------------------------------
int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
	//TINYTIMBER(&sound_0, play_sound, 1);
    return 0;
}


//  sound generator part soudn.c
//sound.c -------------------------------------------------------------------------------------------------------

void play_sound(ObjSound* self, int ON){
			
	int* p = (int*)0x4000741C;
	
	int count_2 = 100;
	
	
	while (count_2--) {
		int count = 100;
		self->starttime = CURRENT_OFFSET();
		while (count--) {
			if (ON == 1){
				*(p) = self-> volume * self->mute;
				ON = 0;
			}
			if (ON == 0){
				*(p) = 0;
				ON = 1;
			}
		}
		self->endtime = CURRENT_OFFSET();
		self->totalTime += self->endtime - self->starttime; 
		self->maxTime = self->maxTime > (self->endtime - self->starttime) ? self->maxTime : (self->endtime - self->starttime);
	}
	snprintf(self->buff, sizeof(self->buff), "\nTotal Time for 100 rounds: %ld\n", self->totalTime);
	SCI_WRITE(&sci0, self->buff);
	snprintf(self->buff, sizeof(self->buff), "\n9 * Max Time: %ld\n", self->maxTime);
	SCI_WRITE(&sci0, self->buff);
	
/*	if (self->cnt %10 == 9) {
		self->end = CURRENT_OFFSET();
		if (self->cnt < 500) {
			self->totalTime += self->end - self->start; 
			self->maxTime = self->maxTime > (self->end - self->start) ? self->maxTime : (self->end - self->start);
			
			snprintf(self->buff, sizeof(self->buff), "\n9 * CUrrent Max Time: %ld\n", self->maxTime);
			SCI_WRITE(&sci0, self->buff);
		}
		if (self->cnt == 500) {
			snprintf(self->buff, sizeof(self->buff), "\nTotal Time: %ld\n", self->totalTime);
			SCI_WRITE(&sci0, self->buff);
			snprintf(self->buff, sizeof(self->buff), "\n9 * Max Time: %ld\n", self->maxTime);
			SCI_WRITE(&sci0, self->buff);
		}
		
	}
	
	self->cnt++;
	
	if (self->ddl_en) {
				
		SEND(USEC(500000 / self->sound_freq), self->ddl, self, play_sound, (ON + 1) % 2);
	}
	else {
		
		AFTER(USEC(500000 / self->sound_freq), self, play_sound, (ON + 1) % 2);
	}*/
}

void set_key(ObjSound* self, int _key) {
	self->key = _key;	
}


void set_volume(ObjSound* self, int c) {
	switch (c) {
		case 'u':
			if (self->volume < 20) {
				self->volume += 1;
			}
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		case 'd':
			if (self->volume > 0) {
				self->volume -= 1;
			}
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		case 'm':
			self->mute = self->mute==0?1:0;
			snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume * self->mute);
			SCI_WRITE(&sci0, self->buff);
		break;
		
		default:;
	}
}
void calc_period(ObjSound* self, int unused) {
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
void set_ddl_sound(ObjSound* self, int c) {
	if (c == 's') {
		self->ddl_en = self->ddl_en == 0 ? 1 : 0;
		snprintf(self->buff, sizeof(self->buff), "\nSound generator deadline enable: %d\n", self->ddl_en);
		SCI_WRITE(&sci0, self->buff);
	}
}
// -----------------------------------------------------------------
void bg_loops(Background_load* self, int unused) {
	Time end;
	Time start = CURRENT_OFFSET();
	int local_cnt = self->background_loop_range;
	while(local_cnt--); // 
	
	if (self->ddl_en) {
		end = CURRENT_OFFSET();
		if (self->cnt++ < 500) {
			self->totalTime += end - start;
			self->maxTime = self->maxTime > (end - start) ? self->maxTime : (end - start);
		}
		if (self->cnt == 500) {
			snprintf(self->buff, sizeof(self->buff), "\nTotal Time: %ld\n", self->totalTime);
			SCI_WRITE(&sci0, self->buff);
			snprintf(self->buff, sizeof(self->buff), "\nMax Time: %ld\n", self->maxTime);
			SCI_WRITE(&sci0, self->buff);
		}
		
		SEND(USEC(self->interval), self->ddl, self, bg_loops, 0);
	}
	else {		
		end = CURRENT_OFFSET();
		if (self->cnt++ < 500) {
			self->totalTime += end - start;
			self->maxTime = self->maxTime > (end - start) ? self->maxTime : (end - start);
		}
		if (self->cnt == 500) {
			snprintf(self->buff, sizeof(self->buff), "\nTotal Time: %ld\n", self->totalTime);
			SCI_WRITE(&sci0, self->buff);
			snprintf(self->buff, sizeof(self->buff), "\nMax Time: %ld\n", self->maxTime);
			SCI_WRITE(&sci0, self->buff);
		}
		AFTER(USEC(self->interval),self, bg_loops, 0);
	}
}
void set_bg_load(Background_load* self, int c) {
	switch (c) {
		case 'h':
			if (self->background_loop_range < 8000) {
				self->background_loop_range += 500;
			}
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