
#include "type_dat.h"
#include "comm_ops.h"
#include "knn_dtw_ops.h"
#include "sd_ops.h"

TaskHandle_t read_dat_handle = NULL;
TaskHandle_t vec_process_handle = NULL;
TaskHandle_t wait_task_handle = NULL;
QueueHandle_t sig_transfer_Q = NULL;
SemaphoreHandle_t sign_lock;

void wait_task(void* param)
{
  if(xSemaphoreTake(sign_lock, portMAX_DELAY) == pdTRUE)
  {
    Serial.println("lock::: Shared Resource (Sign) Locked");
  }

  while(!tx_fin) {
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
  tx_fin = false;
  Serial.println("lock::: Shared Resource (Sign) Unlocked");
  xSemaphoreGive(sign_lock);
  vTaskDelete(NULL);
}

void vec_process(void* param) {
  while(1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    Serial.println("\nproc::: Processing Task");
    
    //Resume SD Read
    Serial.println("proc::: Trigger DTW computation");
    vTaskResume(read_dat_handle);
    
    //DTW Results
    vector<float>* dtw_new = nullptr;
    vector<vector<float>> train_pass, train_fail;
    
    //Training Data
    vector<float>* train_pattern = nullptr;
    int train_count = 0;
    vTaskDelay(10/portTICK_PERIOD_MS);
    while(train_count < df_pass)
    {
      if(xQueueReceive(sig_transfer_Q, &train_pattern, pdMS_TO_TICKS(10000)) == pdPASS) {
        train_count++;
        Serial.print("proc::: Received training pattern (");
        Serial.print(train_pattern->size());
        Serial.println(" values)");
        
        train_pass.push_back(*train_pattern);
        delete train_pattern;
        
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    
    train_count = 0;
    while(train_count < df_fail)
    {
      if(xQueueReceive(sig_transfer_Q, &train_pattern, pdMS_TO_TICKS(10000)) == pdPASS) {
        train_count++;
        Serial.print("proc::: Received training pattern (");
        Serial.print(train_pattern->size());
        Serial.println(" values)");
        
        train_fail.push_back(*train_pattern);
        delete train_pattern;
        
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    
    Serial.print("\nproc::: Total training patterns = ");
    Serial.println(train_pass.size() + train_fail.size());

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for Queue

    //New Data
    if(xQueueReceive(sig_transfer_Q, &dtw_new, pdMS_TO_TICKS(5000)) != pdPASS) {
      Serial.println("proc::: Error - No DTW data received");
      continue;
    }
    
    Serial.print("proc::: Received new DTW (");
    Serial.print(dtw_new->size());
    Serial.println(" values)");

    xSemaphoreGive(sign_lock);
    Serial.println("proc::: Mutex sign_lock unlocked");

    Serial.print("proc::: New signature: = ");
    Serial.print(sign.size());
    Serial.println(" points");
    
    //Classify
    float con;
    bool knn_result = knn_classify(&con, *dtw_new, train_pass, train_fail, train_pass.size(), train_fail.size());
    
    delete dtw_new;  //Cleanup

    noInterrupts();
    if(knn_result) {
      a = 'y';
      b = '1';
    } else {
      a = 'n';
      b = '1';
    }
    interrupts();
    
    if(knn_result && con > CONFIDENCE_THRESHOLD) {
      Serial.println("proc::: Pass");
      vTaskDelay(pdMS_TO_TICKS(10));
      append_dat(sign, true);
      Serial.println("proc::: Saved to SD");
    }
    else if(!knn_result && con > CONFIDENCE_THRESHOLD){
      Serial.println("proc::: Fail");
      vTaskDelay(pdMS_TO_TICKS(10));
      append_dat(sign, false);
      Serial.println("proc::: Saved to SD");
    }
    else {
      Serial.println("proc::: Not Saved to SD; No Confidence");
    }
    
    Serial.println("proc::: Processing Complete\n");
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nSystem Boot");

  sig_transfer_Q = xQueueCreate(20, sizeof(vector<float>*));
  if(sig_transfer_Q == NULL) {
    Serial.println("Error: Failed to create queue");
    while(1);
  }
  Serial.println("Queue created");
    
  //Initialize I2C communication
  comms_init();
  delay(50);
    
  //Initialize SD card
  sd_init();
  delay(50);

  //Initialize TinyML
  knn_init();
  delay(50);

  //Mutex
  sign_lock = xSemaphoreCreateMutex();

  //Training
  while(!Serial.available())
  {
    delay(100);
  }
  if(Serial.available())
  {
    char command = Serial.read();
    if(command == 'f')
    {
      while(!tx_fin)
      {
        delay(200);
      }
      tx_fin = false;

      append_dat(sign, false);

      delay(10);
      ESP.restart();
    }
    else if(command == 'p')
    {
      while(!tx_fin)
      {
        delay(200);
      }
      tx_fin = false;

      append_dat(sign, true);

      delay(10);
      ESP.restart();
    }
    else if(command == 't')
    {
      performance_check();
      delay(10000);
      ESP.restart();
    }
    else if(command == 'x')
    {
      clear_dat();
      delay(10);
      ESP.restart();
    }
    else{

    }
  }

  xTaskCreatePinnedToCore(wait_task, "Input_Sign_Wait", 4000, NULL, 1, &wait_task_handle, 0);
  xTaskCreatePinnedToCore(read_dat, "Read_SD_File", 12000, NULL, 2, &read_dat_handle, 0);
  xTaskCreatePinnedToCore(vec_process, "Vector_Process", 15000, NULL, 2, &vec_process_handle, 0);

  Serial.println("Tasks created");
  Serial.println("System Ready\n");

  xTaskNotifyGive(vec_process_handle);
  
  while(a == 'w' && b == '0') {
    delay(50);
  }
    
  Serial.print("Response ready: ");
  Serial.print(a);
  Serial.println(b);

  performance_check();

  delay(2000);
  ESP.restart();
}

void loop() {
}