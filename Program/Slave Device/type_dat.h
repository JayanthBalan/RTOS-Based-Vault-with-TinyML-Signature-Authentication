
#ifndef TYPE_DAT_H
#define TYPE_DAT_H

#include <Arduino.h>
#include <vector>
#include <Arduino_KNN.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
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

#define KNN_FEATURES 5

//Global Task handles
extern TaskHandle_t read_dat_handle;
extern TaskHandle_t vec_process_handle;
extern TaskHandle_t train_proc_1_handle;
extern TaskHandle_t train_proc_2_handle;
extern KNNClassifier signatureKNN;
extern TaskHandle_t class_task1_handle;
extern TaskHandle_t class_task2_handle;

//Global Queue handle
extern QueueHandle_t sig_transfer_Q;
extern QueueHandle_t sig_transfer_Q1;
extern QueueHandle_t sig_transfer_Q2;

//Global Mutex handle
extern SemaphoreHandle_t sign_lock;

#endif
