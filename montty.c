//
// Created by Chengjiu Zhang on 1/29/17.
//

#include <threads.h>
#include <hardware.h>
#include <terminals.h>

// output register and input register
static char out_reg[NUM_TERMINALS];
static char in_reg[NUM_TERMINALS];


void TransmitInterrupt(int term){
    Declare_Monitor_Entry_Procedure();

};

void ReceiveInterrupt(int term){
    Declare_Monitor_Entry_Procedure();

};


int WriteTerminal(int term, char *buf, int buflen){
    Declare_Monitor_Entry_Procedure();

};


int ReadTerminal(int term, char *buf, int buflen){
    Declare_Monitor_Entry_Procedure();

};


int InitTerminal(int term){
    Declare_Monitor_Entry_Procedure();

    return InitHardware(term);
};


int TerminalDriverStatistics(struct termstat *stats){
    Declare_Monitor_Entry_Procedure();

};


int InitTerminalDriver(void){
    Declare_Monitor_Entry_Procedure();

    return 0;
};
