
/*
  
Taken from http://nerfhaven.com/forums/topic/25012-tr-27-gryphon-cr-18-rapidstrike-mod/
  comment # 16 by Technician Gimmick, posted on Posted 17 May 2014 - 09:56 AM
 
4-23-14
Fixed secret mode so that it will only run identified mags.
Added Variables to identify digits for display.
Added code to indicate if the plunger is out by turning on the RightDigit's decimal point.
4/24/14
Modified Sleep Function to require the lid be opened to wake it from sleep.
Added Safty to Gun Structure and incorperated it into Action Function to bring everything to a quick stop.
Also changed Lid function to turn on/off the safty as well as removed the 2 second delay during brightness adjustment.
Fixed Left/Right Digit in Ammo Function
Found out what my gun needs to run at (noted below) with a plunger that has no resistors.
4/25/14
Negitive counter. -done
PlungerBurst Variables for both flywheel and plunger -done
3rd Reed switch
Make sure it doesn't spin up if it is out of ammo ??
Wakeup happens even if the lid is closed. -done
4/26/14
Fix Spin up problem. (When you start spinup while the motors are still spinning down.) -done!
Indicator problem with Unloaded AND Lid open. -done
Fix when the CheckAmmo looks to see if it is loaded. -maybe working right.
Fixed wake from sleep to display correctly -done
4/27/14
Added Diodes to Motors
Added 3rd Reed switch for detecting upto 7 different mag capacities.
5/2/14
Don't let the gun go into loser Mode if it is still spinning from other modes. -done. Not tested.
Don't let the gun leave loser Mode unless the gun has finished it's burst and spun down entirely. -done. Not Tested.
Don't Fire unless the gun has been loaded for 10ms -done
Added 10 & 25 Round clips to reed switch setup.
Measure the average time it takes from start of the plunger's burst run, to when the plunger stop switch goes high.
Then change the action fuction so that it only allows so many milliseconds of burst after the switch goes high.
i.e. When functioning correctly, the plunger begins it's burst, and 30ms into the plunge, the plunger stop switch
goes high 50ms after that, the plunger finishes it's burst, and applys the brakes.
In situations where the plunger has slid past the normal stopping point, it will cut short the plunger's burst by as much
as 30ms. Hopfully this will help prevent the plunger from further sliding past the stopping point.
loser Mode does not spin down if a burst is inturupted.
Burst shoud not count trigger pulls. - done
5/4/14
Changed the LoadedDelay to be 30ms instead of 10ms. Again my hope is that it will help prevent jams. -done
Change the Out of Ammo indicator to show dashes on the display instead of blinking the left DP. -done
5/5/14
Change the plunger so that auto correct happens as soon as no ammo is detected regarless of the position of the lid.
5/10/14
-There seems to be a problem in detecting ammo. Looking at some of the vidoes we make of Josh's gun firing the 35rnd mag, it seems to
go into an unloaded state often. At least in secret mode. Perhaps I can set a 100ms delay in releasing the ammo counter on Reed sensor
information? -done the problem was in the Ammo funciton. I didn't add the 3rd reed switch to it.
*/

#include "LedControl.h"
//LedControl(DataIn, CLK, LOAD, Number Of MAX7219s)
LedControl Display=LedControl(A5, A4, 12, 1);
const boolean RightDigit = 1;
const boolean LeftDigit = 0;
const static byte DazzleChar[128] = {B01100000, B00110000, B00011000, B00001100, B00000110, B01000010};
unsigned long DazzleTimer;

struct SWITCH {
    const int Pin;
    boolean Value;
    byte Average;
    unsigned long NextRead;
    byte ReadFrequency;
};

struct MOTOR {
    const byte Enable;
    const byte Output1;
    const byte Output2;
    int Speed; //-256 to 256. 1 and -1 are coast. 0 is brake 256 is full forward, -256 is full reverse.
};
MOTOR Piston = {3,2,13,0};

struct BLINKABLE {
    boolean LeftDP;
    boolean LeftDPValue;
    boolean RightDP;
    boolean RightDPValue;
    boolean Display;
    boolean DisplayValue;
    boolean Dashes;
    boolean DashesValue;
};

struct NERF {
    const byte FlywheelPin;
    const byte PlungerPin;
    const byte BrakePin;
    const byte IrPin;
    boolean Safty;
    boolean Secret;
    boolean Loaded;
    unsigned long LoadedDelay;
    boolean Jam;
    unsigned long Pending;
    byte Firing;
    unsigned long NextShot;
    unsigned long NextStandby;
    int Ammo;
    byte BurstCount;
    unsigned long Sleep;
    int Brightness;
    boolean CountDown;
    int ShotsFired;
    int ShotsPending;
    int Reloads;
    byte FlywheelSpeed;
    unsigned long FlywheelNext;
    byte FlywheelStep;
    int PlungerSpeed;
    unsigned long PlungerNext;
    byte PlungerStep;
};

NERF Gun = {5, 3, 2, 6};
SWITCH Reed1 = {7,HIGH,10,0,4};
SWITCH Reed2 = {8,HIGH,10,0,4};
SWITCH Reed3 = {A3,HIGH,10,0,4};
SWITCH Trigger = {4,HIGH,10,0,5};
SWITCH Warmup = {A1,HIGH,10,0,5};
SWITCH PlungerStop = {A0,HIGH,10,0,1};
SWITCH Lid = {9,HIGH,0,0,5};
SWITCH Loaded = {A2,HIGH,10,0,3};
SWITCH Auto = {11,LOW,0,0,10};
SWITCH Burst = {10,HIGH,10,0,10};
//SWITCH Auto = {A5,LOW,0,0,10};
//SWITCH Burst = {A4,HIGH,10,0,10};
BLINKABLE Blinkables ={false, false, false, false};
BLINKABLE BlinkablesTemp ={false, false, false, false};

byte Mode = 0;
unsigned long Timer;
unsigned long Now;
unsigned long ModeTimer;
unsigned long bla;
unsigned long BlinkTimer;
byte BlinkStep = 0;
int secrets = 0;
boolean BrightnessReset = false;
/* the following section seemed to work running at 130~ms per shot.
80ms @ 255
20ms with brake on
contienue on 30
*/

const int PlungerBurst = 0;
const int PlungerBrake = 80;
const int FlywheelBurst = 250;

boolean RunMotor(MOTOR &Motor, int NewSpeed);
void Dazzle(unsigned long Finished);
void Ammo(int AmmoCount);
boolean CheckTrigger(boolean Print);
boolean Check(SWITCH &Switch, boolean Print);
void CheckMode(byte &CurrentMode, unsigned long &timer);
boolean CheckLid();
void Sleep();
boolean CheckAmmo();
void Blinker(unsigned long &BlinkTimer, byte &BlinkStep, BLINKABLE &Items);
void Action(byte FlywheelTarget, byte BurstAmount, int BurstDelay, int StandbyTime);

void setup() {
    pinMode(Reed1.Pin, INPUT_PULLUP);
    pinMode(Reed2.Pin, INPUT_PULLUP);
    pinMode(Reed3.Pin, INPUT_PULLUP);
    pinMode(Trigger.Pin, INPUT_PULLUP);
    pinMode(Warmup.Pin, INPUT_PULLUP);
    pinMode(PlungerStop.Pin, INPUT_PULLUP);
    pinMode(Lid.Pin, INPUT_PULLUP);
    pinMode(Loaded.Pin, INPUT);
    pinMode(Auto.Pin, INPUT_PULLUP);
    pinMode(Burst.Pin, INPUT_PULLUP);
    pinMode(Gun.FlywheelPin, OUTPUT);
    pinMode(Gun.IrPin, OUTPUT);
    pinMode(Piston.Enable, OUTPUT);
    pinMode(Piston.Output1, OUTPUT);
    pinMode(Piston.Output2, OUTPUT);
    Gun.Secret = false;
    Gun.Loaded = false;
    Gun.Safty = false;
    Gun.Jam = false;
    Gun.Pending = 0;
    Gun.Firing = 0;
    Gun.NextShot = 0;
    Gun.NextStandby = 0;
    Gun.Ammo = 0;
    Gun.BurstCount = 0;
    Gun.Sleep = 360000;
    Gun.Brightness = 7;
    Gun.CountDown = false;
    Gun.ShotsFired = 0;
    Gun.ShotsPending = 0;
    Gun.Reloads = 0;
    Gun.FlywheelSpeed = 0;
    Gun.FlywheelNext = 0;
    Gun.FlywheelStep = 0;
    Gun.PlungerSpeed = 0;
    Gun.PlungerNext = 0;
    Gun.PlungerStep = 0;
    Display.shutdown(0,false);
    Display.setIntensity(0,Gun.Brightness);
    Display.clearDisplay(0);
    Dazzle(3000);
    Ammo(Gun.Ammo);
    Serial.begin(9600); 
    Serial.println("Good Morning Commander!");
    //Indicator.OnTime = 70;
    //Indicator.OffTime = 130;
    Serial.println("=================CHECKING SYSTEM...");
    Timer = millis() + 1000;
    do{
        Check(Reed1, false);
        Check(Reed2, false);
        Check(Reed3, false);
        Check(Trigger, false);
        Check(Warmup, false);
        Check(PlungerStop, false);
        Check(Lid, false);
        Check(Loaded, false);
        Check(Auto, false);
        Check(Burst, false);
    } while(millis() <= Timer);
    Reed1.Value = !Reed1.Value;
    Reed2.Value = !Reed2.Value;
    Reed3.Value = !Reed3.Value;
    Trigger.Value = !Trigger.Value;
    Warmup.Value = !Warmup.Value;
    Lid.Value = !Lid.Value;
    Loaded.Value = !Loaded.Value;
    Auto.Value = !Auto.Value;
    Burst.Value = !Burst.Value;
    Timer = millis() + 1000;
    Blinkables.Dashes = true;
    Blinkables.Display = true;
    do{
        Now = millis();
        Check(Reed1, true);
        Check(Reed2, true);
        Check(Reed3, true);
        Check(Trigger, true);
        Check(Warmup, true);
        Check(PlungerStop, true);
        Check(Loaded, true);
        Check(Auto, false);
        Check(Burst, false);
        CheckLid();
        Blinker(BlinkTimer, BlinkStep, Blinkables);
    } while(Now <= Timer || Loaded.Value == LOW || Trigger.Value == LOW || Gun.Safty);
    Blinkables.Dashes = false;
    Blinkables.Display = false;
    CheckMode(Mode, ModeTimer);
    Serial.println("=================SYSTEM READY!");
    Serial.println("Awaiting your orders sir.");
}

void loop() {
    Now = millis();
    Check(PlungerStop, false);
    Check(Burst, false);
    Check(Auto, false);
    if(Gun.Safty)
	  {Blinkables.RightDP = true;}
    else
	  {Blinkables.RightDP = false;};
    if(!Gun.Loaded && !Blinkables.Dashes && !Blinkables.RightDP)
	  {Blinkables.Dashes = true; 
      Blinkables.Display = true;}
    else if(Gun.Loaded && Blinkables.Dashes)
	  {Blinkables.Dashes = false;
      Blinkables.Display = false;}
    Blinker(BlinkTimer, BlinkStep, Blinkables);
    Sleep();
    CheckLid();
    if(Gun.PlungerStep == 0)
	  {CheckMode(Mode, ModeTimer);}
    CheckAmmo();
    switch(Mode){
    case 1:
        //3 BURST
        Action(140, 3, 300, 2500);
        if((!Gun.Safty && !Gun.Jam) && (Gun.ShotsPending == 0 || (Gun.ShotsPending == 1 && Gun.FlywheelStep < 2))){
            CheckTrigger(true);
        };
        break;
    case 2:
        //FULL AUTOMATIC
        Action(130, 1, 100, 2500);
        if(!Gun.Safty && !Gun.Jam){CheckTrigger(true);};
        break;
    case 3:
        //loser
        Action(254, 2, 2500, 2000);
        if(!Gun.Safty && !Gun.Jam){CheckTrigger(true);};
        break;
    case 4:
        //SECRET "EMPTY THE GUN MODE"
        Action(254, Gun.Ammo, 50, 80);
        if(Gun.Ammo == 0 && !Gun.Firing){
            Gun.Secret = false;
            Gun.ShotsPending = 0;
        };
        if(!Gun.Safty && !Gun.Jam){CheckTrigger(true);};
        break;
    };
    //analogWrite(Gun.FlywheelPin, Gun.FlywheelSpeed);
    RunMotor(Piston, Gun.PlungerSpeed);
}

void Action(byte FlywheelTarget, byte BurstAmount, int BurstDelay, int StandbyTime){
    Now = millis();
    if(!Gun.Safty && BurstAmount > 0 && Gun.Loaded && Gun.PlungerStep == 0 && !Gun.Jam && Gun.FlywheelStep == 0 && Gun.ShotsPending > 0 && Now >= Gun.FlywheelNext){
        if(Mode == 3){//loser
            if(Gun.FlywheelSpeed == 0){Serial.println("Flywheel Warming Up");};
            Gun.FlywheelNext = Now + (3000 / FlywheelTarget);
            Gun.FlywheelSpeed++;
            if(Gun.FlywheelSpeed >= 50){
                Gun.FlywheelStep++;
            };
        }else{//Other
            Serial.println("Flywheel Warming Up");
            if(Now >= Gun.NextShot){
                Gun.FlywheelSpeed = 255;
            }else{
                Gun.FlywheelSpeed = FlywheelTarget;
            };
            Gun.FlywheelStep++;
            Gun.FlywheelNext = Now + FlywheelBurst;
        };
        Gun.NextStandby = Now + StandbyTime;
    };
    if(Gun.FlywheelStep == 1 && Now >= Gun.FlywheelNext){
        if(Mode == 3){//loser
            if(Gun.FlywheelSpeed == FlywheelTarget){
                Serial.println("Gun at minimum firing speed.");
                Gun.FlywheelStep++;
                Gun.FlywheelStep++;
                Gun.ShotsPending--;
            }else{
                Gun.FlywheelNext = Now + (3000 / FlywheelTarget);
                Gun.FlywheelSpeed++;
            };
        }else{//Other
            Serial.println("Gun spinning and ready to fire.");
            Gun.FlywheelNext = Now + 150;
            Gun.FlywheelStep++;
            Gun.ShotsPending--;
        };
        Gun.NextStandby = Now + StandbyTime;
    };
    if(Gun.FlywheelStep == 2 && Now >= Gun.FlywheelNext){
        Serial.println("Flywheel Boost Complete");
        Gun.FlywheelSpeed = FlywheelTarget;
        Gun.FlywheelStep++;
        Gun.NextStandby = Now + StandbyTime;
    };
    if(Gun.FlywheelStep == 3 && Gun.PlungerStep == 0 && (Now >= Gun.NextStandby || Gun.Safty)){
        if(Mode == 3){//loser
            if(Gun.Safty){
                Gun.FlywheelSpeed = 0;
                Gun.FlywheelStep = 0;
            }else if(Gun.ShotsPending == 0){
                Serial.println("No trigger pull. Auto Firing.");
                Gun.ShotsPending++;
            };
        }else{//Other
            Serial.println("No trigger pull. Standing Down.");
            Gun.NextShot = Now + 1000;
            Gun.FlywheelSpeed = 0;
            Gun.FlywheelStep = 0;
        };
    };
    if(!Gun.Safty && Gun.FlywheelStep >= 2 && Gun.PlungerStep == 0 && Gun.Loaded && Gun.Firing == 0 && Gun.ShotsPending > 0 && Lid.Value == HIGH && Now >= Gun.NextShot && PlungerStop.Value == LOW){
        if(Gun.BurstCount == 0){Gun.BurstCount = BurstAmount;};
        bla = Now;
        Serial.print("Firing ");
        Serial.print(Gun.BurstCount);
        Serial.println(" shots");
        Gun.PlungerSpeed = 256;
        Gun.PlungerStep = 1;
        //Gun.PlungerNext = Now + PlungerBurst;
        Gun.Firing++;
    };
    if(Now >= Gun.PlungerNext && Gun.PlungerStep == 2){
        // if(!Gun.Secret || (Gun.Secret && Gun.Ammo == 1)){
        // };
        Gun.PlungerStep = 0;
        Gun.PlungerSpeed = 0;
    };
    // if(Now >= Gun.PlungerNext && Gun.PlungerStep == 2){
    // if(!Gun.Secret || (Gun.Secret && Gun.Ammo == 1)){
    // Gun.PlungerSpeed = 90;
    // };
    // Gun.PlungerStep++;
    // };
    if(Gun.Firing == 1 && PlungerStop.Value == HIGH){
        Gun.Firing++;
        if(Gun.CountDown){Gun.Ammo--;}else{Gun.Ammo++;};
        Ammo(Gun.Ammo);
        Gun.ShotsFired++;
        Serial.print("Available Ammo: ");
        Serial.println(Gun.Ammo);
        Serial.print("Shots Fired: ");
        Serial.println(Gun.ShotsFired);
        if(Mode != 3){
            Gun.FlywheelSpeed = 255;
            Gun.FlywheelStep = 2;
            Gun.FlywheelNext = Now + 50;
        };
        Gun.NextStandby = Now + StandbyTime;
    };
    if(Gun.Firing == 2 && PlungerStop.Value == LOW){
        Serial.print("CycleTime: ");
        Serial.println(Now - bla);
        bla = Now;
        Gun.BurstCount--;
        Gun.Firing = 1;
        Gun.NextStandby = Now + StandbyTime;
        if(Gun.BurstCount == 0){
            Serial.println("Burst Fire Complete");
            if(Mode == 3){
                Gun.ShotsPending = 0;
                Gun.FlywheelSpeed = 0;
                Gun.FlywheelStep = 0;
            }else{
                Gun.ShotsPending--;
            };
            Gun.NextShot = Now + BurstDelay;
            Gun.Firing = 0;
            Gun.PlungerStep++;
            Gun.PlungerSpeed = -256;
            Gun.PlungerNext = Now + PlungerBrake;
        }else{
            if(!Gun.Loaded || Gun.Safty){
                Serial.println("Firing Halted");
                Gun.Firing = 0;
                Gun.PlungerStep = 0;
                Gun.PlungerSpeed = 0;
            }else{
                Gun.PlungerSpeed = 256;
                Gun.PlungerStep = 1;
            };
        };
    };
    if(PlungerStop.Value == HIGH && Gun.PlungerStep == 0){
        Blinkables.Display = true;
        if(!Gun.Jam){
            Serial.println("Plunger Stuck Out!");
            Gun.Jam = true;
            Display.setLed(0,RightDigit,0,true);
        };
        if(Gun.Safty || !Gun.Loaded){
            Gun.PlungerSpeed = 101;
        };
    }else if(PlungerStop.Value == LOW && Gun.PlungerStep == 0 && Gun.Jam){
        Gun.Jam = false;
        Blinkables.Display = false;
        Gun.PlungerSpeed = 0;
    };
};

boolean Check(SWITCH &Switch, boolean Print){
    unsigned long Now = millis();
    if(Switch.Pin == Loaded.Pin && Now >= Switch.NextRead - 1){digitalWrite(Gun.IrPin, HIGH);};
    if(Now >= Switch.NextRead){
        Switch.NextRead = Now + Switch.ReadFrequency;
        if(digitalRead(Switch.Pin) == HIGH){
            if(Switch.Average < 10){Switch.Average++;};
        }else{
            if(Switch.Average > 0){Switch.Average--;};
        };
        if(Switch.Pin == Loaded.Pin){digitalWrite(Gun.IrPin, LOW);};
        boolean NewValue;
        if(Switch.Average >= 5){NewValue = true;
        }else{NewValue = false;};
        if(Switch.Value != NewValue){
            Switch.Value = NewValue;
            if(Print){
                if(Switch.Pin == Reed1.Pin){ Serial.print("Reed Switch 1: ");}
                else if(Switch.Pin == Reed2.Pin){ Serial.print("Reed Switch 2: ");}
                else if(Switch.Pin == Reed3.Pin){ Serial.print("Reed Switch 3: ");}
                else if(Switch.Pin == Trigger.Pin){ Serial.print("Trigger Switch: ");}
                else if(Switch.Pin == Warmup.Pin){ Serial.print("Warmup Switch: ");}
                else if(Switch.Pin == PlungerStop.Pin){ Serial.print("Plunger Stop Switch: ");}
                else if(Switch.Pin == Lid.Pin){ Serial.print("Lid Switch Switch: ");}
                else if(Switch.Pin == Loaded.Pin){ Serial.print("Loaded Sensor: ");}
                else if(Switch.Pin == Auto.Pin){ Serial.print("Auto Switch: ");}
                else if(Switch.Pin == Burst.Pin){ Serial.print("Burst Sensor: ");}
                Serial.println(Switch.Value);
            };
            return true;
        };
    };
    return false;
};

boolean CheckLid(){
    if(Check(Lid, false)){
        if(Lid.Value == LOW){
            Serial.println("Lid Open. Safty On.");
            Gun.Safty = true;
            Gun.Secret = false;
            BlinkablesTemp = Blinkables;
            Blinkables.Dashes = false;
            Blinkables.Display = false;
        }else if(Lid.Value == HIGH){
            Serial.println("Lid Closed. Safty Off.");
            Ammo(Gun.Ammo);
            BrightnessReset = false;
            Gun.Safty = false;
            Blinkables = BlinkablesTemp;
        };
    };
    if(Lid.Value == LOW){
        Now = millis();
        if(Check(Trigger, true) && Trigger.Value == LOW && Warmup.Value == HIGH){
            if(BrightnessReset == false){
                Gun.Brightness = -2;
                BrightnessReset = true;
            };
            Gun.Brightness++;
            if(Gun.Brightness == 16){Gun.Brightness = -1;};
            Ammo(Gun.Brightness);
            Serial.println(Gun.Brightness);
        };
        if(Check(Warmup, true) && Warmup.Value == LOW){
            if(Gun.Brightness == 3 && secrets == 0){secrets++;}else
                if(Gun.Brightness == 5 && secrets == 1){secrets++;}else
                    if(Gun.Brightness == 7 && secrets == 2){secrets++;}else
                        if(Gun.Brightness == 11 && secrets == 3){
                            Gun.Secret = true;
                            Dazzle(1000);
                            Ammo(Gun.Ammo);
                        }else{
                            secrets = 0;
                        };
        };
    };
};

void CheckMode(byte &CurrentMode, unsigned long &TimeToNextRun){
    //This Function uses Gun's Structure.
    if(millis() < TimeToNextRun){return;};
    TimeToNextRun = millis() + Auto.ReadFrequency;
    if(Gun.Secret == false){
        if(Auto.Value == LOW && Burst.Value == HIGH){
            if((CurrentMode == 3 && Gun.FlywheelStep == 0 && Gun.BurstCount == 0 && Now >= Gun.NextShot) || (CurrentMode != 1 && CurrentMode != 3)){
                if(CurrentMode == 0){
                    Blinkables.Display = false;
                };
                Serial.println("MODE:Burst");
                CurrentMode = 1;
            }
        }else if(Auto.Value == HIGH && Burst.Value == LOW){
            if(((CurrentMode == 1 || CurrentMode == 2) && Gun.FlywheelStep == 0 && Gun.BurstCount == 0 && Now >= Gun.NextShot) || (CurrentMode == 4 || CurrentMode == 0)){
                if(CurrentMode == 0){
                    Blinkables.Display = false;
                };
                Serial.println("MODE:loser");
                CurrentMode = 3;
            };
        }else if(Auto.Value == HIGH && Burst.Value == HIGH){
            if((CurrentMode == 3 && Gun.FlywheelStep == 0 && Gun.BurstCount == 0 && Now >= Gun.NextShot) || (CurrentMode != 2 && CurrentMode != 3)){
                if(CurrentMode == 0){
                    Blinkables.Display = false;
                };
                Serial.println("MODE:Auto");
                CurrentMode = 2;
            };
        }else{
            if(CurrentMode != 0){
                Serial.println("MODE:ERROR!!!");
                Blinkables.Display = true;
                CurrentMode = 0;
            };
        };
    }else{
        if(CurrentMode != 4){
            Serial.println("MODE:EMPTY THE GUN!");
            CurrentMode = 4;
        };
    };
};

boolean CheckAmmo(){
    if(PlungerStop.Value == LOW || Gun.PlungerStep > 2){
        if(Check(Loaded, true) && Loaded.Value == LOW){
            Gun.LoadedDelay = millis() + 40;
        };
        Check(Reed1, true);
        Check(Reed2, true);
        Check(Reed3, true);
    };
    if(!Gun.Loaded && PlungerStop.Value == LOW && Loaded.Value == LOW && millis() >= Gun.LoadedDelay){
        Now = millis();
        Serial.println("Ammo Detected");
        Gun.Loaded = true;
        if(Gun.Ammo == 0){Gun.Reloads++;};
        Serial.print("Total Reloads: ");
        Serial.println(Gun.Reloads);
        Timer = millis() + 30;
        do{
            Check(Reed1, true);
            Check(Reed2, true);
            Check(Reed3, true);
        }while(millis() < Timer);
        if(Gun.Ammo == 0){
            if(Reed1.Value == LOW && Reed2.Value == HIGH && Reed3.Value == HIGH){
                Gun.Ammo = 6;
                Gun.CountDown = true;
            }else if(Reed1.Value == HIGH && Reed2.Value == LOW && Reed3.Value == LOW){
                Gun.Ammo = 10;
                Gun.CountDown = true;
            }else if(Reed1.Value == LOW && Reed2.Value == LOW && Reed3.Value == HIGH){
                Gun.Ammo = 12;
                Gun.CountDown = true;
            }else if(Reed1.Value == HIGH && Reed2.Value == LOW && Reed3.Value == HIGH){
                Gun.Ammo = 18;
                Gun.CountDown = true;
            }else if(Reed1.Value == LOW && Reed2.Value == HIGH && Reed3.Value == LOW){
                Gun.Ammo = 25;
                Gun.CountDown = true;
            }else if(Reed1.Value == HIGH && Reed2.Value == HIGH && Reed3.Value == LOW){
                Gun.Ammo = 35;
                Gun.CountDown = true;
            }else if(Reed1.Value == HIGH && Reed2.Value == HIGH && Reed3.Value == HIGH){
                Gun.Ammo = 0;
                Gun.CountDown = false;
            };
        };
        if(Gun.CountDown == false){Serial.println("Unknown Mag Inserted");}
        else{Serial.print(Gun.Ammo);Serial.println(" Round Mag Inserted");};
        Ammo(Gun.Ammo);
        return true;
    }else if(PlungerStop.Value == LOW && Loaded.Value == HIGH && Gun.Loaded){
        Now = millis();
        Gun.Loaded = false;
        Gun.Pending = Now + 100;
        Gun.Pending = Now + 1000;
        Gun.Sleep = Now + 360000;
    };
    if(!Gun.Loaded){
        if(Gun.BurstCount != 0 || Gun.Ammo != 0 || Gun.ShotsPending != 0){
            Now = millis();
            if(Gun.CountDown){
                if(Reed1.Value == HIGH && Reed2.Value == HIGH && Reed3.Value == HIGH){
                    Serial.println("Ammo Expired");
                    Gun.Ammo = 0;
                    Gun.BurstCount = 0;
                    Gun.ShotsPending = 0;
                    Ammo(Gun.Ammo);
                };
            }else if(Now >= Gun.Pending){
                Serial.println("Ammo Expired");
                Gun.Ammo = 0;
                Gun.BurstCount = 0;
                Gun.ShotsPending = 0;
                Ammo(Gun.Ammo);
            };
        };
    };
    return false;
};

boolean CheckTrigger(boolean Print){
    if(Mode == 3){
        if(Check(Trigger, true)){
            if(Trigger.Value == LOW && Gun.Loaded && Gun.ShotsPending == 0){
                Gun.ShotsPending++;
                if(Print){
                    Serial.print("Shots Pending: ");
                    Serial.println(Gun.ShotsPending);
                };
            }
            if(Trigger.Value == HIGH && Gun.Loaded && Gun.FlywheelSpeed != 0){
                Gun.ShotsPending++;
                if(Print){
                    Serial.print("Shots Pending: ");
                    Serial.println(Gun.ShotsPending);
                };
            };
        };
    }else{// other
        if(Check(Trigger, true)){
            if(Trigger.Value == LOW && Gun.Loaded){
                Gun.ShotsPending++;
                if(Print){
                    Serial.print("Shots Pending: ");
                    Serial.println(Gun.ShotsPending);
                };
            };
        };
        if(Gun.Secret && Gun.ShotsPending == 1){
            Gun.ShotsPending = 2;
        }else if(millis() >= Gun.NextShot && Trigger.Value == LOW && Gun.ShotsPending == 0 && Gun.Loaded){
            Gun.ShotsPending = 1;
            if(Print){
                Serial.print("Shots Pending: ");
                Serial.println(Gun.ShotsPending);
            };
        };
    };
};

boolean RunMotor(MOTOR &Motor, int NewSpeed){
    constrain(NewSpeed, -256, 256);
    if(Motor.Speed != NewSpeed){
        Serial.print("Speed: ");
        Serial.println(NewSpeed);
        Motor.Speed = NewSpeed;
        if(Motor.Speed == 0){
            analogWrite(Motor.Enable, 0);
            digitalWrite(Motor.Output1, HIGH);
            digitalWrite(Motor.Output2, HIGH);
            analogWrite(Motor.Enable, 255);
            Serial.println("Brake");
        }else if(Motor.Speed > 0){
            analogWrite(Motor.Enable, 0);
            digitalWrite(Motor.Output1, HIGH);
            digitalWrite(Motor.Output2, LOW);
            analogWrite(Motor.Enable, Motor.Speed - 1);
        }else if(Motor.Speed < 0){
            analogWrite(Motor.Enable, 0);
            digitalWrite(Motor.Output1, LOW);
            digitalWrite(Motor.Output2, HIGH);
            analogWrite(Motor.Enable, (Motor.Speed * -1) - 1);
        };
        return true;
    };
    return false;
};

void Ammo(int AmmoCount){
    if(Gun.Brightness >= 0){
        Display.shutdown(0,false);
        Display.setIntensity(0,Gun.Brightness);
        if(AmmoCount < 0){
            Display.setChar(0,LeftDigit,'-',false);
            Display.setDigit(0,RightDigit,AmmoCount * -1,false);
        }else{
            Display.setDigit(0,LeftDigit,(AmmoCount*.1),false);
            while(AmmoCount >= 10){AmmoCount = AmmoCount - 10;};
            Display.setDigit(0,RightDigit,AmmoCount,false);
        };
    }else{ 
        Display.setIntensity(0,0);
        Display.clearDisplay(0);
        Display.shutdown(0,true);
    };
};

void Sleep(){
    if(!Gun.Loaded && millis() >= Gun.Sleep){
        Display.clearDisplay(0);
        Display.shutdown(0,true);
        Serial.println("Entering Sleep Mode...");
        do{
            delay(2000);
            Serial.print(".");
            delay(20);
        }while(digitalRead(Trigger.Pin) == HIGH || digitalRead(Lid.Pin) == HIGH);
        Serial.println("Exiting Sleep Mode");
        Display.clearDisplay(0);
        Display.shutdown(0,false);
        Gun.Sleep = millis() + 360000;
        int Brightness = Gun.Brightness;
        Gun.Brightness = 15;
        for(int i = 99; i != 0; i--){
            Ammo(i);
            delay(30);
        };
        Gun.Brightness = Brightness;
        Dazzle(3000);
        Ammo(Gun.Ammo);
    };
};

void Blinker(unsigned long &BlinkTimer, byte &BlinkStep, BLINKABLE &Items){
    if(millis() >= BlinkTimer){
        switch(BlinkStep){
        case 0:
            BlinkTimer = millis() + 70;
            if(Items.LeftDP){Items.LeftDPValue = true; Display.setLed(0,LeftDigit,0,true);};
            if(Items.RightDP){Items.RightDPValue = true; Display.setLed(0,RightDigit,0,true);};
            if(Items.Display || Items.DisplayValue){Items.DisplayValue = false; Display.shutdown(0,false);};
            if(Items.Dashes){
                Items.DashesValue = true;
                Display.setChar(0,LeftDigit,'-',false);
                Display.setChar(0,RightDigit,'-',false);
            }else if(!Items.Dashes && Items.DashesValue){
                Items.DashesValue = false;
                Ammo(Gun.Ammo);
            };
            break;
        case 1:
            BlinkTimer = millis() + 130;
            if(Items.LeftDP || Items.LeftDPValue){Items.LeftDPValue = false; Display.setLed(0,LeftDigit,0,false);};
            if(Items.RightDP || Items.RightDPValue){Items.RightDPValue = false; Display.setLed(0,RightDigit,0,false);};
            if(Items.Display){Items.DisplayValue = true; Display.shutdown(0,true);};
            if(Items.Dashes){
                Items.DashesValue = true;
                Display.setChar(0,LeftDigit,'-',false);
                Display.setChar(0,RightDigit,'-',false);
            }else if(!Items.Dashes && Items.DashesValue){
                Items.DashesValue = false;
                Ammo(Gun.Ammo);
            };
            break;
        case 2:
            BlinkTimer = millis() + 70;
            if(Items.LeftDP){Items.LeftDPValue = true; Display.setLed(0,LeftDigit,0,true);};
            if(Items.RightDP){Items.RightDPValue = true; Display.setLed(0,RightDigit,0,true);};
            if(Items.Display || Items.DisplayValue){Items.DisplayValue = false; Display.shutdown(0,false);};
            if(Items.Dashes){
                Items.DashesValue = true;
                Display.setChar(0,LeftDigit,'-',false);
                Display.setChar(0,RightDigit,'-',false);
            }else if(!Items.Dashes && Items.DashesValue){
                Items.DashesValue = false;
                Ammo(Gun.Ammo);
            };
            break;
        case 3:
            BlinkTimer = millis() + 130;
            if(Items.LeftDP || Items.LeftDPValue){Items.LeftDPValue = false; Display.setLed(0,LeftDigit,0,false);};
            if(Items.RightDP || Items.RightDPValue){Items.RightDPValue = false; Display.setLed(0,RightDigit,0,false);};
            if(Items.Display){Items.DisplayValue = true; Display.shutdown(0,true);};
            if(Items.Dashes){
                Items.DashesValue = true;
                Display.setChar(0,LeftDigit,'-',false);
                Display.setChar(0,RightDigit,'-',false);
            }else if(!Items.Dashes && Items.DashesValue){
                Items.DashesValue = false;
                Ammo(Gun.Ammo);
            };
            BlinkStep = 255;
            break;
        case 4:
            BlinkTimer = millis() + 600;
            if(Items.LeftDP || Items.LeftDPValue){Items.LeftDPValue = false; Display.setLed(0,LeftDigit,0,false);};
            if(Items.RightDP || Items.RightDPValue){Items.RightDPValue = false; Display.setLed(0,RightDigit,0,false);};
            if(Items.Display){Items.DisplayValue = true; Display.shutdown(0,true);};
            if(Items.Dashes){
                Items.DashesValue = true;
                Display.setChar(0,LeftDigit,'-',false);
                Display.setChar(0,RightDigit,'-',false);
            }else if(!Items.Dashes && Items.DashesValue){
                Items.DashesValue = false;
                Ammo(Gun.Ammo);
            };
            BlinkStep = 255;
            break;
        };
        BlinkStep++;
    };
};

void Dazzle(unsigned long Finished){
    Finished = millis() + Finished;
    byte a = 0;
    byte b = 5;
    Display.clearDisplay(0);
    Display.setIntensity(0,15);
    do{
        Display.setRow(0,LeftDigit,DazzleChar[b]);
        Display.setRow(0,RightDigit,DazzleChar[a]);
        if(millis() >= DazzleTimer){a++; b--; DazzleTimer = millis() + 30;};
        if(a==6){a = 0;};
        if(b==255){b = 5;};
    }while(millis() < Finished);
    Display.setIntensity(0,Gun.Brightness);
};
