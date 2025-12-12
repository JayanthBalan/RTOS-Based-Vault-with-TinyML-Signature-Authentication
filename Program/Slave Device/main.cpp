
#include "type_dat.h"
#include "comm_ops.h"
#include "knn_dtw_ops.h"
#include "sd_ops.h"

TaskHandle_t read_dat_handle = NULL;
TaskHandle_t vec_process_handle = NULL;
TaskHandle_t wait_task_handle = NULL;
TaskHandle_t train_proc_1_handle = NULL;
TaskHandle_t train_proc_2_handle = NULL;
TaskHandle_t class_task1_handle = NULL;
TaskHandle_t class_task2_handle = NULL;
QueueHandle_t sig_transfer_Q = NULL;
QueueHandle_t sig_transfer_Q1 = NULL;
QueueHandle_t sig_transfer_Q2 = NULL;
SemaphoreHandle_t sign_lock;

KNNClassifier signatureKNN(KNN_FEATURES);

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
    float features1[KNN_FEATURES], features2[KNN_FEATURES];
    
    //Training Data
    vector<float>* train_pattern = nullptr;
    int train_count1 = 0, train_count2 = 0;
    vTaskDelay(10/portTICK_PERIOD_MS);

    //Train all patterns
    Serial.println("knn::: Training model");
    Serial.print("knn::: Model trained with ");
    Serial.print(signatureKNN.getCount());
    Serial.println(" examples");
    while(train_count1 < df_pass || train_count2 < df_fail)
    {
      if(xQueueReceive(sig_transfer_Q1, &train_pattern, pdMS_TO_TICKS(10000)) == pdPASS) {
        train_count1++;
        Serial.print("proc::: Received training pattern (");
        Serial.print(train_pattern->size());
        Serial.println(" values)");

        extract_features(*train_pattern, features1);
        signatureKNN.addExample(features1, GENUINE_CLASS);

        delete train_pattern;
        
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      if(xQueueReceive(sig_transfer_Q2, &train_pattern, pdMS_TO_TICKS(10000)) == pdPASS) {
        train_count2++;
        Serial.print("proc::: Received training pattern (");
        Serial.print(train_pattern->size());
        Serial.println(" values)");
        
        extract_features(*train_pattern, features2);
        signatureKNN.addExample(features2, FORGERY_CLASS);

        delete train_pattern;
        
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    }
    
    Serial.print("\nproc::: Total training patterns = ");
    Serial.println(train_count1 + train_count2);

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for Queue

    //New Data
    if(xQueueReceive(sig_transfer_Q, &dtw_new, pdMS_TO_TICKS(5000)) != pdPASS) {
      Serial.println("proc::: Error - No DTW data received in Queue 1");
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
    bool knn_result = knn_classify(&con, *dtw_new, train_count1, train_count2);
    
    delete dtw_new;  //Cleanup

    noInterrupts();
    if(knn_result && con > CONFIDENCE_THRESHOLD) {
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
  esp_task_wdt_init(20, true);
  delay(500);
  Serial.println("\nSystem Boot");

  sig_transfer_Q = xQueueCreate(1, sizeof(vector<float>*));
  sig_transfer_Q1 = xQueueCreate(20, sizeof(vector<float>*));
  sig_transfer_Q2 = xQueueCreate(20, sizeof(vector<float>*));
  if(sig_transfer_Q == NULL || sig_transfer_Q1 == NULL || sig_transfer_Q2 == NULL) {
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
    if(command == 'f') //Append as Forgery
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
    else if(command == 'p') //Append as Genuine
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
    else if(command == 't') //Performance Test
    {
      performance_check();
      delay(10000);
      ESP.restart();
    }
    else if(command == 'x') //Restart and clear data
    {
      clear_dat();
      delay(10);
      ESP.restart();
    }
    else{

    }
  }

  xTaskCreatePinnedToCore(wait_task, "Input_Sign_Wait", 4000, NULL, 1, &wait_task_handle, 1);
  xTaskCreatePinnedToCore(read_dat, "Read_SD_File", 12000, NULL, 2, &read_dat_handle, 1);
  xTaskCreatePinnedToCore(vec_process, "Vector_Process", 15000, NULL, 2, &vec_process_handle, 0);
  xTaskCreatePinnedToCore(train_proc_1, "Training_Process_1", 12000, NULL, 3, &train_proc_1_handle, 0);
  xTaskCreatePinnedToCore(train_proc_2, "Training_Process_2", 12000, NULL, 3, &train_proc_2_handle, 1);
  xTaskCreatePinnedToCore(class_task1, "Classification_Task_1", 8000, NULL, 3, &class_task1_handle, 0);
  xTaskCreatePinnedToCore(class_task2, "Classification_Task_2", 8000, NULL, 3, &class_task2_handle, 1);

  Serial.println("Tasks created");
  Serial.println("System Ready\n");

  xTaskNotifyGive(vec_process_handle);
  
  while(a == 'w' && b == '0') {
    delay(50);
  }
    
  Serial.print("Response ready: ");
  Serial.print(a);
  Serial.println(b);

  dataset_trim(false);
  dataset_trim(true);

  delay(3000);
  ESP.restart();
}

void loop() {
}
