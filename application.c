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

typedef struct {
    Object super;
    int count;
    char c;
	int buffsize;
	int i; 		/* the index of buffer*/
	char buff[20];  /*buf to store the integer chars*/
	int num[3];    /*integer input from uart*/
	int wrcnt ; 	/*number of integer write into*/
	int rdcnt;		/*number of integer can be read*/
	int sum;
	int median;
} App;

App app = { initObject(), 0, 'X', 20, 0, {0}, {0}, 0, 0, 0, 0};

void reader(App*, int);
void receiver(App*, int);
void read_integer(App*, int);
void find_median(App* , int );
void print_info(App*, int);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);

Can can0 = initCan(CAN_PORT0, &app, receiver);

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
	ASYNC(self, print_info, 0);
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
			SCI_WRITE(&sci0, "Warning: Buffer reaches the max legnth!\n");
		}
	}
	else{
		self->buff[self->i] = '\0';
		self->num[self->wrcnt] = atoi(self->buff);
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

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
