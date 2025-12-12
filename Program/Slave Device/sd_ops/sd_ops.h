
#ifndef SD_OPS_H
#define SD_OPS_H

//Libraries
#include <type_dat.h>
#include <SPI.h>
#include <SD.h>
#include "knn_dtw_ops.h"
#include "comm_ops.h"

//Function Prototypes
void append_dat(const vector<sigpoints>&, bool);
void read_dat(void*);
void train_proc_1(void*);
void train_proc_2(void*);
void class_task1(void*);
void class_task2(void*);
void sd_init();
void clear_dat();
void dataset_trim(bool);
void performance_check();

//Global Variables
const int CS = 5;

#endif
