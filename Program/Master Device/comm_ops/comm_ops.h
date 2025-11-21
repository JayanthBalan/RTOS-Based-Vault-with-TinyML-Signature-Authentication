
#ifndef COMM_OPS_H
#define COMM_OPS_H

//Libraries
#include <Wire.h>
#include "type_dat.h"

//Function Prototypes
void comms_init();
void rx_comm(char*, char*);
void tx_comm(vector<sigpoints>&);

#endif
