// MTE 201 Project
// Program for a distance measurement device using a potentiomenter (linear)

#include <LiquidCrystal.h>
#include <EEPROM.h>
typedef unsigned char byte;

// Defines for EEPROM locations
#define SLOPE 0
#define OFFSET 4
#define ERR 8
#define BTNTIME 1500
const uint8_t BUTTONLIST[] = {6, 7, 8};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup()
{
  float f = 0.00f;
  lcd.begin(16, 2);

  // Setup button pins and measurement pins
  // Pin A0 is for the measurement probe
  for (int i = 0; i < sizeof(BUTTONLIST)/sizeof(uint8_t); i++)
  {
    pinMode(i, INPUT);
  }
  
  // If the slope is zero, the device is not calibrated [properly]
  if (abs(EEPROM.get(SLOPE, f)) < 0.00001)
  {
    // Clear EEPROM
    for (int i = 0; i < sizeof(float)*3; i += sizeof(float))
    {
      EEPROM.put(i, 0.00f);
    }
    do
    {
      lcd.write("Calibration req.");
      lcd.setCursor(0, 1);
      lcd.write("Push btn to cont");
      handleButtons();
    } while(!calibrate());
  }
}
// Handles button input
// Returns negative if button is hed for certain amount of time and onlift is true
int handleButtons()
{
  bool x = true;
  uint8_t i = 0;
  unsigned long buttonHeld = 0;
  while(x)
  {
    for (i = 0; i < sizeof(BUTTONLIST)/sizeof(uint8_t); i++)
    {
      if (digitalRead(BUTTONLIST[i]))
      {
        x = false;
        break;
      }
    }
  }
  
  buttonHeld = millis();
  while(digitalRead(BUTTONLIST[i]));
  if (millis() - buttonHeld >= BTNTIME)
  {
    return -i;
  }
  return i;
}

bool calibrate()
{
  return 0;
}

// Datapoints, stepSize, y is the point where the step starts
float findSlope(uint8_t numberOfPoints, uint16_t *datapoints, uint8_t stepSize, uint8_t stepStart)
{
  unsigned long x = 0, y = 0, xy = 0, xx = 0;
  int i;

  for (i = 0; i < numberOfPoints; i++)
  {
    x += datapoints[i];
    y += i * stepSize + stepStart;
    xy += datapoints[i]*(i * stepSize + stepStart);
    xx += datapoints[i]*datapoints[i];
  }

  return ((float)numberOfPoints*xy - x*y)/(numberOfPoints*xx - x*x);
}

void loop()
{
  
}
