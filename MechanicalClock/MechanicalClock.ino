#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "Adafruit_PWMServoDriver.h"      //Include library for servo driver

Adafruit_PWMServoDriver pwmH = Adafruit_PWMServoDriver(0x40);    //Create an object of Hour driver
Adafruit_PWMServoDriver pwmM = Adafruit_PWMServoDriver(0x41);    //Create an object of Minute driver (A0 Address Jumper)

int servoFrequency = 50;      //Set servo operating frequency

int segmentHOn[14] = {385, 375, 385, 375, 382, 375, 354, 367, 375, 385, 375, 368, 371, 375}; //On positions for each Hour servo
int segmentMOn[14] = {382, 395, 378, 315, 375, 340, 345, 380, 385, 365, 290, 365, 315, 365}; //On positions for each Minute servo
int segmentHOff[14] = {200, 200, 550, 480, 200, 520, 200, 200, 200, 480, 550, 200, 515, 200}; //Off positions for each Hour servo
int segmentMOff[14] = {200, 200, 550, 440, 200, 480, 200, 200, 200, 550, 450, 200, 430, 200}; //Off positions for each Minute servo
int digits[10][7] = {{1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 0, 0, 0, 0}, {1, 1, 0, 1, 1, 0, 1}
  , {1, 1, 1, 1, 0, 0, 1}, {0, 1, 1, 0, 0, 1, 1}, {1, 0, 1, 1, 0, 1, 1}
  , {1, 0, 1, 1, 1, 1, 1}, {1, 1, 1, 0, 0, 0, 0}, {1, 1, 1, 1, 1, 1, 1}
  , {1, 1, 1, 1, 0, 1, 1}
};    //Position values for each digit

// CONNECTIONS:
// DS1302 CLK/SCLK --> D5
// DS1302 DAT/IO --> D4
// DS1302 RST/CE --> D2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND

ThreeWire myWire(4, 5, 2); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

int hourTens = 8;                 //Create variables to store each 7 segment display numeral
int hourUnits = 8;
int minuteTens = 8;
int minuteUnits = 8;

int prevHourTens = 8;           //Create variables to store the previous numeral displayed on each
int prevHourUnits = 8;          //This is required to move the segments adjacent to the middle ones out of the way when they move
int prevMinuteTens = 8;
int prevMinuteUnits = 8;

int midOffset = 100;            //Amount by which adjacent segments to the middle move away when required

HardwareSerial FPGA(2);


char *CmdCode[] = {"RST", "SET", "SEG"};
String currentCmd = "XXX";
String currentData = "XXXXXXXXXXXXXX";

void setup()
{
  Serial.begin(115200);
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);

  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  pwmH.begin();                             //Start each board
//  pwmM.begin();
  pwmH.setOscillatorFrequency(27000000);    //Set the PWM oscillator frequency, used for fine calibration
//  pwmM.setOscillatorFrequency(27000000);
  pwmH.setPWMFreq(servoFrequency);          //Set the servo operating frequency
//  pwmM.setPWMFreq(servoFrequency);

  for (int i = 0 ; i <= 13 ; i++) //Set all of the servos to on or up (88:88 displayed)
  {
    pwmH.setPWM(i, 0, segmentHOn[i]);
    delay(10);
//    pwmM.setPWM(i, 0, segmentMOn[i]);
//    delay(10);
  }

  //Serial.begin(Baud Rate, Data Protocol, Rxd pin, Txd pin);
  FPGA.begin(115200, SERIAL_8N1, 16, 17);

  delay(2000);
}

void loop()
{
  RtcDateTime now = Rtc.GetDateTime();

  printDateTime(now);
  packageDateTime(now);

  if (!now.IsValid())
  {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }

  FPGA.print("A");
  String RxdString = "";
  delay(500); // Too fast, Cannot read on the first try.
  while (FPGA.available()) {
    RxdString = FPGA.readString();
  }
  
  if (RxdString != "") {
    Serial.println(RxdString);
//    currentCmd = RxdString.substring(14, 17);
//    currentData = RxdString.substring(0, 14);
//    Serial.println(currentCmd);
//    Serial.println(currentData);

//    switch (currentCmd) {
//      case CmdCode[2]: // RST
//        // statements
//        break;
//      default:
//        break;
//    }
  }

  //  int tempHours = myRTC.hours;             //Get the hours and save to variable temp
  //  hourTens = tempHours / 10;               //Split hours into two digits, tens and units
  //  hourUnits = tempHours % 10;
  //  int tempMins = myRTC.minutes;               //Get the minutes and save to variable temp
  //  minuteTens = tempMins / 10;             //Split minutes into two digits, tens and units
  //  minuteUnits = tempMins % 10;

  //  if(minuteUnits != prevMinuteUnits)  //If minute units has changed, update display
  //    updateDisplay();
  //
  //  prevHourTens = hourTens;            //Update previous displayed numerals
  //  prevHourUnits = hourUnits;
  //  prevMinuteTens = minuteTens;
  //  prevMinuteUnits = minuteUnits;

  delay(500);
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  //Serial.print("Time Now : ");
  //Serial.println(datestring);
}

void packageDateTime(const RtcDateTime& dt)
{
  char datestring[15];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u%02u%04u%02u%02u%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
//  Serial.print("Payload : ");
//  Serial.print(datestring);
//  Serial.println(CmdCode[2]);
}
