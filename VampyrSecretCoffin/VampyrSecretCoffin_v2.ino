// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Adafruit_VS1053.h>


#define INCLUDE_MUSIC 1

const byte entryReedPin = 2; // Pin connected to reed switch 1
const byte exitReedPin = 10; // Pin connected to reed switch 2
const byte entryLockPin = 8; // Pin connected to mag lock 1
const byte exitLockPin = 9; // Pin connected to mag lock 2
const byte exitDebugLightPin = 7;
const byte entryDebugLightPin = 6;
const byte pressureMatPin = 5; // Pin connected to pressure mat
const int musicDelay = 1000;
const int holdDoorDelay = 2000;

#define SHIELD_RESET  -1     // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)#define   PrgButton  6   //Program button

#define I2C_ADDRESS 0x3C
SSD1306AsciiAvrI2c oled;

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin
Adafruit_VS1053_FilePlayer musicPlayer = 
  // create shield-example object!
Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

bool hasMusicPlayer = false;


const byte lock = LOW;
const byte unlock = HIGH;

//just so we can print state changes out without constant serial spam
bool prevEntryOpen;
bool prevExitOpen;
bool prevPressureOnMat;

//we dont want to constantly do the exit door lock/delay/music stuff, so wait for things to cycle before allowing it again
bool transitionAllowed = true;


bool readyForMusic = true;

//0 open entry door
//1 step on pressure mat and close entry door
//2 play music and delay
//3 open exit door and step off pressure mat
//4 close exit door
//repeat

int currentStep = 0;

String Line1 = "";
String Line2 = "";
String Line3 = "";
String Line4 = "";

void printDebug(String line, int id)
{
  if(id == 1)
    Line1 = line;
  else if(id == 2)
    Line2 = line;
  else if(id == 3)
    Line3 = line;
  else if(id == 4)
    Line4 = line;

  oled.clear();
  oled.println(Line1);
  oled.println(Line2);
  oled.println(Line3);
  oled.println(Line4);
  Serial.println(String(id) + ": " + line);
}

void setExitLock(bool unlocked)
{
  digitalWrite(exitLockPin, unlocked);
  digitalWrite(exitDebugLightPin, !unlocked);
  if(!unlocked)
    printDebug(F("Exit Locked"), 4);
  else
    printDebug(F("Exit Unlocked"), 4);
}

void setEntryLock(bool unlocked)
{
  digitalWrite(entryLockPin, unlocked);
  digitalWrite(entryDebugLightPin, !unlocked);
  if(!unlocked)
    printDebug(F("Entry Locked"), 3);
  else
    printDebug(F("Entry Unlocked"), 3);
}

void setup() {
  Serial.begin(9600);

  //initialize our debug display
  oled.begin(&Adafruit128x32, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  
  printDebug(F("Initializing"), 1);
  hasMusicPlayer = true;
  if (! musicPlayer.begin()) { // initialise the music player
     printDebug(F("Couldn't find VS1053 music"), 2);
     hasMusicPlayer = false;
  }
  else
    printDebug(F("VS1053 music found"), 2);

  if(hasMusicPlayer)
  {
    if (!SD.begin(CARDCS)) 
    {
      printDebug(F("SD failed, or not present"), 2);
      hasMusicPlayer = false;
    }
  }

  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(0,0);

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Play one file, don't return until complete
  printDebug(F("Playing track 001"), 2);
  musicPlayer.playFullFile("track001.mp3");

  //set pinmodes - our inputs run to ground, so need to flip them around with input_pullup
  pinMode(entryReedPin, INPUT_PULLUP); //low means door open, high means closed
  pinMode(exitReedPin, INPUT_PULLUP);
  pinMode(pressureMatPin, INPUT_PULLUP);//low means pressure on the mat
  pinMode(entryLockPin, OUTPUT);
  pinMode(exitLockPin, OUTPUT);
  pinMode(entryDebugLightPin, OUTPUT);
  pinMode(exitDebugLightPin, OUTPUT);
  setEntryLock(unlock);
  setExitLock(lock);
  printDebug(F("Step 0"), 1);
}

void nextStep()
{
  currentStep++;
  if(currentStep >= 5)
    currentStep = 0;
  printDebug("Step " + String(currentStep), 1);
}

int aliveCounter;
void loop() 
{
  aliveCounter++;
  if(aliveCounter > 10)
  {
    aliveCounter = 0;
    Serial.println(F("alive"));
  }
  
  byte entryReedState = digitalRead(entryReedPin);
  byte exitReedState = digitalRead(exitReedPin);
  byte pressureState = digitalRead(pressureMatPin);

  bool entryOpen = entryReedState == LOW;
  bool exitOpen = exitReedState == LOW;
  bool pressureOnMat = pressureState == LOW;

//0 open entry door
//1 step on pressure mat and close entry door
//2 play music and delay
//3 open exit door and step off pressure mat
//4 close exit door
if(currentStep == 0)
{
  if(entryOpen)
  {
    //printDebug(F("Go to Step 1"));
    nextStep();
  }
}
else if(currentStep == 1)
{
  if(pressureOnMat && !entryOpen && !exitOpen)
  {
    delay(holdDoorDelay);
    entryReedState = digitalRead(entryReedPin);
    exitReedState = digitalRead(exitReedPin);
    pressureState = digitalRead(pressureMatPin);
    entryOpen = entryReedState == LOW;
    exitOpen = exitReedState == LOW;
    pressureOnMat = pressureState == LOW;
    if(pressureOnMat && !entryOpen && !exitOpen)
    {
      printDebug(F("Lock Entry"), 2);
      setEntryLock(lock);
      nextStep();
    }
  }
}
else if(currentStep == 2)
{
  if(hasMusicPlayer)
  {
    printDebug(F("play music"), 2);
    musicPlayer.playFullFile("track001.mp3");
  }

  printDebug(F("Delaying..."), 2);
  delay(musicDelay);
  printDebug(F("Unlock Exit"), 2);
  setExitLock(unlock);
  nextStep();
}
else if(currentStep == 3)
{
  if(!pressureOnMat && exitOpen)
  {
    nextStep();
  }
}
else if(currentStep == 4)
{
  if(!exitOpen)
  {
    delay(holdDoorDelay);
    exitReedState = digitalRead(exitReedPin);
    exitOpen = exitReedState == LOW;
    if(!exitOpen) {      
      setExitLock(lock);
      printDebug(F("Delaying..."), 2);
      delay(musicDelay);
      printDebug(F("Unlock Entry"), 2);
      setEntryLock(unlock); //wait a moment to make sure the exit latched
      nextStep();
    }
  }
}
  //cache our states so we can check if things flipped next time
  prevEntryOpen = entryOpen;
  prevExitOpen = exitOpen;
  prevPressureOnMat = pressureOnMat;
  delay(100);
}
