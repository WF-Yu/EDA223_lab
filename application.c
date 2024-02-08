/*
 *  User Guide ******************
 * 1. after load the program, type 'go' to start the application
 * 2. the application can save up to three intergers less than 20 digits each 
 * 3. to input the integer , you first type the negative sign (fo negative bumber), 
 *    and then type each digit, after all digits are typed, type 'e' as the delimiter
 * 4. to clear the history of numbers typer 'f' or 'F'
 * 5. If you accidentally typed characteres other than numbers, you should use 'f' to clear the buffer and reset
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
	int freq_index[32];
	int period[25];
	int key;
	int volume;
	int mute;		
	int sound_freq;
} App;



App app = { initObject(), 0, 'X', 20, 0, {0}, {0}, 0, 0, 0, 0, init_freq_index(), init_period(),0,5,0,1000};

void reader(App*, int);
void receiver(App*, int);
void read_integer(App*, int);
void find_median(App* , int );
void print_info(App*, int);
void calc_period(App*, int);
void play_sound(App*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);

void play_sound(App* self, int ON){
	int* p = (int*)0x4000741C;
	if (ON == 1){
		*(p) = self-> volume * self->mute;
	}
	if (ON == 0){
		*(p) = 0;
	}
	
	AFTER(USEC(500000 / self->sound_freq),self, play_sound, (ON + 1) % 2);
}

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

void calc_period(App* self, int unised) {
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
		self->key = atoi(self->buff);
		ASYNC(self, calc_period, 0);
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
	if ( c == 'u') {
		if (self->volume < 20) {
			self->volume += 1;
		}
	} 
	if ( c == 'd') {
		if (self->volume > 0) {
			self->volume -= 1;
		}
	}
	if ( c == 'u' || c == 'd') {
		snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume);
		SCI_WRITE(&sci0, self->buff);
	}
	
	if ( c == 'm') {
		self->mute = self->mute==0?1:0;
		snprintf(self->buff, sizeof(self->buff), "\nCurrent Volume is: %d\n", self->volume * self->mute);
		SCI_WRITE(&sci0, self->buff);
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
	ASYNC(self, play_sound, 1);
	
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
