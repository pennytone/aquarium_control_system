#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int ledPin = 9;
const int en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3; // Define LCD pinout
const int i2c_addr = 0x27;
const int piezoPin = 6;
int daytimeStart = 730;
int daytimeEnd = 2000;
bool LightOn;
char militarytime[16];
char date[16];
char illuminate[8];
int illuminateTime = 4705;
int dimTime = 6250;
int pot = A1;
int pump1 = 3;
unsigned long int lastIteration = 0;

LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

bool checkDaytime() {
  DateTime now = rtc.now();
  int nowInteger = now.hour() * 100 + now.minute();

  if (nowInteger > daytimeStart && nowInteger < daytimeEnd) {
    return true;
  }

  return false;
}

void updateLCD(int interval = 1000) {
  if ((millis() - lastIteration) > interval) {
    lastIteration = millis();
    Serial.println("updating the LCD now");

    DateTime now = rtc.now();

    int farenheit = rtc.getTemperature() * 1.8 + 32; // ds3231 temp units are in celcius by default, convert celcius to Fahrenheit.
    char buf1[] = "MMM DD YYYY DDD";
    char buf2[] = "hh:mm:ss AP";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(now.toString(buf1));
    lcd.setCursor(0, 1);
    lcd.print(now.toString(buf2));
    lcd.setCursor(12, 1);
    lcd.print(farenheit);

  }
}

//// trying to create a function that uses millis rather than delay.
//String updateInterval(String function, int delayTime = 1000) {
//
//  int interval = delayTime;
//
//  if (interval - millis() > lastInterval) {
//    lastInterval = millis();
//    return function;
//  }
//}

void setup () {
  Serial.begin(57600);

  lcd.begin(16, 2);
  lcd.print("Aquarium control 1.0");
  delay(1000);

  pinMode(ledPin, OUTPUT);
  pinMode(piezoPin, OUTPUT);
  pinMode(pot, INPUT_PULLUP);
  pinMode(pump1, OUTPUT);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  for (int t = 0; t < 3; t++) {
    tone(piezoPin, 890);
    delay(400);
    noTone(piezoPin);
    delay(200);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CHECKING TIME...");
  delay(1000);

  DateTime now = rtc.now();

  if (checkDaytime()) {
    lcd.setCursor(0, 1);
    lcd.print("DAY MODE");
    delay(2000);
    LightOn = true;
    lcd.setBacklight(HIGH);
    lcd.print("illuminate");

    for (int x = 0; x < 255; x++) {
      analogWrite(ledPin, x);
      int percent = map(x, 0, 255, 0, 100);
      sprintf(illuminate, "illuminating %d%%", percent);
      lcd.print(illuminate);
      lcd.clear();
      delay(40);
    }

  } else {
    lcd.setCursor(0, 1);
    lcd.print("NIGHT MODE");
    delay(1000);
    LightOn = false;
    lcd.setBacklight(LOW);
    updateLCD();
    analogWrite(ledPin, 0);
  }

  lastIteration = millis();

}

void alarmSound(int x = 890, int y = 400) {

  tone(piezoPin, x);
  delay(y);
  noTone(piezoPin);
  delay(y);
}

void lightfadeOn() {

  char illuminate[8];
  for (int x = 0; x < 192; x++) { // using 192 as starting point because psu max output is reached at 192 steps
    if (x >= 5) {
      lcd.setBacklight(HIGH);
    }
    analogWrite(ledPin, x);
    int percent = map(x, 0, 192, 0, 100);
    sprintf(illuminate, "ILLUMINATE %d", percent);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(illuminate);
    delay(illuminateTime); // 20 minutes = 1200 seconds / 256 steps = 4.705 * 1000 (milliseconds) = 4705
  }
  analogWrite(ledPin, 255);
}

void lightfadeOff() {

  char dim[8];
  for (int y = 192; y >= 0; y-- ) { // using 192 as starting point because psu doesnt give enough power to exceed 192 steps
    analogWrite(ledPin, y);
    int percent = map(y, 0, 192, 0, 100);
    sprintf(dim, "DIMMING %d", percent);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dim);
    if (y <= 5) {
      lcd.setBacklight(LOW);
    }
    delay(dimTime);
  }
}

void motorControl() {
  byte potVal = analogRead(pot);
  potVal = map(potVal, 17, 216, 0, 255);
  analogWrite(pump1, potVal);
  //Serial.print("potVal = ");
  //Serial.println(potVal);

}

void loop () {

  motorControl();

  DateTime now = rtc.now();

  if (checkDaytime()) {
    if (LightOn == 0) {
      for (int t = 0; t < 7; t++) {
        alarmSound();
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ILLUMINATING");
      LightOn = 1;
      //      lcd.setBacklight(HIGH);
      lightfadeOn();
    }
  }
  else {
    if (LightOn > 0) {
      for (int t = 0; t < 4; t++) {
        alarmSound();
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DIMMING");
      LightOn = 0;
      lightfadeOff();
      //      lcd.setBacklight(LOW);
    }
  }

  updateLCD(1000);
}
