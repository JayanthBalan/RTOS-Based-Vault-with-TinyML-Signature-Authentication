
#ifndef COMM_OPS_H
#define COMM_OPS_H

//Libraries
#include "type_dat.h"
#include <Wire.h>

//Function Prototypes
void comms_init();

#define CHUNK_SIZE 30

//Global Variables
extern volatile char a, b;
extern uint16_t expec_size;
extern uint16_t rx_chunk;
extern bool received_header;
extern bool tx_fin;

#endif
