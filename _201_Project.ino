// MTE 201 Project
// Program for a distance measurement device using a potentiomenter (linear)

#include <LiquidCrystal.h>
#include <EEPROM.h>

// Defines for EEPROM locations
#define SLOPE 0
#define OFFSET 4
#define ERR 8
#define BTNTIME 1500

// 6 is up, 7 is down, 8 is select
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
  
  EEPROM.get(SLOPE, f);
  // If the slope is zero, the device is not calibrated [properly]
  if (abs(f) < 0.00001)
  {
    // Clear EEPROM
    for (int i = 0; i < sizeof(float)*3; i += sizeof(float))
    {
      EEPROM.put(i, 0.00f);
    }
    do
    {
      lcd.setCursor(0, 0);
      lcd.print("Calibration req.");
      lcd.setCursor(0, 1);
      lcd.print("Push btn to cont");
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

uint16_t uintCollection(String upperTitle, String lowerTitle, uint8_t maxDigits, uint16_t defaultNum)
{
  // Because of display (16 digits) and uint16_t (max num is 32767) restrictions
  if (maxDigits > 4)
    maxDigits = 4;
  if (maxDigits < 1)
    maxDigits = 1;
  if (upperTitle.length() > 16 - maxDigits)
    upperTitle = upperTitle.substring(0, 16 - maxDigits);
  if (lowerTitle.length() > 16 - maxDigits)
    lowerTitle = lowerTitle.substring(0, 16 - maxDigits);

  lcd.setCursor(0, 0);
  lcd.print(upperTitle);
  lcd.setCursor(0, 1);
  lcd.print(lowerTitle);
  lcd.setCursor(16 - maxDigits, 0);
  lcd.print("v");
  lcd.setCursor(16 - maxDigits, 1);
  uint8_t selectedNum [4];
  for (int i = 0; i < 4; i++)
  {
    selectedNum[i] = defaultNum/pow(10, i);
    lcd.print(defaultNum/pow(10, i));
  }

  uint8_t cursorPosition = maxDigits;
  while (cursorPosition && cursorPosition <= maxDigits)
  {
    lcd.setCursor(16 - cursorPosition, 0);
    lcd.print("v");
    lcd.setCursor(16 - cursorPosition, 1);
    lcd.print(selectedNum[cursorPosition]);
    int action = handleButtons();
    if (abs(action) == 6)
    {
      selectedNum[cursorPosition] = (selectedNum[cursorPosition] + 1) % 10;
    } else if (abs(action) == 7)
    {
      selectedNum[cursorPosition] = (selectedNum[cursorPosition] - 1) % 10;
    } else if (action == 8)
    {
      cursorPosition -= 1;
    } else if (action == -8)
    {
      cursorPosition = cursorPosition % 4 + 1;
    }
  }

  uint16_t finalNumber = 0;
  for (int i = 0; i < 4; i++)
  {
    finalNumber += selectedNum[i]*pow(10, i);
  }
  return finalNumber;
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
