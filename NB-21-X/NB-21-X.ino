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
#define BUTTON_L     2 // Right control button
#define BUTTON_M     3 // Middle control button
#define BUTTON_R     4 // Left control button
#define REV_TRIGGER  5 // Flywheel spinup trigger
#define FIRE_TRIGGER 6 // Fire trigger
#define PUSH_RETURN  7 // Pusher motor return switch
#define MAG_SENSOR   8 // Switch for detecting Magazine presence
#define DART_SENSOR  12 // LED sensor for presence of dart

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
bool fSingleFire = true;


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
  display.drawFastHLine(0,0,127,WHITE);
  display.drawFastVLine(127,0,64,WHITE);
  display.drawFastHLine(0,63,127,WHITE);
  display.drawFastVLine(0,0,63,WHITE);
}

void drawDart(uint8_t numOfDarts)
{
	// A dart drawing is 12 pixels wide and 5 pixels high.
	// XXXXXXXXXXX
	// X       X  X
	// X       X   X
	// X       X  X
	// XXXXXXXXXXX
	for (int iOffset = 0; iOffset < numOfDarts; iOffset++)
	{
	  int iMove = iOffset * 12;
	display.drawFastHLine(3 + iMove,42,10,WHITE);
	display.drawFastHLine(3 + iMove,46,10,WHITE);
	display.drawFastVLine(3 + iMove,42,5,WHITE);
	display.drawFastVLine(10 + iMove,42,5,WHITE);
	display.drawFastVLine(13 + iMove,43,3,WHITE);
	}
}

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
  
  revTrigger.update();
  fireTrigger.update();
  pushReturn.update();
  magSensor.update();
  buttonL.update();
  buttonM.update();
  buttonR.update();

  unsigned long curTime = millis();

  if (true)//(!checkErrors())// This is error checking.
  {
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
	  Serial.println("Mode switch");
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
		Serial.println("flywheel start");
		SoftPWMSetPercent(FLYWHEEL_FET,50);
		if (fireTrigger.read() == LOW)
		{
			Serial.println("firing start");
			fireTrigger.update();
			while (!fireTrigger.rose())
			{
				SoftPWMSet(PUSHER_FET,50);
				Serial.println("firing continue");
				delay(100);
				fireTrigger.update();
			}
			Serial.println("firing stop");
			SoftPWMSet(PUSHER_FET,0);
			delay(500);
		}
	}
    else
    {
		Serial.println("all stop");
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


