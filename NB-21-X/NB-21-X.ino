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

enum magSizes
{
  6ROUND,
  12ROUND,
  18ROUND,
  25ROUND,
  35ROUND
};

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
byte pushPercent = 50; 
byte ledPercent = 50;
byte dartsPerPull = 1;
byte dartsRemaining = 0;
magSizes curMag = 12ROUND;
bool fSingleFire = false;

void drawBorders()
{
  display.drawFastHLine(0,0,127,WHITE);
  display.drawFastVLine(127,0,64,WHITE);
  display.drawFastHLine(0,63,127,WHITE);
  display.drawFastVLine(0,0,63,WHITE);
}

bool checkErrors()
{
  unsigned long curTime = millis();
  bool errorsFound = false;
  // reset error status when we check
  byte errorStatus = B00000000;
  if (magSensor.read() == HIGH)
  {
    errorStatus = errorStatus | B00000001;
    errorsFound = true;
  }

//  if (digitalRead(DART_SENSOR) == HIGH)
//  {
//    errorStatus = errorStatus | B00000010 ;
//    errorsFound = true;
//  }

  if (errorsFound)
  {
    display.clearDisplay();
    drawBorders();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(35,3);
    display.println("ERROR");
    //display.setTextSize(1);
    if (errorStatus & B00000001)
    {
      display.setCursor(18,20);
      display.println("MAGAZINE");
    }
    if (errorStatus & B00000010)
    {
      display.println("No Dart");
    }

    //only update the display every 50ms
    curTime = millis();
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
  display.setCursor(10,7);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("RESETTING");
  display.setCursor(27,30);
  display.println("PUSHER");
  display.display();
  while (!pushReturn.rose())
  {
    SoftPWMSet(PUSHER_FET,10);
    pushReturn.update();
    jamDoor.update();
    if (jamDoor.fell())
    {
      return;
    }
  }
}

void drawDart(byte numOfDarts)
{
  // A dart drawing is 12 pixels wide and 5 pixels high.
  // XXXXXXXXXXX
  // X       X  X
  // X       X   X
  // X       X  X
  // XXXXXXXXXXX
  for (byte iOffset = 0; iOffset < numOfDarts; iOffset++)
  {
  byte iMove = iOffset * 12;
  display.drawFastHLine(3 + iMove,42,10,WHITE);
  display.drawFastHLine(3 + iMove,46,10,WHITE);
  display.drawFastVLine(3 + iMove,42,5,WHITE);
  display.drawFastVLine(10 + iMove,42,5,WHITE);
  display.drawFastVLine(13 + iMove,43,3,WHITE);
  }
}

void setup()   {  
  prevTime = millis();
  //Serial.begin(9600);
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
  
  if (pushReturn.read() == HIGH)
  {
    display.setCursor(3,3);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Please wait while Pusher is reset");
    
    while (!pushReturn.rose())
    {
      SoftPWMSet(PUSHER_FET,10);
      pushReturn.update();
    }
    SoftPWMSet(PUSHER_FET,0);
  }
  
} 

void loop() {
  display.clearDisplay();
  drawBorders();
  
  revTrigger.update();
  fireTrigger.update();
  pushReturn.update();
  magSensor.update();
  jamDoor.update();
  buttonL.update();
  buttonM.update();
  buttonR.update();
  

  unsigned long curTime = millis();
  byte mtnControl = SET_FLY; //Maintenance control 
  while (jamDoor.read()== LOW)
  {
  curTime = millis();
  jamDoor.update();
  buttonL.update();
  buttonM.update();
  buttonR.update();  

  display.clearDisplay();
  drawBorders();
  
  display.setCursor(40,3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("SETTINGS");
  display.drawFastHLine(0,12,127,WHITE);
  
  display.setCursor(3,15);
  display.setTextSize(1);
  display.print("FLYWHEEL");
  display.fillRect(53,15,constrain(map(flyPercent,0,100,0,70),0,100),7,WHITE);
  display.setCursor(3,23);  
  display.print("PUSHER");  
  display.fillRect(53,23,constrain(map(pushPercent,0,100,0,70),0,100),7,WHITE);
  display.setCursor(3,31);  
  display.print("LED");  
  display.fillRect(53,31,constrain(map(ledPercent,0,100,0,70),0,100),7,WHITE);
  display.drawFastHLine(0,39,127,WHITE);
  if (mtnControl == SET_PUSH)
  {
    display.setCursor(19,41);  
    display.print("CHANGING:PUSHER");
  }
  else if (mtnControl == SET_FLY)
  {
    display.setCursor(13,41);  
    display.print("CHANGING:FLYWHEEL");
  }
  else if (mtnControl == SET_LED)
  {
    display.setCursor(28,41);  
    display.print("CHANGING:LED");
  }
  
  
  if (buttonL.fell())
  {
    if (mtnControl == SET_PUSH)
    {
      if (pushPercent > 10)
      {
        pushPercent -= 10;
      }
    }
    else if (mtnControl == SET_FLY)
    {
      if (flyPercent > 10)
      {
        flyPercent -= 10;
      }
    }
    else if (mtnControl == SET_LED)
    {
      if (ledPercent > 0)
      {
        ledPercent -= 10;
      }
    }
  }
  
  
  if (buttonM.fell())
  {
    if (mtnControl == SET_FLY)
    {
      mtnControl = SET_PUSH;
      //Serial.println("settingPush");
    }
    else if (mtnControl == SET_PUSH)
    {
      mtnControl = SET_LED;
      //Serial.println("settingLed");
    }
    else
    {
      mtnControl = SET_FLY;
      //Serial.println("settingFly");
    }
  }

  if (buttonR.fell())
  {
    if (mtnControl == SET_PUSH)
    {
      if (pushPercent < 100)
      {
        pushPercent += 10;
      }
    }
    else if (mtnControl == SET_FLY)
    {
      if (flyPercent < 100)
      {
        flyPercent += 10;
      }
    }
    else if (mtnControl == SET_LED)
    {
      if (ledPercent < 100)
      {
        ledPercent += 10;
      }
    }
  }
  
  display.drawFastHLine(0,50,127,WHITE);
  display.setTextSize(1);
  display.setCursor(21,53);
  display.print("-");
  display.drawFastVLine(41,52,22,WHITE);
  display.setCursor(53,53);
  display.print("MODE");
  display.drawFastVLine(83,52,22,WHITE);
  display.setCursor(105,53);
  display.print("+");
  
  if (curTime - prevTime > 50)
  {
    display.display();
    prevTime = curTime;
  }  
  }
  
  if (!checkErrors())// This is error checking.
  {
    //Serial.println("This is fine");
    display.setCursor(33,3);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.println("READY");
    
    if (fSingleFire)
    {
      if (dartsPerPull == 1)
      {
        display.setTextSize(2);
        display.setCursor(10,20);
        display.println("SEMI-AUTO");
      }
      else
      {
        display.setTextSize(2);
        display.setCursor(5,20);
        display.println("MULTI-FIRE");
        drawDart(dartsPerPull);
      }
    }
    else
    {
      display.setTextSize(2);
      display.setCursor(10,20);
      display.println("FULL-AUTO");
    }

  display.drawFastHLine(0,50,127,WHITE);
  display.setTextSize(1);
  display.setCursor(21,53);
  display.print("-");
  display.drawFastVLine(41,52,22,WHITE);
  display.setCursor(53,53);
  display.print("MODE");
  display.drawFastVLine(83,52,22,WHITE);
  display.setCursor(105,53);
  display.print("+");
  
  if (buttonL.fell())
  {
    if (fSingleFire)
    {
      if (dartsPerPull > 1)
      {
        dartsPerPull--;
      }
    }
  }
  
  if (buttonM.fell())
    {
      fSingleFire = !fSingleFire;
      //Serial.println("Mode switch");
    }
  
  if (buttonR.fell())
  {
    if (fSingleFire)
    {
      if (dartsPerPull < 10)
      {
        dartsPerPull++;
      }
    }
  }

    // There is a soft lock here.  If the flywheel trigger is held down, spin up the flywheels.
    // When the trigger is pulled, start up the pusher motor and run it.
    // When the trigger is released, slow the pusher motor until the switch for 
    // the pusher is triggered.
    if (revTrigger.read() == LOW)
    {
    //Serial.println("flywheel start");
    SoftPWMSetPercent(FLYWHEEL_FET,flyPercent);
      if (fireTrigger.read() == LOW)
      {
        //Serial.println("firing start");
        fireTrigger.update();
        if (fSingleFire)
        {
          if (dartsPerPull == 1)
          {
            pushReturn.update();
            jamDoor.update();
            while (!pushReturn.fell())
            {
              if (jamDoor.fell())
               {
                 return;
               }
               SoftPWMSet(PUSHER_FET,pushPercent);
            }
            resetPusher();
            SoftPWMSet(PUSHER_FET,0);
          }
          else
          {
            byte dartsRemaining = dartsPerPull;
            
            while (dartsRemaining > 0)
            {
               curTime = millis();
               pushReturn.update();
               jamDoor.update();
               if (jamDoor.fell())
               {
                 return;
               }
               if (curTime - prevTime > 50)
                 {
                   display.clearDisplay();
                   drawBorders();
                   display.setCursor(33,3);
                   display.setTextSize(2);
                   display.setTextColor(WHITE);
                   display.println("READY");        
                   display.setTextSize(2);
                   display.setCursor(5,20);
                   display.println("MULTI-FIRE");
                   drawDart(dartsRemaining);
                   display.display();
                   prevTime = curTime;
                 }
              //fire
              SoftPWMSet(PUSHER_FET,pushPercent);
              if (pushReturn.fell())
              {
                dartsRemaining--;
              }
            }
            resetPusher();
            SoftPWMSet(PUSHER_FET,0);       
          }
        }
        else
        {
          while (!fireTrigger.rose())
          {
            SoftPWMSet(PUSHER_FET,pushPercent);
            //Serial.println("firing continue");
            delay(100);
            fireTrigger.update();
          }
        }
        //Serial.println("firing stop");
        resetPusher();
        SoftPWMSet(PUSHER_FET,0);
        delay(500);
      }
    }
    else
    {
    //Serial.println("all stop");
      SoftPWMSetPercent(FLYWHEEL_FET,0);
      resetPusher();
    }
  }
  
  if (curTime - prevTime > 50)
  {
    display.display();
    prevTime = curTime;
  }
}


