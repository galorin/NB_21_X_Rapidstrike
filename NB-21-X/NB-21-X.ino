#include <SPI.h> 
#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 

// If using software SPI (the default case): 
#define OLED_MOSI   9 
#define OLED_CLK   10 
#define OLED_DC    11 
#define OLED_CS    12 
#define OLED_RESET 13 
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS); 



#define PUSH_POT 0 
bool blinkOn = true; 
bool fDartSeen = true; 

int flyPercent = 0; 
int pushPercent = 0; 

#if (SSD1306_LCDHEIGHT != 64) 
#error("Height incorrect, please fix Adafruit_SSD1306.h!"); 
#endif 

void setup()   {                 
 Serial.begin(9600); 
 display.begin(SSD1306_SWITCHCAPVCC); 
 display.clearDisplay(); 
} 

void loop() { 
 display.clearDisplay(); 
  
 display.fillRect(0, 0, display.width(), display.height(), WHITE); 
 display.fillRect(2, 2, display.width()-4, display.height()-4, BLACK); 
 display.drawLine(61, 0, 61, 40, WHITE); 
 display.drawLine(62, 0, 62, 40, WHITE); 
 display.drawLine(0,40,127,40,WHITE); 

 display.drawLine(0,28,127,28,WHITE); 

//display flywheel percent 
 display.setCursor(7,8); 
 display.setTextSize(2); 
 display.print(flyPercent); 
 display.println("%");   
  
 display.setCursor(7,31); 
 display.setTextSize(1); 
 display.setTextColor(WHITE); 
 display.println("Flywheel"); 

// display pusher percent   
 display.setCursor(68,8); 
 display.setTextSize(2); 
 int percent = map(analogRead(PUSH_POT),0,1023,0,100); 
 int newPer = round(percent / 10) * 10; 
 display.print(newPer); 
 display.println("%"); 

 display.setCursor(68,31); 
 display.setTextSize(1); 
 display.println("Pusher"); 

if (fDartSeen) 
{ 
 display.setCursor(5,44); 
 display.setTextSize(2); 
 display.setTextColor(WHITE); 
 display.println("DART READY"); 
} 
else 
{ 
 if (blinkOn) 
 { 
 display.setCursor(20,44); 
 display.setTextSize(2); 
 display.setTextColor(WHITE); 
 display.println("NO DART"); 
 blinkOn = false; 
 } 
 else 
 { 
   blinkOn = true; 
 } 
} 

 display.display(); 
 delay(50);   
} 
