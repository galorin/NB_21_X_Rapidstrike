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
#define FLYWHEEL_FET  A6 // Flywheel MOSFET signal pin
#define PUSHER_FET    A7 // Dart pusher MOSFET signal pin
#define LED_FET       A5 // LED effect signal button

// Button and input #define section
#define PUSH_RETURN  1 // Pusher motor return switch
#define REV_TRIGGER  2 // Flywheel spinup trigger
#define FIRE_TRIGGER 3 // Fire trigger
#define MAG_SENSOR   4 // Switch for detecting Magazine presence
#define DART_SENSOR  5 // LED sensor for presence of dart
#define BUTTON_L     6 // Left control button
#define BUTTON_M     7 // Middle control button
#define BUTTON_R     8 // Right control button

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

unsigned long prevTime;

Adafruit_SSD1306 display(OLED_MOSI, OLED_SCK, OLED_A0, OLED_RST, OLED_CS); 

int flyPercent = 0; 
int pushPercent = 0; 
uint8_t dartsPerPull = 1;
bool fSingleFire = false;

void setup()   {  
  prevTime = millis();
  Serial.begin(9600);
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
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_M, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  revTrigger.attach(REV_TRIGGER);
  fireTrigger.attach(FIRE_TRIGGER);
  pushReturn.attach(MAG_SENSOR);
  magSensor.attach(DART_SENSOR);
  buttonL.attach(BUTTON_L);
  buttonM.attach(BUTTON_M);
  buttonR.attach(BUTTON_R);

  revTrigger.interval(5);
  fireTrigger.interval(5);
  pushReturn.interval(5);
  magSensor.interval(5);
  buttonL.interval(5);
  buttonM.interval(5);
  buttonR.interval(5);
} 

void loop() {
  display.clearDisplay();
  drawBorders();

  unsigned long curTime = millis();

  if (!checkErrors())// This is error checking.
  {
    display.setCursor(20,10);
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.println("READY");

    // There is a soft lock here.  If the flywheel trigger is held down, spin up the flywheels.
    // When the trigger is pulled, start up the pusher motor and run it.
    // When the trigger is released, slow the pusher motor until the switch for 
    // the pusher is triggered.
    if (revTrigger.read() == LOW)
    {
      SoftPWMSet(FLYWHEEL_FET,50);
      if (fireTrigger.read() == LOW)
      {
        SoftPWMSet(PUSHER_FET,50);
      }
      else
      {
        while (pushReturn.read() == HIGH)
        {
          SoftPWMSet(PUSHER_FET,10);
        }
        SoftPWMSet(PUSHER_FET,0);
      }
    }
    else
    {
      SoftPWMSetPercent(FLYWHEEL_FET,0);
      SoftPWMSet(PUSHER_FET,0);
    }
  }
  
  if (curTime - prevTime > 50)
  {
    display.display();
    prevTime = curTime;
  }
}

bool checkErrors()
{
  unsigned long curTime = millis();
  bool errorsFound = false;
  // reset error status when we check
  byte errorStatus = B00000000;
  display.clearDisplay();
  drawBorders();
  if (magSensor.read() == false)
  {
    errorStatus | 1 ;
    errorsFound = true;
  }

  if (digitalRead(DART_SENSOR) == HIGH)
  {
    errorStatus | 2 ;
    errorsFound = true;
  }

  if (errorStatus)
  {
    display.setCursor(10,10);
    display.println("ERROR");
    if (errorStatus && 1)
    {
      display.println("No Magazine");
    }
    if (errorStatus && 2)
    {
      display.println("No Dart");
    }

    //only update the display every 50ms
    if (curTime - prevTime > 50)
    {
      display.display();
      prevTime = curTime;
    }
  }
  
  return errorsFound;
}

void resetPusher()
{
  display.clearDisplay();
  drawBorders();
  display.setCursor(10,10);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("RESETTING");
  display.println("PUSHER");
  display.display();
  while (pushReturn.read() == HIGH)
  {
    SoftPWMSet(PUSHER_FET,10);
  }
}

void drawBorders()
{
  display.drawFastHLine(0,0,display.width()-1,WHITE);
  display.drawFastVLine(display.width()-1,0,display.height()-1,WHITE);
  display.drawFastHLine(display.height()-1,0,display.width()-1,WHITE);
  display.drawFastVLine(0,0,display.height()-1,WHITE);
}

