
#include "vault_ops.h"

const String path = "/Password_Archive.bin";
const String common_key = "Secret101!!";

//XOR Encrypt
void xor_encrypt(String inpass[]) {
    String key = "1Secret101!!y";
    uint8_t len = MAX_STORAGE;
    for(int j = 0; j < len; j++)
    {
        String output = "";
        for(int i = 0; i < inpass[j].length(); i++) {
            char encrypted = inpass[j][i] ^ key[i % 13];
            output += encrypted;
        }
        inpass[j] = output;
    }
    key = "";
}

//XOR Decrypt
void xor_decrypt(String inpass[], char key1, char key2) {
    String key = key2 + common_key + key1;
    uint8_t len = MAX_STORAGE;
    for(int j = 0; j < len; j++)
    {
        String output = "";
        for(int i = 0; i < inpass[j].length(); i++) {
            
            char encrypted = inpass[j][i] ^ key[i % 13];
            output += encrypted;
        }
        inpass[j] = output;
    }
    key = "";
}

void archive_spiffs(String passwords[]) {
    if(!SPIFFS.begin(true)) {
        Serial.println("spiffs::: Mount Failed");
        return;
    }
    
    File file = SPIFFS.open(path, FILE_WRITE);
    if(!file) {
        Serial.println("spiffs::: Failed file open");
        return;
    }

    for(int i = 0; i < 50; i++) {
        uint16_t len = passwords[i].length();
        file.write((uint8_t*)&len, sizeof(uint16_t)); // Write length
        file.write((uint8_t*)passwords[i].c_str(), len);
    }
    
    file.close();
    SPIFFS.end();
    Serial.println("spiffs::: Saved");
}

void extract_spiffs(String passwords[]) {
    if(!SPIFFS.begin(true)) {
        Serial.println("spiffs::: Mount Failed");
        return;
    }
    
    if(!SPIFFS.exists(path))
    {
        File file = SPIFFS.open(path, FILE_WRITE);
        file.close();
        SPIFFS.end();
        return;
    }
    File file = SPIFFS.open(path, FILE_READ);
    if(!file) {
        Serial.println("spiffs::: Empty file");
        file.close();
        SPIFFS.end();
        return;
    }
    
    int i = 0;
    while(file.available() && i < 50) {
        uint16_t len = 0;
        file.read((uint8_t*)&len, sizeof(uint16_t)); // Read length
        
        if(len > 0 && len < 1000) {
            char buffer[1001];
            file.read((uint8_t*)buffer, len);
            buffer[len] = '\0';
            passwords[i] = String(buffer);
            i++;
        }
    }
    
    file.close();
    SPIFFS.end();
    Serial.println("spiffs::: Loaded");
}
