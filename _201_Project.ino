#include <LiquidCrystal.h>
#include <EEPROM.h>

// Defines for EEPROM locations
#define slope 0
#define offset 4

LiquidCrystal display(12, 11, 5, 4, 3, 2);

void setup()
{
  lcd.begin(16, 2);
}

// Datapoints, stepSize, y is the point where the step starts
double findSlope(uint16_t *datapoints, uint8_t stepSize, uint8_t stepStart)
{
  // if smaller than 2 exit
  if (!(steps & 0xfe))
  {
    return 0;
  }
  unsigned long x = 0, y = 0, xy = 0, xx = 0;
  int i;

  for (i = 0; i < datapoints.length(); i++)
  {
    x += datapoints[i];
    y += i * stepSize + stepStart;
    xy += datapoints[i]*(i * stepSize + stepStart);
    xx += datapoints[i]*datapoints[i];
  }

  return ((float)datapoints.length()*xy - x*y)/(datapoints.length()*xx - x*x);
}

void loop()
{
  
}
