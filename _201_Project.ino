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
  lcd.clear();
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
  lcd.clear();
  uint16_t startPos = uintCollection("Start dist", "(mm)", 3, 5),
           stepping = uintCollection("Step size", "(mm)", 2, 5),
           currentPos = startPos;
  uint8_t numberOfPoints = 0, input;
  uint16_t points [255];
  
  if (!stepping)
  {
    lcd.print("Calibration Fail");
    lcd.setCursor(0, 1);
    lcd.print("Step must be > 0");
    handleButtons();
    return 0;
  }
  
  lcd.print("Put probe @ ");
  lcd.print(currentPos);
  lcd.setCursor(0, 1);
  lcd.print("and press btn.");
  while (input = handleButtons() != -8)
  {
    points[numberOfPoints] = analogRead(0);
    ++numberOfPoints;
    currentPos = startPos + stepping*numberOfPoints;
    lcd.setCursor(12, 0);
    lcd.print(currentPos);
  }

  lcd.clear();
  if (!(numberOfPoints & 0xfe))
  {
    lcd.print("Calibration Fail");
    lcd.setCursor(0, 1);
    lcd.print("# of Points < 2");
    handleButtons();
    return 0;
  }

  lcd.print("Calculating...");
  // Calculate slope with linear regression, offset with center of mass, max distance with formulas
  unsigned long x = 0, y = 0, xy = 0, xx = 0;
  uint8_t i;

  for (i = 0; i < numberOfPoints; i++)
  {
    y += points[i];
    x += i * stepping + startPos;
    xy += points[i]*(i * stepping + startPos);
    xx += (i * stepping + startPos)*(i * stepping + startPos);
  }
  float slope = ((float)numberOfPoints*xy - x*y)/(numberOfPoints*xx - x*x),
  offset = ((float)y/numberOfPoints) - (slope*x/numberOfPoints),
  maxDev = 0, currentDev;

  for (i = 0; i < numberOfPoints; i++)
  {
    currentDev = abs(slope * (i * stepping + startPos) - points[i] + offset) / sqrt(1 + slope);
    if (currentDev > maxDev)
      maxDev = currentDev;
  }
  
  EEPROM.put(SLOPE, slope);
  EEPROM.put(OFFSET, offset);
  EEPROM.put(ERR, maxDev);
  displayStoredData();
  return 1;
}

void displayStoredData()
{
  float currentData;
  const String titles[] = {"Slope:", "Intercept:", "Max Deviation:"};
  for (int i = 0; i < sizeof(float)*3; i += sizeof(float))
  {
    EEPROM.get(i, currentData);
    lcd.clear();
    lcd.print(titles[i]);
    lcd.setCursor(0, 1);
    lcd.print(currentData);
    handleButtons();
  }
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
  lcd.clear();
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

void loop()
{
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("mm");
  lcd.setCursor(0, 0);
  float slope, offset;
  EEPROM.get(SLOPE, slope);
  EEPROM.get(OFFSET, offset);
  while(true)
  {
    lcd.print((analogRead(0) - offset) / slope);
    if (digitalRead(8))
    {
      unsigned long holdTime= millis();
      while (digitalRead(8));
      if (millis() - holdTime > BTNTIME)
      {
        calibrate();
        EEPROM.get(SLOPE, slope);
        EEPROM.get(OFFSET, offset);
      } else {
        displayStoredData();
      }
    }
  }
}
