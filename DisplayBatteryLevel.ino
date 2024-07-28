/* Display Battery Level
 * 
 */

#include "TM1651.h"
#define CLK 11//pins definitions for TM1651 and can be changed to other ports       
#define DIO 10
TM1651 batteryDisplay(CLK,DIO);

//======Advisor======
bool bDsplyOn;

volatile bool newPress = 0, 
  validPress,
  bDsplyState;

//======Organizer======
int btnBounceTime;

long vMax = 7500, // [mV]
  vMin, // [mV]
  blinkV;

unsigned long prevSenseTime,
  prevPressTime;

//======Interface======
const byte numBLevels = 5, // no. discrete batt levels
  batPin = 15,
  btnPin = 21;

byte bLevel; // [0-5]

long vInput;

void setup() 
{
  Serial.begin(115200);

  pinMode(btnPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPin), btnPressed, FALLING);

  btnBounceTime = 250; // [ms]
  
  batteryDisplay.init();
  batteryDisplay.set(BRIGHTEST);
  
  //levelStep = ( vMax - vMax * 5 / 6 ) / numBLevels; // 5/6 is reference ratio of min V

  vMin = (long) round(5 * vMax / 6.0);
  Serial.print("vMin (mV): ");
  Serial.println(vMin);

  blinkV = (long) round(51 / 60.0 * vMax);
  Serial.print("blinkV (mV): ");
  Serial.println(blinkV);
  
  //vInput = 6370; // analogRead
}

void loop() 
{
  unsigned long curTime = millis();
  
  if(curTime - prevPressTime > btnBounceTime) // if the noise is gone
  {
    noInterrupts();
      
    validPress = true;

    if(newPress)
    {
      prevPressTime = curTime; // record the time of the first pulse in bounce group
      
      newPress = false;
    }

    interrupts();
  }
  
  if(bDsplyState)
  {
    readBatV(); // read battery voltage
    
    if(vInput < blinkV)
    {
      if(millis() - prevSenseTime > 100)
      {
        blink();
  
        prevSenseTime = millis();
      }
    }
    else
    {
      bLevel = (byte) round(map(vInput, vMin, vMax, 0, numBLevels * 100) / 100.0);
      Serial.print("bLevel (0-5): ");
      Serial.println(bLevel);
  
      batteryDisplay.displayLevel(bLevel);
      batteryDisplay.frame(FRAME_ON);
    }
  }
  else
  {
    batteryDisplay.displayLevel(0);
    batteryDisplay.frame(FRAME_OFF);
  }
}

void blink()
{
  Serial.println("======Blink======");
  bDsplyOn = !bDsplyOn;
  Serial.print("bDsplyOn: ");
  Serial.println(bDsplyOn);
  
  if(bDsplyOn)
  {
    batteryDisplay.displayLevel(0);
    batteryDisplay.frame(FRAME_OFF);
  }
  else
  {
    batteryDisplay.displayLevel(1);
    batteryDisplay.frame(FRAME_ON);
  }
}

void readBatV()
{
  const byte numSamples = 10; //number of analog readings taken to determine more accurate supply voltage
  
  int analogVal, //supply voltage converted to analog value by Arduino
    sum; //sum of sampled voltage readings
    
  float divFactor = 1.97; 
    
  for (int i=0; i < numSamples; i++) //take 10 analog samples:
  {
    analogVal = analogRead(batPin); //read value on analog pin
    
    sum += analogVal; //add values until all samples taken (sum averaged later)
    
    delayMicroseconds(3); //avoid potentially jumbled readings
  }
  
  vInput = (long) round( 625 * divFactor * sum / ( 128 * numSamples ) ); // [mV], 5 * divFactor * sum * 1000 / ( 1024 * numSamples )
  Serial.print("vInput (mV): ");
  Serial.println(vInput);

  vInput = clip(vInput, vMax, vMin);
  
  sum = 0;
}

int clip(int a, int maximum, int minimum)
{
  //Serial.print("Computed val: ");
  //Serial.print(a);
    
  if(a > maximum) 
    a = maximum;
  else if(a < minimum) 
    a = minimum;

  //Serial.print(", Clipped val: ");
  //Serial.println(a);

  return a;
}

//Interrupt service routine
void btnPressed()
{
  if(validPress)
  {
    bDsplyState = !bDsplyState;

    validPress = 0;
  }

  newPress = 1;
}

