//
// Created by Chengjiu Zhang on 1/29/17.
//

#include <threads.h>
#include <hardware.h>
#include <terminals.h>

#include <stdio.h>
//#include "threads.h"
//#include "hardware.h"
//#include "terminals.h"

#define ECHO_SIZE 2048
#define INPUT_SIZE 2048
#define ACTIVE 1
#define IDLE 0

// output register and input register
//static char out_reg[NUM_TERMINALS];
//static char in_reg[NUM_TERMINALS];

// Terminal Driver Statistics
static struct termstat statarr[NUM_TERMINALS];

// Terminal initialized(0) or not(1).
static int terminalinit[NUM_TERMINALS];

// buffer
static char echobuffer[NUM_TERMINALS][ECHO_SIZE];
static int echoindex[NUM_TERMINALS];
static int curechindex[NUM_TERMINALS];
static char inputbuffer[NUM_TERMINALS][INPUT_SIZE];
static int inputindex[NUM_TERMINALS];
static int curinputindex[NUM_TERMINALS];
static char* outputbuffer[NUM_TERMINALS];
static int outputindex[NUM_TERMINALS];
static int curoutindex[NUM_TERMINALS];

// condition variable
static cond_id_t condwrite[NUM_TERMINALS];
static cond_id_t condread[NUM_TERMINALS];
static cond_id_t  condline[NUM_TERMINALS];
static cond_id_t condbusy[NUM_TERMINALS];
static cond_id_t condecho[NUM_TERMINALS];

// states related to condition variables
static int statewrite[NUM_TERMINALS];
static int stateecho[NUM_TERMINALS];
static int stateread[NUM_TERMINALS];
static int numlines[NUM_TERMINALS];
static int statenewline[NUM_TERMINALS];
static int statenewchar[NUM_TERMINALS];
static int statebusy[NUM_TERMINALS];

void TransmitInterrupt(int term){
    Declare_Monitor_Entry_Procedure();

    // cannot wait here

    char tempchar = '\0';


    // echo first
    if(echoindex[term] != curechindex[term]){
        tempchar = echobuffer[term][curechindex[term]];
        curechindex[term] = (curechindex[term]+1)%ECHO_SIZE;
        WriteDataRegister(term, tempchar);
    }
    else {
        stateecho[term] = IDLE;
        CondSignal(condecho[term]);
        if(outputindex[term] != curoutindex[term]){
            tempchar = outputbuffer[term][curoutindex[term]];
            if(tempchar == '\n' && statenewline[term] == ACTIVE){
                statenewline[term] = IDLE;
                WriteDataRegister(term, '\r');
            }
            else if(tempchar == '\n'){
                statenewline[term] = ACTIVE;
                curoutindex[term]++;
                WriteDataRegister(term, tempchar);
            }
            else{
                curoutindex[term]++;
                WriteDataRegister(term, tempchar);
            }

        }
        else{
            statenewchar[term] = ACTIVE;
            statebusy[term] = IDLE;
            CondSignal(condbusy[term]);
        }
    }
    // output next


    if(tempchar != '\0'){
        statarr[term].tty_out++;

    }

};

void ReceiveInterrupt(int term){

    // shouldn't use a condition variable to wait

    Declare_Monitor_Entry_Procedure();

    // get char typed
    char typed = ReadDataRegister(term);
    statarr[term].tty_in++;

    // echo & input
    if(typed == '\r'){
        inputbuffer[term][inputindex[term]] = '\n';
        inputindex[term] = (inputindex[term]+1)%INPUT_SIZE;
    }
    else{
        inputbuffer[term][inputindex[term]] = typed;
        inputindex[term] = (inputindex[term]+1)%INPUT_SIZE;
    }

    echobuffer[term][echoindex[term]] = typed;
    echoindex[term] = (echoindex[term]+1)%ECHO_SIZE;

    if(typed == '\b' || typed == '\177'){
        echobuffer[term][echoindex[term]] = ' ';
        echoindex[term] = (echoindex[term]+1)%ECHO_SIZE;
        echobuffer[term][echoindex[term]] = '\b';
        echoindex[term] = (echoindex[term]+1)%ECHO_SIZE;
        if(inputbuffer[term][(inputindex[term]-2)%INPUT_SIZE] != '\n' &&
                inputbuffer[term][(inputindex[term]-2)%INPUT_SIZE] != '\0')
            inputindex[term] = (inputindex[term]-2)%INPUT_SIZE;
        else{
            inputindex[term] = (inputindex[term]-1)%INPUT_SIZE;
        }
    }
    else if(typed == '\r'){
        echobuffer[term][echoindex[term]] = '\n';
        echoindex[term] = (echoindex[term]+1)%ECHO_SIZE;
        numlines[term]++;
        CondSignal(condline[term]);
    }

    if(statenewchar[term] == ACTIVE){
        stateecho[term] = ACTIVE;
        statenewchar[term] = IDLE;
        curechindex[term]++;
        if(statebusy[term] == IDLE)
            WriteDataRegister(term, typed);
    }


};


int WriteTerminal(int term, char *buf, int buflen){
    Declare_Monitor_Entry_Procedure();

    if(term < 0 || term >= NUM_TERMINALS) return -1;

    // not initialized
    if(terminalinit[term] == -1) return -1;// how about buffer size?

    if(buflen == 0) return 0;

    if(buflen > 0){

        while(statewrite[term] == ACTIVE) CondWait(condwrite[term]);
        statenewchar[term] = IDLE;
        statewrite[term] = ACTIVE;

        outputbuffer[term] = buf;
        outputindex[term] = buflen;
        curoutindex[term] = 1;

        // to be continue
        while(stateecho[term] == ACTIVE) CondWait(condecho[term]);
        statebusy[term] = ACTIVE;
        WriteDataRegister(term, outputbuffer[term][0]);
        while(statebusy[term] == ACTIVE) CondWait(condbusy[term]);


        statewrite[term] = IDLE;
        statarr[term].user_in += buflen;
        CondSignal(condwrite[term]);
        return buflen;

    }
    else{
        return -1;
    }

};


int ReadTerminal(int term, char *buf, int buflen){
    Declare_Monitor_Entry_Procedure();
    int len;
    char tempchar = '\0';

    if(term < 0 || term >= NUM_TERMINALS) return -1;
    // not initialized
    if(terminalinit[term] == -1) return -1; // how about buffer size?

    if(buflen >= 0){

        // mesa
        while(stateread[term] == ACTIVE) CondWait(condread[term]);
        stateread[term] = ACTIVE;

        while(numlines[term] == 0) CondWait(condline[term]);
        numlines[term]--;

        for(len = 0; len < buflen; len++){
            tempchar = inputbuffer[term][curinputindex[term]];
            curinputindex[term] = (curinputindex[term] + 1) % INPUT_SIZE;
//            printf("%s\n","ininin");
            buf[len] = tempchar;

            if(tempchar == '\n'){
                break;
            }
        }

        if(tempchar != '\n'){
            numlines[term]++;
            CondSignal(condline[term]);
        }
        stateread[term] = IDLE;
        statarr[term].user_out += len;
        CondSignal(condread[term]);
        return len;
    }
    else{
        return -1;
    }

};


int InitTerminal(int term){
    Declare_Monitor_Entry_Procedure();

    // initialize values for each terminal
    if(term < 0 || term >= NUM_TERMINALS){
        printf("%s", "Parameter wrong.");
        return -1;
    }
    if(terminalinit[term] == 0){
        printf("%s", "Already initialized.");
        return -1;
    }

    int temp = InitHardware(term);

    if(temp == 0){

        // change state
        terminalinit[term] = 0;
        condwrite[term] = CondCreate();
        condread[term] = CondCreate();
        condline[term] = CondCreate();
        condbusy[term] = CondCreate();
        condecho[term] = CondCreate();

        numlines[term] = 0;
        statewrite[term] = IDLE;
        stateread[term] = IDLE;
        statenewline[term] = ACTIVE;
        statenewchar[term] = ACTIVE;
        statebusy[term] = IDLE;
        stateecho[term] = IDLE;

        echoindex[term] = 0;
        curechindex[term] = 0;
        inputindex[term] = 0;
        outputindex[term] = 0;
        curoutindex[term] = 0;
        curinputindex[term] = 0;

        // statistics
        statarr[term].tty_in = 0;
        statarr[term].tty_out = 0;
        statarr[term].user_in = 0;
        statarr[term].user_out = 0;
    }


    return temp;
};


int TerminalDriverStatistics(struct termstat *stats){
    Declare_Monitor_Entry_Procedure();
    int i;
    for(i = 0; i < NUM_TERMINALS; i++){
        stats[i].tty_in = statarr[i].tty_in;
        stats[i].tty_out = statarr[i].tty_out;
        stats[i].user_in = statarr[i].user_in;
        stats[i].user_out = statarr[i].user_out;
    }

    return 0;
};


int InitTerminalDriver(void){
    Declare_Monitor_Entry_Procedure();

    // set statistics to -1
    int i;
    for(i = 0; i < NUM_TERMINALS; i++){
        terminalinit[i] = -1;
        statarr[i].tty_in = -1;
        statarr[i].tty_out = -1;
        statarr[i].user_in = -1;
        statarr[i].user_out = -1;
    }

    return 0;
};
