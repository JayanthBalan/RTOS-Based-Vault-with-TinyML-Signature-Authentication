
#ifndef TYPE_DAT_H
#define TYPE_DAT_H

#include <Arduino.h>
#include <vector>
using namespace std;

//Signature Object
struct sigpoints
{
    uint16_t xpos;
    uint16_t ypos;
    uint16_t zpos;
};

extern vector<sigpoints> sign;
extern int df_pass, df_fail;

//Global Task handles
extern TaskHandle_t read_dat_handle;
extern TaskHandle_t vec_process_handle;

//Global Queue handle
extern QueueHandle_t sig_transfer_Q;

//Global Mutex handle
extern SemaphoreHandle_t sign_lock;

#endif
