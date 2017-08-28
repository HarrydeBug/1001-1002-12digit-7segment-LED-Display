//This is a I2C Master software that receives I2C data from an Optical encoder, and KeyBoard and sends data to a LED Display
//The Optical encoder acts as a I2C slave and waits for this program to query it.
//The same with the Keyboard.
//The LED dispplay is also a slave but Receives data insteads of sending it.
//This is usefull if you are out of pins on an arduino but have the use of I2C.
//To use this program you need run it on an Arduino like the UNO and then connect the following devices to it :
//Zachtek "Big Rotary Encoder" https://www.tindie.com/products/deBug67/big-rotary-encoder/
//Zachtek "12 Digit LED display" https://www.tindie.com/products/deBug67/12-digit-led-display/
//over I2C (SDA, SCL and GND should be conencted between the devices.)
//Built upon the example I2C code "Wire Master Reader" by Nicholas Zambetti <http://www.zambetti.com>
//I2c Addresses :
// 8=Rotary Encoder
// 9=Keyboard
//10=LED Display
//ZachTec 2016-2017

#include <Wire.h>
const char softwareversion[] = "1.01" ; //Version of this program, sent to serialport at startup

char RotaryStringIn[6]={'d','a','t','a','\0'}; //String to hold Rotary Encoder input
char KeyIn='\0';                               // Char to hold received Keyboard input
char LEDString[12]={'1','2','1','2','1','2','1','2','1','2','1','2'};//Char to hold LED digits to be displayed, used by the Keyboard portion
uint64_t LEDNumber=0; //Number displayed on LED as unsigned 64bit version as used by the rotary encoder portion.
byte InputState=0; //State 0=Rotary encoder updates LED display, 1=Keyboard inputs updates display

boolean RotaryFound, KeypadFound=false; //If Rotary Encoder, Keypad and LCD Display was detected on the I2C Bus
boolean LastRotaryFound, LastKeypadFound=false; //Last state of detection

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  ClearLEDString();
  Serial.print("Zachtek I2C Master Software, Version: ");
  Serial.println(softwareversion); 
  
  //Startup blink
  digitalWrite(13,HIGH); 
  delay(100);
  digitalWrite(13,LOW);
  delay(100);  
  digitalWrite(13,HIGH); 
  delay(100);
  digitalWrite(13,LOW);
}

  
void loop() {
int EncValue;  
int charcount;
unsigned long I2CTimeOut; 
boolean Notreceived;
  
  //Check Rotary Encoder
  charcount=0;
  Wire.requestFrom(8, 6);    // request 6 bytes from slave device #8 (Optical Encoder)
  I2CTimeOut=millis()+100;   //Timeout if not all 5 bytes are received in 100mS
  while (charcount<6 && millis()<I2CTimeOut ) {//untill all data is received or TimeOut is reached.
    while (Wire.available()) { // call may return with less than requested at so keep on filling buffer
      RotaryStringIn[charcount] = Wire.read(); // receive a byte 
      charcount++;
    }
  }
  if (millis ()>=I2CTimeOut) {
    RotaryFound=false;
  }
  else
    RotaryFound=true;
  { //We recevied data from I2C slave
    EncValue= atoi (RotaryStringIn); //Convert to Integer
    if (EncValue != 0) {//Rotary was moved
      if (EncValue>0) { //Rotary positive number
         functInc(abs(EncValue));//Increment LEDNumber
      }
      else  {  //Rotary negative number
        functDec(abs(EncValue)); //Decrement LEDNumber
      }
      if (InputState==0){//State machine is in update Rotary update state
        DisplayNumber(); //Update LED Display
        Serial.println(uint64ToStr (LEDNumber, false)); //Print the received data
      }
    }  
  }
    
   //Check Keyboard
   KeyIn='\0';
   Notreceived=true;
  Wire.requestFrom(9, 1);    // request 1 bytes from slave device #9 (Keyboard)
  I2CTimeOut=millis()+100;  //Timeout if not all bytes are received in 100mS
  while (Notreceived && millis()<I2CTimeOut ) {//until all data is received or TimeOut is reached.
    while (Wire.available()) { // call may return with less than requested so keep on filling buffer
      KeyIn = Wire.read(); // receive a byte as character  
      Notreceived=false;
    }
  }
  if (millis ()>=I2CTimeOut) 
  {
    KeypadFound=false;
  }
  else
    {//We recevied data from I2C slave
     KeypadFound=true;
    if (KeyIn != '\0') //If a key was pressed 
    {    
      if (InputState==0){//if State machine was in Rotary state then change it to keyboard now that we got input from it 
        InputState=1;
        ClearLEDString;
      }
      if (KeyIn=='E'){//Enter Key was pressed
        //All numbers in, update the display
        InputState=0;  //Go back to update the display from the rotary encoder now that Enter was pressed on the Keyboard
        LEDStringtoLEDnumber(); //Take the numbers from the Keyboard and store it 
        //Serial.println(uint64ToStr(LEDNumber,false)); //Print the result of the Keyboard input
        ClearLEDString();  //Clear the input buffer for Keyboard presses so it is blank at next use.
      }
      if (KeyIn=='<'){//Backspace Key was pressed
        RightShiftLEDString();  //delete the last digit pressed
        DisplayString();        //Update the LCD 
      }
      if (KeyIn>='0' && KeyIn <='9'){
        LeftShiftLEDString(); // make room for the last digit enter on the keyboard
        LEDString[11]=KeyIn-48;//Convert ASCII to numbers that suits the LCD display I2C protocol 
        DisplayString(); //Update the LCD 
      }
      Serial.println(KeyIn); //Print the received Keyboard data on the serial port
    }  
  }

  
  delay (50); //Query 20 times per second


  // If any of the I2C devices was plugged or unpluged then print status messages

  if (RotaryFound!=LastRotaryFound)
  {
    //if the Rotary Encoder  was pluged or unplugged 
    LastRotaryFound=RotaryFound;
    if (RotaryFound) 
    {
      Serial.println("Rotary Encoder connected on address 8"); 
    }
  }

  if (KeypadFound!=LastKeypadFound)
  { 
    LastKeypadFound=KeypadFound; 
    //if the Rotary Encoder  was pluged or unplugged 
    if (KeypadFound) 
    {
      Serial.println("Keyboard connected on address 9"); 
    }
  }
}

void functInc(int Amount) {
  //Serial.println (Amount);
  LEDNumber = LEDNumber + Amount;
  if (LEDNumber > 999999999999)//Bounds check
  {
    LEDNumber = 999999999999; 
  }
}

void functDec(int Amount) {
  uint64_t decNumber;
  decNumber = Amount;
  //Serial.println (Amount);
  if (decNumber > LEDNumber)
  {
    LEDNumber = 0; //Bounds check
  }
  else
  {
    LEDNumber = LEDNumber - decNumber;
  }
}


/*
void functGetNumber (CmdParser *myParser) {
  Serial.println(uint64ToStr(LEDNumber, false));
}
*/


uint64_t  StrTouint64_t (String InString)
{
  uint64_t y = 0;

  for (int i = 0; i < InString.length(); i++) {
    char c = InString.charAt(i);
    y *= 10;
    if (c >= '0' && c <= '9') {
      y += (c - '0');
    }  
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

void ClearLEDString (){
  for (byte i=0; i<12;i++ ) {
    LEDString[i]=16; //16=blank digit on LED display 
  }
}

void RightShiftLEDString (){
  for (int i=11; i>0 ;i--) {
    LEDString[i]=LEDString[i-1]; //16=blank digit on LED display 
  }
  LEDString[0]=16;//16=blank digit on LED display 
}

void LeftShiftLEDString (){
  for (int i=0; i<11; i++ ) {
    LEDString[i]=LEDString[i+1]; 
  }
  LEDString[11]=16;//16=blank digit on LED display 
}

String ASCIIToNumbers (String InString){//Convert a string with numbers to suit the LED display
char l_ResultBuffer [13]; //12 digits + null terminator char
int i=0;
boolean NotDigit=true;

  InString.toCharArray(l_ResultBuffer, 13);
  while (i<12) {
    if (l_ResultBuffer[i]=='0' && NotDigit) {
      l_ResultBuffer[i]=16;//blank digit on LED display if digit is zero and we havent seen another digit yet (Leding zeros blank) 
    }
    else 
    {
      NotDigit=false;
      l_ResultBuffer[i]=l_ResultBuffer[i]-48; //-48 converts ASCII number. (ASCII for char '1'=value 49) LED Display expect it as regular numbers 0-9
    }
    i++;
  }
  l_ResultBuffer[12]='\0';
  return String (l_ResultBuffer);
}


//Sends output to 12 Digit LED Display
void DisplayNumber (){
String DisplayString;
char l_ResultBuffer [13]; //12 digits + null terminator char
  DisplayString= ASCIIToNumbers(uint64ToStr (LEDNumber, true));
  DisplayString.toCharArray(l_ResultBuffer, 13);
  Wire.beginTransmission(10); // transmit to device #10
  Wire.write(l_ResultBuffer[11]);              // send 12 bytes
  Wire.write(l_ResultBuffer[10]);              
  Wire.write(l_ResultBuffer[9]);
  Wire.write(l_ResultBuffer[8]);
  Wire.write(l_ResultBuffer[7]);
  Wire.write(l_ResultBuffer[6]);
  Wire.write(l_ResultBuffer[5]); 
  Wire.write(l_ResultBuffer[4]);
  Wire.write(l_ResultBuffer[3]);
  Wire.write(l_ResultBuffer[2]);
  Wire.write(l_ResultBuffer[1]);
  Wire.write(l_ResultBuffer[0]);
  Wire.endTransmission();    // stop transmitting
}


//Sends output to 12 Digit LED Display
void DisplayString (){
  Wire.beginTransmission(10); // transmit to device #10
  Wire.write(LEDString[11]);              // send 12 bytes
  Wire.write(LEDString[10]); 
  Wire.write(LEDString[9]);
  Wire.write(LEDString[8]);
  Wire.write(LEDString[7]);
  Wire.write(LEDString[6]);
  Wire.write(LEDString[5]); 
  Wire.write(LEDString[4]);
  Wire.write(LEDString[3]);
  Wire.write(LEDString[2]);
  Wire.write(LEDString[1]);
  Wire.write(LEDString[0]);
  Wire.endTransmission();    // stop transmitting
}


//Sends output to 12 Digit LED Display
void  LEDStringtoLEDnumber (){
  char l_ResultBuffer[12]; //12 digits 
  int i=0;
  while (i<12) {
    if (LEDString[i]==16) {
      l_ResultBuffer[i]=' ';//blank digit=16 on LCD I2C protocol, replace with aspace in the output string
    }
    else 
    {
      l_ResultBuffer[i]=LEDString[i]+48; //Converts ASCII number. (ASCII for char '0' is 48 but in LED I2C protocoll it is 0
    }
    i++;
  }
  LEDNumber=StrTouint64_t(l_ResultBuffer);//Convert the string to a 64bit unsigned Integer and store in LEDNumber so the 
                                          //optical encoder can decrement or increment it
                                        
}
