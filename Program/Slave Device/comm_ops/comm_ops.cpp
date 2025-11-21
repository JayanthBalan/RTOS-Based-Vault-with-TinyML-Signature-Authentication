
#include "comm_ops.h"

volatile char a, b;
vector<sigpoints> sign;
uint16_t expec_size = 0;
uint16_t rx_chunk = 0;
bool received_header = false;
bool tx_fin = false;

void rx_comm(int);
void tx_comm();

void comms_init()
{
    Serial.begin(115200);

    Wire.begin(8); //Slave Mode; Address = 8
    Wire.onReceive(rx_comm); //Register event
    Wire.onRequest(tx_comm); //Register event

    a = 'w';
    b = '0';

    Serial.println("comm_ini::: Initialized");
}

void rx_comm(int datasize)
{
    if (datasize == 0) {
        return;
    }

    uint8_t first_byte = Wire.read();
    datasize--;

    // Check for header
    if (first_byte == 0xFF && datasize >= 2) {
        uint8_t size_high = Wire.read();
        uint8_t size_low = Wire.read();
        
        expec_size = (size_high << 8) | size_low;
        
        sign.clear();
        sign.reserve(expec_size);
        rx_chunk = 0;
        received_header = true;
        tx_fin = false;
        
        while (Wire.available()) {
            Wire.read();
        }
        return;
    }

    if (received_header && datasize >= 7) {
        //Point index
        uint8_t idx_high = first_byte;
        uint8_t idx_low = Wire.read();
        uint16_t point_idx = (idx_high << 8) | idx_low;
        
        //xpos
        uint8_t x_high = Wire.read();
        uint8_t x_low = Wire.read();
        uint16_t xpos = (x_high << 8) | x_low;
        
        //ypos
        uint8_t y_high = Wire.read();
        uint8_t y_low = Wire.read();
        uint16_t ypos = (y_high << 8) | y_low;
        
        //zpos
        uint8_t z_high = Wire.read();
        uint8_t z_low = Wire.read();
        uint16_t zpos = (z_high << 8) | z_low;
        
        sigpoints point = {xpos, ypos, zpos};
        sign.push_back(point);
        rx_chunk++;
        
        //Progress indicator
        if (rx_chunk % 100 == 0) {
            Serial.print("rx_comm::: Received ");
            Serial.print(rx_chunk);
            Serial.print("/");
            Serial.println(expec_size);
        }
    
        if (rx_chunk >= expec_size) {
            tx_fin = true;
        }
        
        while (Wire.available()) {
            Wire.read();
        }
    }
    else if (!received_header) {
        while (Wire.available()) {
            Wire.read();
        }
    }
    else {
        Serial.println(datasize);
        while (Wire.available()) {
            Wire.read();
        }
    }
}

void tx_comm() //Request event
{
    Wire.write(a);
    Wire.write(b);
}
