
/* Software for Zachtek "12 Digit LED Display R1" 
   
   Arduino software that takes incoming data on serial port and displays it on LED display.
   Filename is "SerialDisplay"
   The Version of this software is stored in the constant "softwareversion" and is displayed on the Serialport att startup
   For Arduino Pro Mini.

   Arduino Output pin 2-9 drives 7 segment LEDs.
   All 7 segment digits are coupled in paralell. 7 segment is common cathode.
   output 10,11,12 are counter output to a 74HC238 3 to 8 demultiplexer that sinks the common cathodes one at a time via a FET transistor for each 7 segment digit.
   output 13 is enable to the 74HC238, it needs to be pulsed every once in while to keep the LED lighted. This a safeguard to make sure a LED is not
   getting overcurrent in case of stuck software or unprogramed Arduinos pulling or sinking random outputs
*/

#include <Wire.h>
#include <TimerOne.h>
#include <CmdBuffer.hpp>
#include <CmdCallback.hpp>
#include <CmdParser.hpp>

CmdCallback<8> cmdCallback; // Number of commands listed below

char strSetNumber[] ="SETNUMBER";
char strInc[]       ="INC";
char strDec[]       ="DEC";
char strGetNumber[] ="GETNUMBER";
char strTest[]      ="TEST";
char strZeros[]     ="ZEROS";
char strDisplay[]   ="DISPLAY";
char strHelp[]      ="HELP";
const uint8_t Font[17] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111, 119, 124, 57, 94, 123, 113,0}; //LED fonts, first=0, second=1 etc. sixteen is=F, last is blank digit


const char softwareversion[] = "1.25" ; //Version of this program, sent to serialport at startup

uint8_t LEDDigittoUpdate = 1; //Scan the digits and update one digit every timerupdate, rightmost digit is 1
uint8_t LEDdigit[12]; // array of 12 digits that are displayed.
uint64_t LEDNumber; //The number that is displayed in unsigned 64bit version so we can do math on it.

boolean NoLeadingZeros = true; //Dont Display leading zeros on the LEDs
boolean LEDDisplayON = true; //Display is turned on


void setup()
{
  // set the digital pin as output:
  pinMode(2, OUTPUT) ; pinMode(3, OUTPUT) ; pinMode(4, OUTPUT); pinMode(5, OUTPUT) ; pinMode(6, OUTPUT) ; pinMode(7, OUTPUT);
  pinMode(8, OUTPUT); pinMode(9, OUTPUT); pinMode(10, OUTPUT); pinMode(11, OUTPUT); pinMode(12, OUTPUT); pinMode(13, OUTPUT); pinMode(A0, OUTPUT);
  digitalWrite(10, HIGH); digitalWrite(11, HIGH); digitalWrite(12, HIGH); digitalWrite(13, HIGH); // Do not sink any Cathodes while we initialize (set 74HC238 output to 15 that dont have a LED digit) )
  
  Serial.begin(9600);

  cmdCallback.addCmd(strSetNumber, &functSetNumber);
  cmdCallback.addCmd(strInc, &functInc);
  cmdCallback.addCmd(strDec, &functDec);
  cmdCallback.addCmd(strGetNumber, &functGetNumber);
  cmdCallback.addCmd(strTest, &functTest);
  cmdCallback.addCmd(strZeros, &functZeros);
  cmdCallback.addCmd(strDisplay, &functDispONOFF);
  cmdCallback.addCmd(strHelp, &functHelp);

  Wire.begin(10);                  // join i2c bus with address #10 as a I2C Slave unit
  Wire.onReceive(I2CreceiveEvent); // register event so we can receive data from I2C bus

  InitTimer (1000);   //turn on LED scanning
  delay (1000);//
  Serial.print("Zachtek 12 Digit LED Display Software, Version: ");
  Serial.println(softwareversion);
  Serial.println("Type Help and press enter for command information");
  Serial.println("");
}

void loop()  //Main loop
{
  CmdBuffer<26> myBuffer;
  CmdParser     myParser;

  // Automatic handling of incoming comands on the serial port.
  cmdCallback.loopCmdProcessing(&myParser, &myBuffer, &Serial);
}

void functSetNumber(CmdParser *myParser) {
  LEDNumber = StrTouint64_t(myParser->getCmdParam(1));
  DisplayNumber();
}

void functInc(CmdParser *myParser) {
  LEDNumber = LEDNumber + StrTouint64_t(myParser->getCmdParam(1));
  if (LEDNumber > 999999999999)
  {
    LEDNumber = 999999999999; //Bounds check
  }
  DisplayNumber();
  GroundCathode (StrTouint64_t(myParser->getCmdParam(1)));
}

void functDec(CmdParser *myParser) {
  uint64_t decNumber;
  decNumber = StrTouint64_t(myParser->getCmdParam(1));
  if (decNumber > LEDNumber)
  {
    LEDNumber = 0; //Bounds check
  }
  else
  {
    LEDNumber = LEDNumber - decNumber;
  }
  DisplayNumber();
}



void functGetNumber (CmdParser *myParser) {
  Serial.println(uint64ToStr(LEDNumber, false));
}


void functTest(CmdParser *myParser) {
  int64_t Counter ;
  int64_t CounterIncrement ;

  Serial.println("Running LED Tests");

  for (Counter = 0; Counter < 2; Counter++) {
    //Do a check of the enable cicuit, one digit should flash briefely before changing postion to next digit. If not lit or lit constantly then the enable is not working.
    //Test pattern is AbCdAbCd
    InitTimer (800000);  //extremely  Slow scanning to test automatic turn off circuit
    LEDdigit[0] = 15, LEDdigit[1] = 14, LEDdigit[2] = 13, LEDdigit[3] = 12, LEDdigit[4] = 11, LEDdigit[5] = 10;
    LEDdigit[6] = 15, LEDdigit[7] = 14, LEDdigit[8] = 13, LEDdigit[9] = 12, LEDdigit[10] = 11, LEDdigit[11] = 10;

    delay (6000);//

    LEDNumber = 888888888888; //Light upp all segments in all digits
    DisplayNumber();
    InitTimer (30000);  //Slow scanning so we can see any stuck-ON segments.
    delay (6000);//

    InitTimer (1000); //Normal scanning so we can se any broken segments
    NoLeadingZeros = false; //display leading zeros

    LEDNumber = 000000000000;
    DisplayNumber();
    delay (200);
    LEDNumber = 111111111111;
    DisplayNumber();
    delay (200);
    LEDNumber = 222222222222;
    DisplayNumber();
    delay (200);
    LEDNumber = 333333333333;
    DisplayNumber();
    delay (200);
    LEDNumber = 444444444444;
    DisplayNumber();
    delay (200);
    LEDNumber = 555555555555;
    DisplayNumber();
    delay (200);
    LEDNumber = 666666666666;
    DisplayNumber();
    delay (200);
    LEDNumber = 777777777777;
    DisplayNumber();
    delay (200);
    LEDNumber = 888888888888;
    DisplayNumber();
    delay (200);
    LEDNumber = 999999999999;
    DisplayNumber();
    delay (200);
  }

  NoLeadingZeros = true; //dont display leading zeros
  //Count up faster and faster up to one trillion
  CounterIncrement = 1;
  for (Counter = 1; Counter < 1000000000000; Counter = Counter + CounterIncrement)
  {
    LEDNumber = Counter;
    DisplayNumber();
    delay(10);
    CounterIncrement = CounterIncrement + (CounterIncrement / 100) + 1;
  }
  delay (1000);
  LEDNumber = 0;
  DisplayNumber();
  Serial.println("LED Test completed");
}


void functZeros(CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "ON")) {
    Serial.println(myParser->getCmdParam(1));
    NoLeadingZeros = false;
    DisplayNumber();
  }
  if (myParser->equalCmdParam(1, "OFF")) {
    Serial.println(myParser->getCmdParam(1));
    NoLeadingZeros = true;
    DisplayNumber();
  }

}


void functDispONOFF (CmdParser *myParser) {
  if (myParser->equalCmdParam(1, "ON")) {
    LEDDisplayON = true;
  }

  if (myParser->equalCmdParam(1, "OFF")) {
    LEDOutputdigit (0);//Turn Off all segments
    LEDDisplayON = false; //Led scanner routine will do nothing
  }
}


void functHelp (CmdParser *myparser) {

  Serial.println("Type one of the following commands to affect the display: ");
  Serial.println(" : SetNumber ...  , sets a new number on the LED display");
  Serial.println(" : Inc ...  , Increment the number in the display.  e.g. 'Inc 15' will increment the existing number with 15");
  Serial.println(" : Dec ...  , decrements the number on the display");
  Serial.println(" : GetNumber  ,sends the LED number to the serial port");
  Serial.println(" : Test   , runs a testrutine that can be used to find hardware faults");
  Serial.println(" : Zeros ON/OFF  ,turns off or on leading zeros on the LED display");
  Serial.println(" : Display ON/OFF  ,blanks the display");
  Serial.println(" : Help  ,prints this information");
  Serial.println("");

}




void DisplayNumber () //Calculate each digit for the LED display and save the result. The Timer routine will diplay them for us.
{
  uint8_t DigitCount;
  int64_t Number;
  uint8_t TempLEDdigit[12]; //Work on a copy of the digit array as the timer event can ocur mid in to this routine and we want
  //all digits to be correct before we use them
  Number = LEDNumber;       // Work on a copy of the 64bit version so we can retain the orginal value

  TempLEDdigit[11] = Number /  100000000000;
  Number = Number - (int64_t (TempLEDdigit[11]) * 100000000000);

  TempLEDdigit[10] = Number /  10000000000;
  Number = Number -  (int64_t (TempLEDdigit[10]) * 10000000000);

  TempLEDdigit[9] = Number /  1000000000;
  Number = Number -  (int64_t (TempLEDdigit[9]) * 1000000000);

  TempLEDdigit[8] = Number /  100000000;
  Number = Number -  (int64_t (TempLEDdigit[8]) * 100000000);

  TempLEDdigit[7] = Number /  10000000;
  Number = Number -  (int64_t (TempLEDdigit[7]) * 10000000);

  TempLEDdigit[6] = Number /  1000000;
  Number = Number -  (int64_t (TempLEDdigit[6]) * 1000000);

  TempLEDdigit[5] = Number /  100000;
  Number = Number - (int64_t (TempLEDdigit[5]) * 100000);

  TempLEDdigit[4] = Number /  10000;
  Number = Number -  (int64_t (TempLEDdigit[4]) * 10000);

  TempLEDdigit[3] = Number /  1000;
  Number = Number -  (int64_t (TempLEDdigit[3]) * 1000);

  TempLEDdigit[2] = Number /  100;
  Number = Number -  (int64_t (TempLEDdigit[2]) * 100);

  TempLEDdigit[1] = Number /  10;
  Number = Number -  (int64_t (TempLEDdigit[1]) * 10);

  TempLEDdigit[0] = Number;

  if (NoLeadingZeros)
  {
    for (DigitCount = 11; DigitCount > 0; DigitCount-- )
    {
      if (TempLEDdigit[DigitCount] == 0)
      {
        TempLEDdigit[DigitCount] = 16; // 16=blank digit
      }
      else
      {
        break; //We have found all the leading Zeros, exit loop
      }
    }
  }
  for (DigitCount = 0; DigitCount < 12; DigitCount++ )
  {
    LEDdigit[DigitCount] = TempLEDdigit[DigitCount];
  }
}


void GroundCathode (uint8_t p_Count) //Set port 10 to 13 to ground the correct Cathode with external 74HC238 3-to-8 demultiplexer IC
{
  uint8_t l_Loop;

  for (l_Loop = 0 ; l_Loop < 4 ; l_Loop++) {
    if ((p_Count & (1 << l_Loop)) == 0) {
      digitalWrite(10 + l_Loop, LOW);
    }
    else {
      digitalWrite(10 + l_Loop, HIGH);
    }
  }
}


//Turn on the correct 7 Segments to form a digit (Font routine) output is port 2 to 9
void LEDOutputdigit (uint8_t p_Digit)
{
  uint8_t l_Loop;

  for (l_Loop = 0 ; l_Loop < 8  ; l_Loop++) {//check each bit, start at bit 0
    if ((Font[p_Digit] & (1 << l_Loop)) == 0) {//If bit not set then 
      digitalWrite(2 + l_Loop, LOW);           //turn off the LED segment
    }
    else {                                     //If bit is set then 
      digitalWrite(2 + l_Loop, HIGH);          //light up the LED segment
    }
  }
}


void InitTimer ( uint32_t uDelay)
{
  Timer1.initialize(uDelay);
  Timer1.attachInterrupt(LEDScanner); // Update the LED display to run on Timer
}


void LEDScanner(void) { //Turn on one LED digit in the display at each call.
  //This will be called from Timer routine and is performed so fast that the eye will see all displays lit up simultaneously.
  if (LEDDisplayON) {
    digitalWrite(A0, LOW); //Puls the enable on the driver cicuits
    LEDOutputdigit (16);//Turn Off all segments before we change the grounding of chatodes to avoid ghost digits.
    GroundCathode (LEDDigittoUpdate - 1); //Ground the correct Cathode
    LEDOutputdigit (LEDdigit[LEDDigittoUpdate - 1]); //Turn on the segements
    LEDDigittoUpdate++; //Next time this timer is triggered - lit up the next display digit
    if (LEDDigittoUpdate > 12) LEDDigittoUpdate = 1; //If last digit is updated then start over at number one
    digitalWrite(A0, HIGH); //Puls the enable
  }
}


uint64_t  StrTouint64_t (String InString)
{
  uint64_t y = 0;

  for (int i = 0; i < InString.length(); i++) {
    char c = InString.charAt(i);
    if (c < '0' || c > '9') break;
    y *= 10;
    y += (c - '0');
  }
  return y;
}

String  uint64ToStr (uint64_t p_InNumber, boolean p_LeadingZeros)
{
  char l_HighBuffer[7]; //6 digits + null terminator char
  char l_LowBuffer[7]; //6 digits + null terminator char
  char l_ResultBuffer [13]; //12 digits + null terminator char
  String l_ResultString = "";
  uint8_t l_Digit;

  sprintf(l_HighBuffer, "%06lu", p_InNumber / 1000000L); //Convert high part of 64bit unsigned integer to char array
  sprintf(l_LowBuffer, "%06lu", p_InNumber % 1000000L); //Convert low part of 64bit unsigned integer to char array
  l_ResultString = l_HighBuffer;
  l_ResultString = l_ResultString + l_LowBuffer; //Copy the 2 part result to a string

  if (!p_LeadingZeros) //If leading zeros should be romeved
  {
    l_ResultString.toCharArray(l_ResultBuffer, 13);
    for (l_Digit = 0; l_Digit < 12; l_Digit++ )
    {
      if (l_ResultBuffer[l_Digit] == '0')
      {
        l_ResultBuffer[l_Digit] = ' '; // replace zero with a space character
      }
      else
      {
        break; //We have found all the leading Zeros, exit loop
      }
    }
    l_ResultString = l_ResultBuffer;
    l_ResultString.trim();//Remove all leading spaces
  }
  return l_ResultString;
}


// function that executes whenever data is received from I2C master
// this function is registered as an event, see setup()
void I2CreceiveEvent(int howMany) {
byte count;
  if (howMany>12) howMany=12;
   for (count=0; count < (howMany); count++){ 
    LEDdigit[count] = Wire.read();      
  }
}
