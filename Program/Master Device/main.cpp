
#include "lcd_sigman.h"
#include "comm_ops.h"
#include "type_dat.h"

vector<sigpoints> sig_resample(const vector<sigpoints>&);

void setup()
{
  Serial.begin(9600);
  delay(2);
  comms_init();
  delay(2);
  analogReadResolution(10);
  delay(2);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  delay(2);
}

void loop()
{
  char a, b;

  Serial.println("lcd::: Start Screen");
  delay(2);
  lcd_start_screen();
  Serial.println("lcd::: Access Screen");
  delay(2);
  lcd_access_screen();
  delay(1);

  vector<sigpoints> sign = lcd_sign_reader();

  vector<sigpoints> sign_opt = sig_resample(sign);

  delay(1);
  tx_comm(sign_opt);
  delay(5);

  Serial.println("main::: Latency measure start");
  uint32_t start_time = millis();

  while(1)
  {
    rx_comm(&a, &b);
    
    if(a =='y' && b == '1')
    {
      Serial.println("lcd::: Valid Signature\nlcd::: Storage Page");
      uint32_t latency = millis() - start_time;
      Serial.print("main::: Latency = ");
      Serial.print(latency);
      Serial.println(" ms");
      lcd_lock_screen(true);
      delay(3);
      lcd_storage_page(a, b);
      break;
    }
    else if(a =='n' && b == '1')
    {
      Serial.println("lcd::: Invalid Signature\nlcd::: Restart");
      uint32_t latency = millis() - start_time;
      Serial.print("main::: Latency = ");
      Serial.print(latency);
      Serial.println(" ms");
      lcd_lock_screen(false);
      delay(3);
      break;
    }
    else if(a == 'x' && b == 'x')
    {
      Serial.println("comm_rx::: No Reply");
      delay(1000);
    }
    else if(a == 'w' && b == '0')
    {
      delay(1000);
    }
    else if(a == 'r' && b == '2')
    {
      ESP.restart();
    }
    else
    {
      Serial.print("comm_rx::: Invalid Reply = ");
      Serial.print(a);
      Serial.println(b);
      delay(5);
    }
    delay(10);
  }
}

vector<sigpoints> sig_resample(const vector<sigpoints>& sig) {
  int original_size = sig.size(), target = 150;
    
  if(original_size == target) {
      return sig;
  }
    
  if(original_size == 0) {
    return vector<sigpoints>();
  }
    
  vector<sigpoints> resampled;
  resampled.reserve(target);
    
  float step_size = (float)(original_size - 1) / (float)(target - 1);
    
  for(int i = 0; i < target; i++) {
    float idx = i * step_size;
    int idx_low = (int)idx;
    int idx_high = min(idx_low + 1, original_size - 1);
    float t = idx - idx_low;  //Interpolation factor (0 - 1)
        
    // Linear interpolation
    sigpoints interpolated;
    interpolated.xpos = sig[idx_low].xpos + t * (sig[idx_high].xpos - sig[idx_low].xpos);
    interpolated.ypos = sig[idx_low].ypos + t * (sig[idx_high].ypos - sig[idx_low].ypos);
    interpolated.zpos = sig[idx_low].zpos + t * (sig[idx_high].zpos - sig[idx_low].zpos);
    resampled.push_back(interpolated);
  }
    
  return resampled;
}
