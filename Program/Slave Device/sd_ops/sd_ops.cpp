#include "sd_ops.h"

File myFile;
const char* path0 = "/NexRoot/signat0.bin";
const char* path1 = "/NexRoot/signat1.bin";
int df_pass, df_fail;


int count_signatures(bool sel_fil) {
    const char* path = (sel_fil) ? path1 : path0;
    File file = SD.open(path, FILE_READ);
    if(!file) return 0;
    
    int count = 0;
    while(file.available()) {
        uint16_t vec_size = 0;
        if(file.read((uint8_t*)&vec_size, sizeof(uint16_t)) != sizeof(uint16_t)) {
            break;
        }
        if(vec_size == 0 || vec_size > 20000) {
            break;
        }

        file.seek(file.position() + vec_size * sizeof(sigpoints));
        count++;
    }
    
    file.close();
    return count;
}

vector<sigpoints>* sign_index(int index, bool sel_fil) {
    const char* path = (sel_fil) ? path1 : path0;
    File file = SD.open(path, FILE_READ);
    if(!file){
        return nullptr;
    }
    
    int curr_idx = 0;
    vector<sigpoints>* sig = nullptr;
    
    while(file.available() && curr_idx <= index) {
        uint16_t vec_size = 0;
        if(file.read((uint8_t*)&vec_size, sizeof(uint16_t)) != sizeof(uint16_t)) {
            break;
        }
        if(vec_size == 0 || vec_size > 20000) {
            break;
        }
        
        if(curr_idx == index) {
            sig = new vector<sigpoints>();
            sig->resize(vec_size);
            
            size_t data_bytes = vec_size * sizeof(sigpoints);
            if(file.read((uint8_t*)sig->data(), data_bytes) != data_bytes) {
                delete sig;
                sig = nullptr;
                break;
            }
            break;
        } else {
            file.seek(file.position() + vec_size * sizeof(sigpoints));
        }
        
        curr_idx++;
    }
    
    file.close();
    return sig;
}

void sd_init(){
    delay(5);
    
    if (!SD.begin(CS)) {
        Serial.println("sd::: Init failed");
        return;
    }
    
    if(!SD.exists(path0)) {
        File file0 = SD.open(path0, FILE_WRITE);
        if(!file0){
            Serial.println("sd::: Create failed 0");
            return;
        }
        file0.close();
        Serial.println("sd::: Created new file 0");
    }
    if(!SD.exists(path1)) {
        File file1 = SD.open(path1, FILE_WRITE);
        if(!file1){
            Serial.println("sd::: Create failed 1");
            return;
        }
        file1.close();
        Serial.println("sd::: Created new file 1");
    }

    df_pass = count_signatures(true);
    df_fail = count_signatures(false);
    Serial.print("sd::: Pass signatures = ");
    Serial.println(df_pass);
    Serial.print("sd::: Fail signatures = ");
    Serial.println(df_fail);
    
    Serial.println("sd::: Initialized");
}

void append_dat(const vector<sigpoints> &sigvec, bool path_sel){
    Serial.print("sd::: Appending ");
    Serial.print(sigvec.size());
    Serial.println(" points");
    
    const char *path = (path_sel) ? path1 : path0; 
    File file = SD.open(path, FILE_APPEND);
    if(!file){
        Serial.println("sd::: Append Failed");
        return;
    }

    uint16_t vecsize = sigvec.size();
    file.write((uint8_t*)&vecsize, sizeof(uint16_t));
    size_t written = file.write((uint8_t*)sigvec.data(), vecsize * sizeof(sigpoints));
    file.close();
    
    char ix = *(path + 15);

    if(written == vecsize * sizeof(sigpoints)){
        Serial.print("sd::: Append Success at path: ");
        Serial.println(ix);
    } else {
        Serial.print("sd::: Append Failure at path: ");
        Serial.println(ix);
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));
}

void read_dat(void* param){
    while(1) {
        vTaskSuspend(NULL);
        
        Serial.println("\nsd::: DTW Computation");
        
        Serial.print("sd::: Stored genuine signatures: ");
        Serial.println(df_pass);

        Serial.print("sd::: Stored forgery signatures: ");
        Serial.println(df_fail);
        
        if(df_pass == 0) {
            Serial.println("sd::: Empty file - initial dataset");
            
            vector<float>* empty_new = new vector<float>();
            xQueueSend(sig_transfer_Q, &empty_new, portMAX_DELAY);
            
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if(df_fail == 0) {
            Serial.println("sd::: Empty file - initial dataset");
            
            vector<float>* empty_new = new vector<float>();
            xQueueSend(sig_transfer_Q, &empty_new, portMAX_DELAY);
            
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        
        if(df_pass >= 2) {
            Serial.println("\nsd::: Compute training patterns: Genuine Class");
            
            for(int i = 0; i < df_pass; i++) {
                Serial.print("sd::: Training for Stored[");
                Serial.print(i);
                Serial.println("]");
                
                vector<float>* pattern = new vector<float>();
                
                vector<sigpoints>* sig_i = sign_index(i, true);
                if(sig_i == nullptr) {
                    delete pattern;
                    Serial.println("sd::: ERROR reading sig_i");
                    break;
                }
                
                for(int j = 0; j < df_pass; j++) {
                    if(i != j) {
                        vector<sigpoints>* sig_j = sign_index(j, true);
                        if(sig_j == nullptr) {
                            Serial.println("sd::: ERROR reading sig_j");
                            continue;
                        }
                        
                        float dist = dtw_distance(*sig_i, *sig_j);
                        pattern->push_back(dist);
                        
                        delete sig_j;
                        vTaskDelay(pdMS_TO_TICKS(5));
                        yield();
                    }
                }
                for(int j = 0; j < df_fail; j++) {
                    if(i != j) {
                        vector<sigpoints>* sig_j = sign_index(j, false);
                        if(sig_j == nullptr) {
                            Serial.println("sd::: ERROR reading sig_j");
                            continue;
                        }
                        
                        float dist = dtw_distance(*sig_i, *sig_j);
                        pattern->push_back(dist);
                        
                        delete sig_j;
                        delay(pdMS_TO_TICKS(5));
                        yield();
                    }
                }
                
                delete sig_i;

                while(xQueueSend(sig_transfer_Q, &pattern, pdMS_TO_TICKS(100)) != pdPASS) {
                    Serial.println("sd::: Queue full");
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
            }
        }
        else {
            vector<float>* dummy = new vector<float>();
            dummy->push_back(0.0);
            xQueueSend(sig_transfer_Q, &dummy, portMAX_DELAY);
            Serial.println("sd::: Dummy pattern");
        }

        if(df_fail >= 2) {
            Serial.println("\nsd::: Compute training patterns: Forgery Class");
            
            for(int i = 0; i < df_fail; i++) {
                Serial.print("sd::: Training for Stored[");
                Serial.print(i);
                Serial.println("]");
                
                vector<float>* pattern = new vector<float>();
                
                vector<sigpoints>* sig_i = sign_index(i, false);
                if(sig_i == nullptr) {
                    delete pattern;
                    Serial.println("sd::: ERROR reading sig_i");
                    break;
                }
                
                for(int j = 0; j < df_pass; j++) {
                    if(i != j) {
                        vector<sigpoints>* sig_j = sign_index(j, true);
                        if(sig_j == nullptr) {
                            Serial.println("sd::: ERROR reading sig_j");
                            continue;
                        }
                        
                        float dist = dtw_distance(*sig_i, *sig_j);
                        pattern->push_back(dist);
                        
                        delete sig_j;
                        vTaskDelay(pdMS_TO_TICKS(5));
                        yield();
                    }
                }
                for(int j = 0; j < df_fail; j++) {
                    if(i != j) {
                        vector<sigpoints>* sig_j = sign_index(j, false);
                        if(sig_j == nullptr) {
                            Serial.println("sd::: ERROR reading sig_j");
                            continue;
                        }
                        
                        float dist = dtw_distance(*sig_i, *sig_j);
                        pattern->push_back(dist);
                        
                        delete sig_j;
                        vTaskDelay(pdMS_TO_TICKS(5));
                        yield();
                    }
                }
                
                delete sig_i;

                while(xQueueSend(sig_transfer_Q, &pattern, pdMS_TO_TICKS(100)) != pdPASS) {
                    Serial.println("sd::: Queue full");
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
            }
        }
        else {
            vector<float>* dummy = new vector<float>();
            dummy->push_back(0.0);
            xQueueSend(sig_transfer_Q, &dummy, portMAX_DELAY);
            Serial.println("sd::: Dummy pattern");
        }

        while(xSemaphoreTake(sign_lock, portMAX_DELAY) != pdTRUE)
        {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        Serial.println("sd::: Mutex sign_lock locked");

        Serial.println("sd::: Computing new signature DTW.");
        vector<float>* dtw_new = new vector<float>();
        
        for(int i = 0; i < df_pass; i++) {
            Serial.print("sd::: New vs Genuine Stored[");
            Serial.print(i);
            Serial.println("]");
            
            vector<sigpoints>* stored_sig = sign_index(i, true);
            if(stored_sig == nullptr) {
                Serial.println("sd::: Error reading signature");
                break;
            }
            
            float dist = dtw_distance(sign, *stored_sig);
            dtw_new->push_back(dist);
            
            delete stored_sig;
            vTaskDelay(pdMS_TO_TICKS(10));
            yield();
        }
        for(int i = 0; i < df_fail; i++) {
            Serial.print("sd::: New vs Forgery Stored[");
            Serial.print(i);
            Serial.println("]");
            
            vector<sigpoints>* stored_sig = sign_index(i, false);
            if(stored_sig == nullptr) {
                Serial.println("sd::: Error reading signature");
                break;
            }
            
            float dist = dtw_distance(sign, *stored_sig);
            dtw_new->push_back(dist);
            
            delete stored_sig;
            vTaskDelay(pdMS_TO_TICKS(10));
            yield();
        }
        
        while(xQueueSend(sig_transfer_Q, &dtw_new, pdMS_TO_TICKS(100)) != pdPASS) {
            Serial.println("sd::: Queue full");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        xSemaphoreGive(sign_lock);
        Serial.println("sd::: Mutex sign_lock unlocked");
        Serial.println("sd::: New DTW");
        xTaskNotifyGive(vec_process_handle);
        
        Serial.print("sd::: Total: 1 new + ");
        Serial.print(df_pass + df_fail);
        Serial.println(" dataset");
        Serial.println("sd::: Completed SD Transactions\n");
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void clear_dat()
{
    if(SD.exists(path0)) {
        Serial.print("sd::: Removing File 0");
        if(SD.remove(path0)) {
            Serial.println("sd::: File 0 removed.");

            File file = SD.open(path0, FILE_WRITE);
            if(file)
            {
                Serial.println("sd::: Empty file 0 created");
                delay(50);
                file.close();
            }
            else{
                Serial.println("sd::: File 0 not created");
            }
        }
        else {
            Serial.println("sd::: Error removing file 0.");
        }
    }
    if(SD.exists(path1)) {
        Serial.print("sd::: Removing File 1");
        if(SD.remove(path1)) {
            Serial.println("sd::: File 1 removed.");

            File file = SD.open(path1, FILE_WRITE);
            if(file)
            {
                Serial.println("sd::: Empty file 1 created");
                delay(50);
                file.close();
            }
            else{
                Serial.println("sd::: File 1 not created");
            }
        }
        else {
            Serial.println("sd::: Error removing file 1.");
        }
    }

}

void performance_check()
{
    vector<vector<float>> train_pass, train_fail, test_pass, test_fail;

    if(df_pass == 0) {            
        vector<float>* empty_new = new vector<float>();
        xQueueSend(sig_transfer_Q, &empty_new, portMAX_DELAY);
            
        vTaskDelay(pdMS_TO_TICKS(10));
        return;
    }
    if(df_fail == 0) {            
        vector<float>* empty_new = new vector<float>();
        xQueueSend(sig_transfer_Q, &empty_new, portMAX_DELAY);
            
        vTaskDelay(pdMS_TO_TICKS(10));
        return;
    }
        
    if(df_pass >= 2) {            
        for(int i = 0; i < df_pass; i++) {                
            vector<float>* pattern = new vector<float>();
                
            vector<sigpoints>* sig_i = sign_index(i, true);
            if(sig_i == nullptr) {
                delete pattern;
                break;
            }
                
            for(int j = 0; j < df_pass; j++) {
                if(i != j) {
                    vector<sigpoints>* sig_j = sign_index(j, true);
                    if(sig_j == nullptr) {
                        continue;
                   }
                        
                    float dist = dtw_distance(*sig_i, *sig_j);
                    pattern->push_back(dist);
                        
                    delete sig_j;
                    delay(10);
                }
            }
            for(int j = 0; j < df_fail; j++) {
                if(i != j) {
                    vector<sigpoints>* sig_j = sign_index(j, false);
                    if(sig_j == nullptr) {
                        continue;
                    }
                        
                    float dist = dtw_distance(*sig_i, *sig_j);
                    pattern->push_back(dist);
                        
                    delete sig_j;
                    delay(10);
                }
            }
                
            delete sig_i;
            if(i%2 == 0)
            {
                train_pass.push_back(*pattern);
            }
            else {
                test_pass.push_back(*pattern);
            }
        }
    }

    if(df_fail >= 2) {            
        for(int i = 0; i < df_fail; i++) {                
            vector<float>* pattern = new vector<float>();
            
            vector<sigpoints>* sig_i = sign_index(i, false);
            if(sig_i == nullptr) {
                delete pattern;
                break;
            }
                
            for(int j = 0; j < df_pass; j++) {
                if(i != j) {
                    vector<sigpoints>* sig_j = sign_index(j, true);
                    if(sig_j == nullptr) {
                        continue;
                    }
                        
                    float dist = dtw_distance(*sig_i, *sig_j);
                    pattern->push_back(dist);
                    
                    delete sig_j;
                    delay(10);
                }
            }
            for(int j = 0; j < df_fail; j++) {
                if(i != j) {
                    vector<sigpoints>* sig_j = sign_index(j, false);
                    if(sig_j == nullptr) {
                        continue;
                    }
                        
                    float dist = dtw_distance(*sig_i, *sig_j);
                    pattern->push_back(dist);
                        
                    delete sig_j;
                    delay(10);
                }
            }
                
            delete sig_i;
            if(i%2 == 0)
            {
                train_fail.push_back(*pattern);
            }
            else {
                test_fail.push_back(*pattern);
            }
        }
    }

    float con;
    uint16_t TP = 0, TN = 0, FP = 0, FN = 0;
    for(int i = 0; i < test_pass.size(); i++)
    {
        bool res = knn_classify(&con, test_pass[i], train_pass, train_fail, train_pass.size(), train_fail.size());
        if(res)
        {
            TP++;
        }
        else {
            FN++;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
        yield();
    }
    for(int i = 0; i < test_fail.size(); i++)
    {
        bool res = knn_classify(&con, test_fail[i], train_pass, train_fail, train_pass.size(), train_fail.size());
        if(!res)
        {
            TN++;
        }
        else {
            FP++;
        }
        vTaskDelay(pdMS_TO_TICKS(30));
        yield();
    }

    float accuracy = (TP + TN)/float(TP + TN + FP + FN);
    float precision = TP/float(TP + FP);
    float recall = TP/float(TP + FN);
    float f1 = 2*(precision*recall)/float(precision + recall);

    float far = FP/float(FP + TN);
    float frr = FN/float(TP + FN);

    Serial.println("\nsd::: Performance Metrics");
    Serial.print("sd::: TP = ");
    Serial.print(TP);
    Serial.print("; TN = ");
    Serial.print(TN);
    Serial.print("; FP = ");
    Serial.print(FP);
    Serial.print("; FN = ");
    Serial.println(FN);

    Serial.print("sd::: Accuracy = ");
    Serial.println(accuracy);
    Serial.print("sd::: Precision = ");
    Serial.println(precision);
    Serial.print("sd::: Recall = ");
    Serial.println(recall);
    Serial.print("sd::: FAR = ");
    Serial.println(far);
    Serial.print("sd::: FRR = ");
    Serial.println(frr);
    Serial.print("sd::: F1 = ");
    Serial.println(f1);

    Serial.println("\n");
}
