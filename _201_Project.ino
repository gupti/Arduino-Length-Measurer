// MTE 201 Project
// Program for a distance measurement device using a potentiomenter (linear)

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <math.h>
// Defines for EEPROM locations, and default hold-down time
#define SLOPE 0
#define OFFSET 4
#define ERR 8
#define BTNTIME 1500

// Button list. 6 is up, 7 is down, 8 is select
const uint8_t BUTTONLIST[] = {6, 7, 8};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup()
{ 
  float f;
  lcd.begin(16, 2);
  // Setup button pins and measurement pins
  // Pin A0 is for the measurement probe
  for (int i = 0; i < sizeof(BUTTONLIST)/sizeof(uint8_t); i++)
  {
    pinMode(BUTTONLIST[i], INPUT);
  }
  
  EEPROM.get(SLOPE, f);
  // If the slope is zero, the device is not calibrated [properly]
  if (isnan(f) || abs(f) < 0.00001)
  {
    f = 0;
    // Clear EEPROM
    for (int i = 0; i < sizeof(float)*3; i += sizeof(float))
    {
      EEPROM.put(i, 0.00f);
    }
    do
    {
      lcd.print("Calibration req");
      lcd.setCursor(0, 1);
      lcd.print("Push btn to ctn");
      handleButtons();
    } while(!calibrate());
  }
}
// Handles and waits for button input
// Returns negative if button is hed for certain amount of time and onlift is true
int handleButtons()
{
  uint8_t i = 0;
  unsigned long buttonHeld = millis();
  while(millis() - buttonHeld < 40)
  {
    while (true)
    {
      i = (i + 1) % (sizeof(BUTTONLIST)/sizeof(uint8_t));
      if (digitalRead(BUTTONLIST[i]) == HIGH)
      {
        buttonHeld = millis();
        break;
      }
    }
    while(digitalRead(BUTTONLIST[i]) == HIGH);
  }
  if (millis() - buttonHeld >= BTNTIME)
  {
    return - BUTTONLIST[i];
  }
  return BUTTONLIST[i];
}

bool calibrate()
{

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
  
  lcd.clear();
  lcd.print("Put probe @ ");
  lcd.print(currentPos);
  lcd.setCursor(0, 1);
  lcd.print("and press btn.");
  while (input = handleButtons() != -BUTTONLIST[2])
  {
    points[numberOfPoints] = analogRead(0);
    ++numberOfPoints;
    currentPos = startPos + stepping*numberOfPoints;
    lcd.setCursor(12, 0);
    lcd.print(currentPos);
  }

  lcd.clear();
  if (numberOfPoints < 2)
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
    currentDev = abs(points[i] - (slope * (i * stepping + startPos) + offset));
//    currentDev = abs(slope * (i * stepping + startPos) - points[i] + offset) / sqrt(1 + slope);
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
    lcd.print(titles[i/sizeof(float)]);
    lcd.setCursor(0, 1);
    lcd.print(currentData);
    handleButtons();
  }
  lcd.clear();
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
  lcd.setCursor(16 - maxDigits, 1);
  uint8_t selectedNum [4];
  memset(selectedNum, 0, sizeof(selectedNum));
  for (int i = 0; i < maxDigits; i++)
    lcd.print(selectedNum[maxDigits - i - 1] = defaultNum/pow(10, maxDigits - i - 1));

  uint8_t cursorPosition = maxDigits - 1;
  int action;
  while (cursorPosition >= 0 && cursorPosition <= maxDigits)
  {
    lcd.setCursor(15 - cursorPosition, 0);
    lcd.print("v");
    lcd.setCursor(15 - cursorPosition, 1);
    lcd.print(selectedNum[cursorPosition]);
    action = handleButtons();
    if (abs(action) == BUTTONLIST[1])
    {
      selectedNum[cursorPosition] = (selectedNum[cursorPosition] + 1) % 10;
    } else if (abs(action) == BUTTONLIST[0])
    {
      selectedNum[cursorPosition] = (selectedNum[cursorPosition] - 1) % 10;
    } else if (action == BUTTONLIST[2])
    {
      lcd.setCursor(15 - cursorPosition, 0);
      lcd.print(" ");
      --cursorPosition;
    } else if (action == - BUTTONLIST[2])
    {
      lcd.setCursor(15 - cursorPosition, 0);
      lcd.print(" ");
      cursorPosition = (cursorPosition + 1) % maxDigits;
    }
  }

  uint16_t finalNumber = 0;
  for (int i = 0; i < maxDigits; i++)
  {
    finalNumber += selectedNum[i]*pow(10, i);
  }
  return finalNumber;
}

void loop()
{
  lcd.clear();
  float slope, offset;
  unsigned long holdTime;
  EEPROM.get(SLOPE, slope);
  EEPROM.get(OFFSET, offset);
  while(true)
  {
    lcd.setCursor(0, 0);
    lcd.print((analogRead(0) - offset) / slope);
    lcd.print("mm");
    // Quick button reading, cannot use handleButtons()
    if (digitalRead(8) == HIGH)
    {
      holdTime = millis();
      while (digitalRead(8) == HIGH);
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
