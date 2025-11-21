
#ifndef VAULT_OPS_H
#define VAULT_OPS_H

//Libraries
#include <SPIFFS.h>

//Macros
#define MAX_STORAGE 50

//Function Prototypes
void archive_spiffs(String[]);
void extract_spiffs(String[]);
void xor_encrypt(String[]);
void xor_decrypt(String[], char, char);

#endif
