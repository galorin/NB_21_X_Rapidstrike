#include <SoftPWM.h>
//#include <SoftPWM_timer.h>
#include <Bounce2.h>
//#include <SPI.h>
//#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
//#define OLED_MOSI   9
//#define OLED_CLK   10
//#define OLED_DC    11
//#define OLED_CS    12
//#define OLED_RESET 13
//Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define FLYWHEEL_FET  19 //A6
#define PUSHER_FET 22 //A7

//#define BUTTON1 23 //A0
//#define BUTTON2 24 //A1
//#define BUTTON3 25 //A2

#define REV_TRIGGER 32 //D2
#define FIRE_TRIGGER 1 //D3
//#define JAM_DOOR 2     //D4
#define PUSHER 9 //D5
//#define DART_SENSOR 10 //D6
//#define MAG_SENSOR 11 //D7

Bounce revTrigger = Bounce();
Bounce fireTrigger = Bounce();
//Bounce jamDoor = Bounce();
Bounce pusher = Bounce();
//Bounce magSensor = Bounce();
//Bounce but1 = Bounce();
//Bounce but2 = Bounce();
//Bounce but3 = Bounce();

//bool blinkOn = true;
//bool fDartSeen = true;

//int flyPercent = 0;
//int pushPercent = 0;

//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h!");
//#endif

void setup()   {                
  Serial.begin(9600);
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
//  display.begin(SSD1306_SWITCHCAPVCC);
  // init done  
//  display.clearDisplay();
//  display.setTextColor(WHITE);

  pinMode(REV_TRIGGER, INPUT_PULLUP);
  revTrigger.attach(REV_TRIGGER);
  revTrigger.interval(5);
  
  pinMode(FIRE_TRIGGER, INPUT_PULLUP);
  fireTrigger.attach(FIRE_TRIGGER);    
  fireTrigger.interval(5);
  
  pinMode(PUSHER, INPUT_PULLUP);
  pusher.attach(PUSHER);
  pusher.interval(5);

  SoftPWMBegin();
  SoftPWMSet(FLYWHEEL_FET,0);
  SoftPWMSetFadeTime(FLYWHEEL_FET,100,500);
  
  SoftPWMSet(PUSHER_FET,0);

  //pinMode(JAM_DOOR, INPUT_PULLUP);  
  //jamDoor.attach(JAM_DOOR);
  //jamDoor.interval(5);

  //pinMode(BUTTON1, INPUT_PULLUP);
  //but1.attach(BUTTON1);
  //but1.interval(5);
  
  //pinMode(BUTTON2, INPUT_PULLUP);
  //but2.attach(BUTTON2);
  //but2.interval(5);
  
  //pinMode(BUTTON3, INPUT_PULLUP);
  //but3.attach(BUTTON3);
  //but3.interval(5);

  //pinMode(DART_SENSOR, INPUT_PULLUP);
}

void loop() {
 revTrigger.update();
 fireTrigger.update();
 pusher.update();
 //jamDoor.update();
 //but1.update();
 //but2.update();
 //but3.update();

 int revOn = revTrigger.read();
 int fireOn = fireTrigger.read();
 int pusherOn = pusher.read();

 if (revOn == LOW)
 {
  SoftPWMSetPercent(FLYWHEEL_FET,50);
  if (fireOn == LOW)
  {
    SoftPWMSet(PUSHER_FET,50);
  }
  else
  {
    while (pusherOn == HIGH)
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
 //int jamDoorOn = jamDoor.read();
 //int but1On = but1.read();
 //int but2On = but2.read();
 //int but3On = but3.read();

}
