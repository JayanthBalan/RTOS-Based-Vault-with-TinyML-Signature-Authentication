
#include "lcd_sigman.h"

//Touchscreen Initializations
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

//TFT Parameters
uint16_t boxsize, boxmod, ID, penradius = 1.5;
uint8_t Orientation = 1;

MCUFRIEND_kbv tft;

String curr_in = "";
int curr_row = 0;
int scroll_offset = 0;
const int max_rows = 5;
String passwords[MAX_STORAGE];

//Page 1
void lcd_start_screen()
{    
    tft.reset();
    ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(Orientation);
    tft.fillScreen(BLACK);

    tft.setCursor(0, 0);
    tft.setTextSize(1);
    tft.setTextColor(BLUE);
    tft.print(F("ID = 0x"));
    tft.println(ID, HEX);
    tft.println("Screen size = " + String(tft.width()) + "x" + String(tft.height()));

    tft.setTextSize(3);
    tft.setTextColor(YELLOW);
    tft.setCursor(64, tft.height()/2);
    tft.print(F("NEXUS VAULT"));

    extract_spiffs(passwords);

    do
    {
        tp = ts.getPoint();

        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);

        delay(1);
    } while(tp.z < MINPRESSURE || tp.z > MAXPRESSURE);
}

//Page 2
void lcd_access_screen()
{
    boxsize = tft.width() / 3;
    boxmod = 50;

    tft.fillScreen(BLACK);
    
    //Signature Box
    tft.fillRect(boxsize - boxmod/2, 0, boxsize + boxmod, boxsize/3, WHITE);
    tft.setCursor(boxsize+2, 10);
    tft.setTextSize(2);
    tft.setTextColor(BLUE);
    tft.print(F("SIGNATURE"));

    //Clear Box
    tft.fillRect(0, tft.height() - boxsize/3, boxsize*3/4, boxsize/3, RED);
    tft.setCursor(5, tft.height() - boxsize/3 + 10);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print(F("CLEAR"));

    //Submit Box
    tft.fillRect(tft.width() - boxsize*3/4, tft.height() - boxsize/3, boxsize*3/4, boxsize/3, GREEN);
    tft.setCursor(tft.width() - boxsize*3/4 + 5, tft.height() - boxsize/3 + 10);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print(F("SUBMIT"));
}

//Reads Pointer; Signature Collection
vector<sigpoints> lcd_sign_reader()
{
    uint16_t xpos, ypos, zpos;
    vector<sigpoints> sign_dat;

    while(1)
    {
        tp = ts.getPoint();

        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);

        delay(1);

        if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE)
        {
            ypos = map(tp.x, TS_TOP, TS_BOT, 0, tft.height());
            xpos = map(tp.y, TS_LT, TS_RT, 0, tft.width());
            zpos = ((float)tp.z)/((float)MAXPRESSURE); //Normalize zpos for DTW

            if (ypos > tft.height() - boxsize/3)
            {
                if (xpos < boxsize*3/4) //Clear
                {
                    tft.fillRect(0, boxsize/3, tft.width(), tft.height() - 2*boxsize/3, BLACK);
                    tft.fillRect(0, 0, boxsize - boxmod/2, boxsize/3, BLACK);
                    tft.fillRect(boxsize*2 + boxmod/2, 0, tft.width() - (boxsize*2 + boxmod/2), boxsize/3, BLACK);
                    tft.fillRect(boxsize*3/4, tft.height() - boxsize/3, tft.width() - boxsize*3/2, boxsize/3, BLACK);
                }
                else if (xpos > tft.width() - boxsize*3/4) //Submit
                {
                    tft.fillRect(0, boxsize/3, tft.width(), tft.height() - 2*boxsize/3, BLACK);
                    tft.fillRect(0, 0, boxsize - boxmod/2, boxsize/3, BLACK);
                    tft.fillRect(boxsize*2 + boxmod/2, 0, tft.width() - (boxsize*2 + boxmod/2), boxsize/3, BLACK);
                    tft.fillRect(boxsize*3/4, tft.height() - boxsize/3, tft.width() - boxsize*3/2, boxsize/3, BLACK);
                    return sign_dat;
                }
            }

            //Draw
            if (((ypos + penradius) > (boxsize/3)) && ((ypos + penradius) < (tft.height() - boxsize/3)))
            {
                tft.fillCircle(xpos, ypos, penradius, MAGENTA);
                sigpoints point = {xpos, ypos, zpos};
                sign_dat.push_back(point);
            }
        }
    }
}

// Page 3 - Main Menu
void lcd_menu() {
    tft.fillScreen(BLACK);
    
    float use_width = tft.width();
    float use_height = tft.height();
    float scroll_width = use_width / 10;
    float content_width = use_width - scroll_width;
    float row_height = use_height / max_rows;
    
    //Rows
    for(int i = 0; i < max_rows; i++) {
        int actualIndex = i + scroll_offset;
        
        //Row Border
        tft.drawRect(0, i * row_height, content_width, row_height, YELLOW);
        
        //Password Info
        if(passwords[actualIndex].length() > 0) {
            tft.setCursor(5, i * row_height + row_height/2 - 4);
            tft.setTextSize(2);
            tft.setTextColor(BLUE);
            
            //Username
            int spacePos = passwords[actualIndex].indexOf(" - ");
            if(spacePos > 0) {
                String username = passwords[actualIndex].substring(0, spacePos);
                if(username.length() > 20) {
                    username = username.substring(0, 17) + "...";
                }
                tft.print(username);
            } else {
                tft.print("Entry " + String(actualIndex + 1));
            }
        }
    }
    
    //Scroll
    //Up button
    tft.fillRect(content_width, 0, scroll_width, use_height/3, BLUE);
    tft.drawRect(content_width, 0, scroll_width, use_height/3, BLACK);
    tft.setCursor(content_width + scroll_width/2 - 3, use_height/6 - 3);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print("^");
    
    //Down button
    tft.fillRect(content_width, use_height/3, scroll_width, use_height/3, BLUE);
    tft.drawRect(content_width, use_height/3, scroll_width, use_height/3, BLACK);
    tft.setCursor(content_width + scroll_width/2 - 3, 3*use_height/6 - 3);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print("v");

    //Down button
    tft.fillRect(content_width, 2*use_height/3, scroll_width, use_height/3, BLUE);
    tft.drawRect(content_width, 2*use_height/3, scroll_width, use_height/3, BLACK);
    tft.setCursor(content_width + scroll_width/2 - 3, 5*use_height/6 - 3);
    tft.setTextSize(2);
    tft.setTextColor(WHITE);
    tft.print("X");
}

//Password Manager Page
void lcd_password_page(bool alpha_case) {
    float use_width = tft.width();
    float use_height = tft.height();
    float boxsize2 = use_width / 11;
    float display_height = use_height - boxsize2 * 5;
    tft.drawRect(0, 0, use_width, display_height, WHITE);
    
    //Keyboard area
    int offset1 = 2;
    
    //Row 3: 0-9
    String row0 = "0123456789";
    for(int i = 0; i < 10; i++) {
        char alpha = row0[i];
        tft.fillRect(i*boxsize2, display_height, boxsize2-1, boxsize2-1, BLUE);
        tft.setCursor(offset1 + i*boxsize2 + boxsize2/2 - 3, display_height + boxsize2/2 - 4);
        tft.setTextSize(1);
        tft.setTextColor(BLACK);
        tft.print(alpha);
    }

    //Row 1: A-J (or a-j)
    char ascii_beg = alpha_case ? 65 : 97;
    for(int i = 0; i < 10; i++) {
        char alpha = ascii_beg + i;
        tft.fillRect(i*boxsize2, display_height + boxsize2, boxsize2-1, boxsize2-1, YELLOW);
        tft.setCursor(offset1 + i*boxsize2 + boxsize2/2 - 3, display_height + boxsize2 + boxsize2/2 - 4);
        tft.setTextSize(1);
        tft.setTextColor(BLACK);
        tft.print(alpha);
    }
    //Space key
    tft.fillRect(10*boxsize2, display_height, boxsize2-1, 2*boxsize2-1, MAGENTA);
    tft.setCursor(offset1 + 10*boxsize2 + boxsize2/2 - 3, display_height + boxsize2 - 4);
    tft.setTextColor(BLACK);
    tft.print("_");
    
    //Row 2: K-T (or k-t)
    for(int i = 0; i < 10; i++) {
        char alpha = ascii_beg + 10 + i;
        tft.fillRect(i*boxsize2, display_height + 2*boxsize2, boxsize2-1, boxsize2-1, YELLOW);
        tft.setCursor(offset1 + i*boxsize2 + boxsize2/2 - 3, display_height + boxsize2*2 + boxsize2/2 - 4);
        tft.setTextSize(1);
        tft.setTextColor(BLACK);
        tft.print(alpha);
    }
    
    //Row 3: U-Z + special chars + numbers
    String row3 = alpha_case ? "UVWXYZ-_.@" : "uvwxyz-_.@";
    for(int i = 0; i < 10; i++) {
        char alpha = row3[i];
        tft.fillRect(i*boxsize2, display_height + boxsize2*3, boxsize2-1, boxsize2-1, YELLOW);
        tft.setCursor(offset1 + i*boxsize2 + boxsize2/2 - 3, display_height + boxsize2*3 + boxsize2/2 - 4);
        tft.setTextSize(1);
        tft.setTextColor(BLACK);
        tft.print(alpha);
    }

    //Backspace key
    tft.fillRect(10*boxsize2, display_height + boxsize2*2, boxsize2-1, 2*boxsize2-1, MAGENTA);
    tft.setCursor(offset1 + 10*boxsize2 + boxsize2/2 - 3, display_height + boxsize2*2 + boxsize2 - 4);
    tft.setTextColor(BLACK);
    tft.print("<");
    
    //Row 4: Control buttons
    float button_width = use_width / 3;
    
    //Enter button
    tft.fillRect(0, display_height + boxsize2*4, button_width-1, boxsize2-1, GREEN);
    tft.setCursor(button_width/2 - 15, display_height + boxsize2*4 + boxsize2/2 - 4);
    tft.setTextSize(1);
    tft.setTextColor(BLACK);
    tft.print("ENTER");
    
    //Case button (toggle uppercase/lowercase)
    tft.fillRect(button_width, display_height + boxsize2*4, button_width-1, boxsize2-1, CYAN);
    tft.setCursor(button_width + button_width/2 - 15, display_height + boxsize2*4 + boxsize2/2 - 4);
    tft.setTextColor(BLACK);
    tft.print(alpha_case ? "ALPHA" : "alpha");
    
    //Delete button
    tft.fillRect(button_width*2, display_height + boxsize2*4, button_width, boxsize2-1, RED);
    tft.setCursor(button_width*2 + button_width/2 - 10, display_height + boxsize2*4 + boxsize2/2 - 4);
    tft.setTextColor(WHITE);
    tft.print("DEL");

}

void lcd_password_manager() {
    float use_width = tft.width();
    float use_height = tft.height();
    float boxsize2 = use_width / 11;
    float display_height = use_height - boxsize2 * 5;

    //Display area at top
    tft.fillRect(0, 0, use_width, display_height, BLACK);
        
    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(0,0);
    if(curr_in.length() < 67)
    {
        tft.print(curr_in);
    }
    else {
        tft.print(curr_in.substring(0,67));
    }
}

//Touch Password Manager
void handle_password_touch() {
    float use_width = tft.width();
    float use_height = tft.height();
    float boxsize2 = use_width / 11;
    float display_height = use_height - boxsize2 * 5;
    bool alpha_case = true;
    uint16_t xpos, ypos;
    String temp_in;

    tft.setTextSize(3);
    tft.setTextColor(WHITE);
    tft.setCursor(0,0);
    if(curr_in.length() < 67)
    {
        tft.print(curr_in);
    }
    else {
        tft.print(curr_in.substring(0,67));
    }

    while(1)
    {
        TSPoint tp = ts.getPoint();
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        delay(1);
        temp_in = curr_in;
                        
        if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE)
        {
            ypos = map(tp.x, TS_TOP, TS_BOT, 0, tft.height());
            xpos = map(tp.y, TS_LT, TS_RT, 0, tft.width());
    
            //Row Checker
            int key_y = (ypos - display_height) / boxsize2;
            int key_x = xpos / boxsize2;
            
            char ascii_beg = alpha_case ? 65 : 97;
            char num_vals[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
            
            if(key_y == 0) //Zeroth row
            {
                if(key_x < 10) {
                    curr_in += num_vals[key_x];
                }
                else {
                    curr_in += " "; //Space
                }
            }
            if(key_y == 1) { //First row
                if(key_x < 10) {
                    curr_in += (char)(ascii_beg + key_x);
                }
                else {
                    curr_in += " "; //Space
                }
            }
            else if(key_y == 2) { //Second row
                if(key_x < 10) {
                    curr_in += (char)(ascii_beg + 10 + key_x);
                } 
                else {
                    //Backspace
                    if(curr_in.length() > 0) {
                        curr_in.remove(curr_in.length() - 1);
                    }
                }
            }
            else if(key_y == 3) { //Third row
                String row3 = alpha_case ? "UVWXYZ-_.@" : "uvwxyz-_.@";
                if(key_x < 10) {
                    curr_in += row3[key_x];
                }
                else {
                    //Backspace
                    if(curr_in.length() > 0) {
                        curr_in.remove(curr_in.length() - 1);
                    }
                }
            }
            else if(key_y == 4) { //Control row
                float button_width = use_width / 3;
                int button = xpos / button_width;
                
                if(button == 0) {
                    //ENTER Key
                    passwords[curr_row] = curr_in;
                    return;
                }
                else if(button == 1) {
                    //Toggle case
                    alpha_case = !alpha_case;
                    lcd_password_page(alpha_case);
                }
                else {
                    //Clear Input
                    curr_in = "";
                }
            }

            //Redraw screen
            if(temp_in != curr_in)
            {
                temp_in = curr_in;
                lcd_password_manager();
                tft.print(F("|"));
                delay(50);
            }
        }
    }
}

//Menu Operation
void handle_menu_touch() {
    float use_width = tft.width();
    float use_height = tft.height();
    float scroll_width = use_width / 10;
    float content_width = use_width - scroll_width;
    float row_height = use_height / max_rows;
    uint16_t xpos, ypos;
    
    while(1)
    {
        TSPoint tp = ts.getPoint();
        pinMode(XM, OUTPUT);
        pinMode(YP, OUTPUT);
        delay(1);
        
        if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE)
        {
            ypos = map(tp.x, TS_TOP, TS_BOT, 0, tft.height());
            xpos = map(tp.y, TS_LT, TS_RT, 0, tft.width());

            if(xpos > content_width) {
                if(ypos < use_height/3) {
                    //Scroll up
                    if(scroll_offset > 0) {
                        scroll_offset--;
                        lcd_menu();
                    }
                }
                else if(ypos > use_height/3 && ypos < 2*use_height/3) {
                    //Scroll down
                    if(scroll_offset < 11) {
                        scroll_offset++;
                        lcd_menu();
                    }
                }
                else
                { //Exit
                    xor_encrypt(passwords);
                    archive_spiffs(passwords);
                    delay(100);
                    return;
                }
            }
            else { //Row Check
                int row = ypos / row_height;
                if(row >= 0 && row < max_rows) {
                    curr_row = row + scroll_offset;
                    curr_in = passwords[curr_row]; //Data Load
                    tft.fillScreen(BLACK);
                    lcd_password_page(true);
                    handle_password_touch();
                    Serial.print("lcd::: Password = ");
                    Serial.println(curr_in);
                    lcd_menu();
                }
            }
        }
    }
}

void lcd_storage_page(char a, char b)
{
    xor_decrypt(passwords, a, b);
    lcd_menu();
    handle_menu_touch();
}

void lcd_lock_screen(bool validity)
{
    if(validity)
    {
        tft.fillScreen(BLACK);

        tft.setCursor(0, 0);
        tft.setTextSize(1);
        tft.setTextColor(BLUE);
        tft.print(F("ID = 0x"));
        tft.println(ID, HEX);
        tft.println("Screen size = " + String(tft.width()) + "x" + String(tft.height()));

        tft.setTextSize(3);
        tft.setTextColor(YELLOW);
        tft.setCursor(7, tft.height()/2);
        tft.print(F("Valid Signature"));
    }
    else
    {
        tft.fillScreen(BLACK);

        tft.setCursor(0, 0);
        tft.setTextSize(1);
        tft.setTextColor(BLUE);
        tft.print(F("ID = 0x"));
        tft.println(ID, HEX);
        tft.println("Screen size = " + String(tft.width()) + "x" + String(tft.height()));

        tft.setTextSize(3);
        tft.setTextColor(YELLOW);
        tft.setCursor(3, tft.height()/2);
        tft.print(F("Invalid Signature"));
    }
    delay(3000);
}

