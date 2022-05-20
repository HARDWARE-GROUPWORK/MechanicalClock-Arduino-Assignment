#include <ThreeWire.h>
#include <RtcDS1302.h>
#include "Adafruit_PWMServoDriver.h"      //Include library for servo driver

Adafruit_PWMServoDriver pwmH = Adafruit_PWMServoDriver(0x40);    //Create an object of Hour driver
Adafruit_PWMServoDriver pwmM = Adafruit_PWMServoDriver(0x41);    //Create an object of Minute driver (A0 Address Jumper)

int servoFrequency = 50;      //Set servo operating frequency

int segmentHOn[14] = {315, 320, 260, 310, 250, 280, 280, 330, 290, 320, 250, 330, 270, 300}; //On positions for each Hour servo
int segmentMOn[14] = {300, 245, 335, 260, 240, 240, 295, 260, 230, 250, 230, 330, 300, 265}; //On positions for each Minute servo
int segmentHOff[14] = {115, 120, 95, 100, 90, 100, 100, 120, 100, 120, 95, 130, 100, 110}; //Off positions for each Hour servo
int segmentMOff[14] = {100, 10, 120, 90, 90, 95, 100, 100, 85, 90, 90, 120, 110, 110}; //Off positions for each Minute servo
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
  pwmM.begin();
  pwmH.setOscillatorFrequency(27000000);    //Set the PWM oscillator frequency, used for fine calibration
  pwmM.setOscillatorFrequency(27000000);
  pwmH.setPWMFreq(servoFrequency);          //Set the servo operating frequency
  pwmM.setPWMFreq(servoFrequency);

  for (int i = 0 ; i <= 13 ; i++) //Set all of the servos to on or up (88:88 displayed)
  {
    pwmH.setPWM(i, 0, segmentHOn[i]);
    delay(10);
    pwmM.setPWM(i, 0, segmentMOn[i]);
    delay(10);
  }

  delay(1000);

  for (int i = 0 ; i <= 13 ; i++) //Set all of the servos to on or up (00:00 displayed)
  {
    pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(10);
    pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(10);
  }

  // DEBUG
  //  int target = 0;
  //  delay(1000);
  //  pwmH.setPWM(target, 0, segmentHOff[target]);
  //  delay(1000);
  //  pwmH.setPWM(target, 0, segmentHOn[target]);

  //Serial.begin(Baud Rate, Data Protocol, Rxd pin, Txd pin);
  FPGA.begin(9600, SERIAL_8N1, 16, 17);
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

  FPGA.write("ABCDEFGHIJKLMNSEG");

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
    //
    //    switch (currentCmd) {
    //      case CmdCode[2]: // SEG
    //        currentData = currentData.substring(10, 14);
    //        hourTens = currentData.substring(10, 11).toInt();
    //        hourUnits = currentData.substring(11, 12).toInt();
    //        minuteTens = currentData.substring(12, 13).toInt();
    //        minuteUnits = currentData.substring(13, 14).toInt();
    //        break;
    //      case CmdCode[1]: // SET
    //        // statements
    //        break;
    //      default:
    //        Serial.println("Invalid command by UART!");
    //        break;
  }

  //    if (minuteUnits != prevMinuteUnits) //If minute units has changed, update display
  //      updateDisplay();
  //
  //    prevHourTens = hourTens;            //Update previous displayed numerals
  //    prevHourUnits = hourUnits;
  //    prevMinuteTens = minuteTens;
  //    prevMinuteUnits = minuteUnits;
  //  }
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

void updateMid()                                              //Function to move the middle segements and adjacent ones out of the way
{
  if (digits[minuteUnits][6] != digits[prevMinuteUnits][6])   //Move adjacent segments for Minute units
  {
    if (digits[prevMinuteUnits][1] == 1)
      pwmM.setPWM(1, 0, segmentMOn[1] - midOffset);
    if (digits[prevMinuteUnits][6] == 1)
      pwmM.setPWM(5, 0, segmentMOn[5] + midOffset);
  }
  delay(100);                                                 //Delay allows adjacent segments to move before moving middle
  if (digits[minuteUnits][6] == 1)                            //Move Minute units middle segment if required
    pwmM.setPWM(6, 0, segmentMOn[6]);
  else
    pwmM.setPWM(6, 0, segmentMOff[6]);
  if (digits[minuteTens][6] != digits[prevMinuteTens][6])     //Move adjacent segments for Minute tens
  {
    if (digits[prevMinuteTens][1] == 1)
      pwmM.setPWM(8, 0, segmentMOn[8] - midOffset);
    if (digits[prevMinuteTens][6] == 1)
      pwmM.setPWM(12, 0, segmentMOn[12] + midOffset);
  }
  delay(100);                                                 //Delay allows adjacent segments to move before moving middle
  if (digits[minuteTens][6] == 1)                             //Move Minute tens middle segment if required
    pwmM.setPWM(13, 0, segmentMOn[13]);
  else
    pwmM.setPWM(13, 0, segmentMOff[13]);
  if (digits[hourUnits][6] != digits[prevHourUnits][6])       //Move adjacent segments for Hour units
  {
    if (digits[prevHourUnits][1] == 1)
      pwmH.setPWM(1, 0, segmentHOn[1] - midOffset);
    if (digits[prevHourUnits][6] == 1)
      pwmH.setPWM(5, 0, segmentHOn[5] + midOffset);
  }
  delay(100);                                                 //Delay allows adjacent segments to move before moving middle
  if (digits[hourUnits][6] == 1)                              //Move Hour units middle segment if required
    pwmH.setPWM(6, 0, segmentHOn[6]);
  else
    pwmH.setPWM(6, 0, segmentHOff[6]);
  if (digits[hourTens][6] != digits[prevHourTens][6])         //Move adjacent segments for Hour tens
  {
    if (digits[prevHourTens][1] == 1)
      pwmH.setPWM(8, 0, segmentHOn[8] - midOffset);
    if (digits[prevHourTens][6] == 1)
      pwmH.setPWM(12, 0, segmentHOn[12] + midOffset);
  }
  delay(100);                                                 //Delay allows adjacent segments to move before moving middle
  if (digits[hourTens][6] == 1)                               //Move Hour tens middle segment if required
    pwmH.setPWM(13, 0, segmentHOn[13]);
  else
    pwmH.setPWM(13, 0, segmentHOff[13]);
}

void updateDisplay ()                               //Function to update the displayed time
{
  updateMid();                                      //Move the segments out of the way of the middle segment and then move the middle segments
  for (int i = 0 ; i <= 5 ; i++)                    //Move the remaining segments
  {
    if (digits[hourTens][i] == 1)                   //Update the hour tens
      pwmH.setPWM(i + 7, 0, segmentHOn[i + 7]);
    else
      pwmH.setPWM(i + 7, 0, segmentHOff[i + 7]);
    delay(10);
    if (digits[hourUnits][i] == 1)                  //Update the hour units
      pwmH.setPWM(i, 0, segmentHOn[i]);
    else
      pwmH.setPWM(i, 0, segmentHOff[i]);
    delay(10);
    if (digits[minuteTens][i] == 1)                 //Update the minute tens
      pwmM.setPWM(i + 7, 0, segmentMOn[i + 7]);
    else
      pwmM.setPWM(i + 7, 0, segmentMOff[i + 7]);
    delay(10);
    if (digits[minuteUnits][i] == 1)                //Update the minute units
      pwmM.setPWM(i, 0, segmentMOn[i]);
    else
      pwmM.setPWM(i, 0, segmentMOff[i]);
    delay(10);
  }
}
