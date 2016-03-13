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

// Length of a LEGO stud
#define STUDS 8
// Return code if no button pressed, 0 by default
#define NOBUTTON 0

// Button list. 6 is up, 7 is down, 8 is select
const uint8_t BUTTONLIST[] = {6, 7, 8};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup()
{ 
  float f;
  lcd.begin(16, 2);
  // Setup button pins and measurement pins
  // Pin A4 is for the measurement probe
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
    while(!calibrate());
  }
}
// Handles and waits for button input
// Returns negative if button is hed for certain amount of time and onlift is true
int handleButtons(bool returnImmediately)
{
  uint8_t i = 0;
  unsigned long buttonHeld;
  if (!returnImmediately)
  {
    buttonHeld = millis();
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

  } else {
    for (i = 0; i < sizeof(BUTTONLIST)/sizeof(uint8_t) + 1; i++)
    {
      // Not button was pressed condition, return zero
      if (i >= sizeof(BUTTONLIST)/sizeof(uint8_t))
        return NOBUTTON;
      
      if (digitalRead(BUTTONLIST[i]))
      {
        buttonHeld = millis();
        while(digitalRead(BUTTONLIST[i]));
        if (millis() - buttonHeld > 40)
          break;
      }
    }
  }
  if (millis() - buttonHeld >= BTNTIME)
  {
    return - BUTTONLIST[i];
  }
  return BUTTONLIST[i];
}

bool calibrate()
{
  lcd.clear();
  lcd.print("Calibration");
  lcd.setCursor(0, 1);
  lcd.print("Push btn to ctn");
  if (handleButtons(false) < 0)
    return 0;
  uint16_t startPos = uintCollection("Start dist", "(mm)", 3, 5),
           stepping = uintCollection("Step size", "(mm)", 2, 5),
           currentPos = startPos;
  uint8_t numberOfPoints = 0, input;
  uint16_t points [255], debounce;
  
  if (!stepping)
  {
    lcd.print("Calibration Fail");
    lcd.setCursor(0, 1);
    lcd.print("Step must be > 0");
    handleButtons(false);
    return 0;
  }
  
  lcd.clear();
  lcd.print("Put probe @ ");
  lcd.print(currentPos);
  lcd.setCursor(0, 1);
  int8_t button;
  while (numberOfPoints < 255)
  {
    debounce = 0;
    button = NOBUTTON;
    while(button == NOBUTTON)
    {
      lcd.setCursor(0, 1);
      lcd.print("Reading: ");
      lcd.print(analogRead(A4));
      lcd.print("   ");
      button = handleButtons(true);
    }
    if (button < 0)
      break;
    for (int i = 0; i < 10; i++)
      debounce += analogRead(A4);
    points[numberOfPoints] = round((float)debounce/10);
    ++numberOfPoints;
    currentPos = startPos + stepping * numberOfPoints;
    lcd.setCursor(12, 0);
    lcd.print(currentPos);
  }

  lcd.clear();
  if (numberOfPoints < 2)
  {
    lcd.print("Calibration Fail");
    lcd.setCursor(0, 1);
    lcd.print("# of Points < 2");
    handleButtons(false);
    return 0;
  }

  lcd.print("Calculating...");
  // Calculate slope with linear regression, offset with center of mass, max distance with formulas
  unsigned long x = 0, y = 0, xy = 0, xx = 0;
  uint8_t i;
  for (i = 0; i < numberOfPoints; i++)
  {
    y += points[i];
    x += (i * stepping) + startPos;
    xy += ((unsigned long)points[i]) * ((unsigned long)((i * stepping) + startPos));
    xx += ((unsigned long)((i * stepping) + startPos))*((unsigned long)((i * stepping) + startPos));
  }
  float slope = ((float)((numberOfPoints*xy) - (x*y))) / ((numberOfPoints*xx) - (x*x)),
  offset = ((float)y/numberOfPoints) - (slope*x/numberOfPoints),
  maxDev = 0, currentDev;

  for (i = 0; i < numberOfPoints; i++)
  {
    currentDev = abs(points[i] - (slope * (i * stepping + startPos) + offset));
    if (currentDev > maxDev)
      maxDev = currentDev;
  }

  Serial.println("Distance (mm),Analog Reading");
  for (i = 0; i < numberOfPoints; i++)
  {
    Serial.print(i * stepping + startPos);
    Serial.print(",");
    Serial.println(points[i]);
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
    Serial.print(titles[i/sizeof(float)]);
    Serial.print(" ");
    Serial.println(currentData);
    handleButtons(false);
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
  while (cursorPosition >= 0 && cursorPosition < maxDigits)
  {
    lcd.setCursor(15 - cursorPosition, 0);
    lcd.print("v");
    lcd.setCursor(15 - cursorPosition, 1);
    lcd.print(selectedNum[cursorPosition]);
    action = handleButtons(false);
    if (abs(action) == BUTTONLIST[0])
    {
      selectedNum[cursorPosition] = (selectedNum[cursorPosition] + 1) % 10;
    } else if (abs(action) == BUTTONLIST[1])
    {
      if (!selectedNum[cursorPosition])
        selectedNum[cursorPosition] = 9;
      else
        --selectedNum[cursorPosition];
    } else if (action == BUTTONLIST[2])
    {
      lcd.setCursor(15 - cursorPosition, 0);
      lcd.print(" ");
      cursorPosition = (cursorPosition - 1) % maxDigits;
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
    finalNumber += round(pow(10, i))*selectedNum[i];
  }
  return finalNumber;
}

void loop()
{
  lcd.clear();
  float slope, offset, reading;
  int8_t button;
  uint16_t holdTime;
  EEPROM.get(SLOPE, slope);
  EEPROM.get(OFFSET, offset);
  while(true)
  {
    reading = (analogRead(A4) - offset) / slope;
    lcd.setCursor(0, 0);
    lcd.print(reading);
    lcd.print(" mm            ");
    lcd.setCursor(0, 1);
    lcd.print(round(reading/STUDS));
    lcd.print(" FLU           ");
    // Quick button reading, cannot use handleButtons()
    button = handleButtons(true);
    if (button < 0)
    {
      if (calibrate())
      {
        EEPROM.get(SLOPE, slope);
        EEPROM.get(OFFSET, offset);
      }
    } else if (button != NOBUTTON)
    {
        displayStoredData();
    }
  }
}
