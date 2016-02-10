#include <SoftPWM.h>
#include <SoftPWM_timer.h>
#include <Bounce2.h>
#include <SPI.h> 
#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 

// If using software SPI (the default case): 
/*
  GND GND
VCC +3.3V
D0 (SCK) D10
D1 (MOSI) D9
RES (RST) D13
DC (A0) D11
CS (CS) D12
 */
#define OLED_SCK  10 // D0 
#define OLED_MOSI  9 // D1
#define OLED_RST  13 // RES
#define OLED_A0   11 // DC
#define OLED_CS   12 // CS

// MOSFET #define section
#define LED_FET       A5 // LED effect signal button
#define FLYWHEEL_FET  A6 // Flywheel MOSFET signal pin
#define PUSHER_FET    A7 // Dart pusher MOSFET signal pin
#define BRAKE_FET     A8 // Pusher brake FET signal pin

// Button and input #define section
#define BUTTON_L      2   // Right control button
#define BUTTON_M      3   // Middle control button
#define BUTTON_R      4   // Left control button
#define REV_TRIGGER   5   // Flywheel spinup trigger
#define FIRE_TRIGGER  6   // Fire trigger
#define PUSH_RETURN   7   // Pusher motor return switch
#define MAG_SENSOR    8   // Switch for detecting Magazine presence
#define DART_SENSOR   12  // LED sensor for presence of dart
#define JAM_DOOR      A0  // Jam door sensor.  This is used to enter maintenance mode and adjusting settings

#define SET_FLY      0    // Flywheel setting
#define SET_PUSH     1    // Pusher setting
#define SET_LED      2    // LED brightness setting

#if (SSD1306_LCDHEIGHT != 64) 
#error("Height incorrect, please fix Adafruit_SSD1306.h!"); 
#endif

// Button debounce sections
Bounce revTrigger = Bounce();
Bounce fireTrigger = Bounce();
Bounce pushReturn = Bounce();
Bounce magSensor = Bounce();
Bounce buttonL = Bounce();
Bounce buttonM = Bounce();
Bounce buttonR = Bounce();
Bounce jamDoor = Bounce();

unsigned long prevTime;

Adafruit_SSD1306 display(OLED_MOSI, OLED_SCK, OLED_A0, OLED_RST, OLED_CS); 

byte flyPercent = 50; 
byte pushPercent = 100; 
byte ledPercent = 50;
byte dartsPerPull = 1;
byte dartsRemaining = 0;
bool fSingleFire = false;
byte runsLeft = 10;

void drawBorders()
{
  display.drawFastHLine(0,0,127,WHITE);
  display.drawFastVLine(127,0,64,WHITE);
  display.drawFastHLine(0,63,127,WHITE);
  display.drawFastVLine(0,0,63,WHITE);
}

void setup()   {  
  prevTime = millis();
  Serial.begin(9600);
  Serial.println("PWM %, prevTime, curTime");
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();

  SoftPWMBegin();
  SoftPWMSet(FLYWHEEL_FET,0);
  SoftPWMSetFadeTime(FLYWHEEL_FET,100,500);
  
  SoftPWMSet(PUSHER_FET,0);

  SoftPWMSet(LED_FET,0);
  SoftPWMSetFadeTime(LED_FET,100,100);

  pinMode(PUSH_RETURN, INPUT_PULLUP);
  pinMode(REV_TRIGGER, INPUT_PULLUP);
  pinMode(FIRE_TRIGGER, INPUT_PULLUP);
  pinMode(MAG_SENSOR, INPUT_PULLUP);
  pinMode(DART_SENSOR, INPUT_PULLUP);
  pinMode(JAM_DOOR, INPUT_PULLUP);
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_M, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  revTrigger.attach(REV_TRIGGER);
  fireTrigger.attach(FIRE_TRIGGER);
  pushReturn.attach(PUSH_RETURN);
  magSensor.attach(MAG_SENSOR);
  jamDoor.attach(JAM_DOOR);
  buttonL.attach(BUTTON_L);
  buttonM.attach(BUTTON_M);
  buttonR.attach(BUTTON_R);

  revTrigger.interval(5);
  fireTrigger.interval(5);
  pushReturn.interval(0);
  magSensor.interval(5);
  jamDoor.interval(5);
  buttonL.interval(5);
  buttonM.interval(5);
  buttonR.interval(5);  
  
  display.clearDisplay();
  drawBorders();
  display.display();
  
  magSensor.update();
  pushReturn.update();
  
} 

void loop() {
 display.clearDisplay();
  drawBorders();
  unsigned long curTime;
  unsigned long dispTime;
  pushReturn.update();

  display.setCursor(4,3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("PUSHER TEST");
  display.drawFastHLine(0,12,127,WHITE);
  display.println(pushPercent);
  
  SoftPWMSet(PUSHER_FET,pushPercent);
 //We want ten samples per percentage.  
 //Count the number of times run at a percent, 
 //decrease percent by 5
 //each run will be sent to serial console
 if (pushReturn.fell())
 {
  curTime = millis();
  Serial.print(pushPercent);
  Serial.print(",");
  Serial.print(prevTime);
  Serial.print(",");
  Serial.print(curTime);
  Serial.println("");
  runsLeft--;
  prevTime = curTime;
  display.display();

  if (runsLeft <=1)
  {
    runsLeft = 10;
    pushPercent -= 5;

    if (pushPercent <=1)
    {
      pushPercent = 100;
    }
  }
 }
 
}
