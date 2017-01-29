//
// Created by Chengjiu Zhang on 1/29/17.
//

#include <threads.h>
#include <hardware.h>
#include <terminals.h>

void TransmitInterrupt(int term){

};

void ReceiveInterrupt(int term){

};

void WriteDataRegister(int term, char c){

};


char ReadDataRegister(int term){

};


int InitHardware (int term){

};

int HardwareOutputSpeed(int term, int msecs){

};


int HardwareInputSpeed(int term, int msecs){

};


int WriteTerminal(int term, char *buf, int buflen){

};


int ReadTerminal(int term, char *buf, int buflen){

};


int InitTerminal(int term){

};


int TerminalDriverStatistics(struct termstat *stats){

};


int InitTerminalDriver(void){

};
