
#include "comm_ops.h"

void comms_init()
{
    Wire.begin(); //Master Mode
    Serial.println("comm_ini::: I2C Initialized");
}

void rx_comm(char* alpha, char* numera)
{
    byte msg = Wire.requestFrom(8, 2);
    delay(5);
    if (msg == 2) {
        *alpha = Wire.read();
        *numera = Wire.read();
    } 
    else {
        *alpha = *numera = 'x';
    }
}

void tx_comm(vector<sigpoints> &sign)
{
    if(sign.empty())
    {
        Serial.println("comm_tx::: No Signature");
        return;
    }

    uint16_t vec_size = sign.size();

    Wire.beginTransmission(8);
    Wire.write(0xFF);  //Header marker
    Wire.write(vec_size >> 8);   //High byte
    Wire.write(vec_size & 0xFF); //Low byte
    uint8_t res = Wire.endTransmission(true);
    if (res != 0) {
        Serial.println("comm_tx::: Header transmission failed");
        return;
    }
    delay(50);
    Serial.println("comm_tx::: Header Transmitted");

    delay(5);
    Serial.print("comm_tx::: vec_size = ");
    Serial.println(vec_size);

    for (uint16_t chunk = 0; chunk < vec_size; chunk++) {
        Wire.beginTransmission(8);

        //Point Index
        Wire.write(chunk >> 8);   // High byte
        Wire.write(chunk & 0xFF); // Low byte

        Wire.write(sign[chunk].xpos >> 8);
        Wire.write(sign[chunk].xpos & 0xFF);
        
        Wire.write(sign[chunk].ypos >> 8);
        Wire.write(sign[chunk].ypos & 0xFF);

        Wire.write(sign[chunk].zpos >> 8);
        Wire.write(sign[chunk].zpos & 0xFF);

        uint8_t res = Wire.endTransmission(true);

        if (res == 1) {
            Serial.print("comm_tx::: Chunk ");
            Serial.print(chunk);
            Serial.print(" failed: ");
            Serial.println(res);
            Serial.print("comm_tx::: Chunk size ");
            chunk--;
        }
        else if (res == 2) {
            Serial.print("comm_tx::: Chunk ");
            Serial.print(chunk);
            Serial.print(" failed: ");
            Serial.println(res);
            Serial.print("comm_tx::: NACK Address ");
            chunk--;
        }
        else if (res == 3) {
            Serial.print("comm_tx::: Chunk ");
            Serial.print(chunk);
            Serial.print(" failed: ");
            Serial.println(res);
            Serial.print("comm_tx::: NACK Data ");
            chunk--;
        }

        else if (res == 5) {
            Serial.print("comm_tx::: Chunk ");
            Serial.print(chunk);
            Serial.print(" failed: ");
            Serial.println(res);
            Serial.print("comm_tx::: Timeout ");
            chunk--;
        }
        else if (res == 4) {
            Serial.print("comm_tx::: Chunk ");
            Serial.print(chunk);
            Serial.print(" failed: ");
            Serial.println(res);
            Serial.print("comm_tx::: Other ");
            chunk--;
        }

        // Progress indicator
        if (chunk % 10 == 0) {
            Serial.print("comm_tx::: Sent ");
            Serial.print(chunk);
            Serial.print("/");
            Serial.println(vec_size);
        }
        
        delay(5);
    }
}
