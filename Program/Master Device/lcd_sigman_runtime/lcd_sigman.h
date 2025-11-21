
#ifndef LCD_SIGMAN_H
#define LCD_SIGMAN_H

//Libraries
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include "type_dat.h"
#include "vault_ops.h"

//TFT Object
extern MCUFRIEND_kbv tft;

//Colors
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

//Pressure Thresholds
#define MINPRESSURE 5
#define MAXPRESSURE 1000

//Constants; Calibrations and Pinouts
const int XP = 27, XM = 15, YP = 4, YM = 14;
const int TS_LT = 964, TS_RT = 139, TS_TOP = 936, TS_BOT = 116;

//Function Prototypes
void lcd_access_screen();
void lcd_start_screen();
vector<sigpoints> lcd_sign_reader();
void lcd_lock_screen(bool);
void lcd_storage_page(char, char);


/*
ID = 0x9341

XP=27,XM=15,YP=4,YM=14; 240x320; ID=0x9341

PORTRAIT  CALIBRATION     240 x 320
x = map(p.x, LEFT=116, RT=936, 0, 240)
y = map(p.y, TOP=964, BOT=139, 0, 320)

LANDSCAPE CALIBRATION     320 x 240
x = map(p.y, LEFT=964, RT=139, 0, 320)
y = map(p.x, TOP=936, BOT=116, 0, 240)
*/

#endif
