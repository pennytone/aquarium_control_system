#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_PWMServoDriver.h>

const uint8_t pwmLed = 0;
const uint8_t en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3; // Define LCD pinout
const uint8_t piezoPin = 6;
uint16_t daytimeStart = 730;
uint16_t daytimeEnd = 2000;
bool LightOn;
char illuminate[8];
uint16_t pot = A1;
uint8_t pump1 = 5;
uint8_t speaker = 3;
unsigned long int lastIteration = 0;
uint16_t fadeDelay = 500;
uint16_t maxPWMsteps = 4095;

LiquidCrystal_I2C lcd(0x27, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);

RTC_DS3231 rtc;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

bool checkDaytime() {
  DateTime now = rtc.now();
  int nowInteger = now.hour() * 100 + now.minute();

  if (nowInteger > daytimeStart && nowInteger < daytimeEnd) {
    return true;
  }
  return false;
}

void updateLCD(uint16_t interval = 1000) {
  if ((millis() - lastIteration) > interval) {
    lastIteration = millis();

    DateTime now = rtc.now();

    int farenheit = rtc.getTemperature() * 1.8 + 32;
    char buf1[] = "MMM DD YYYY DDD";
    char buf2[] = "hh:mm:ssAP";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(now.toString(buf1));
    lcd.setCursor(0, 1);
    lcd.print(now.toString(buf2));
    lcd.setCursor(11, 1);
    lcd.print(farenheit);
    lcd.setCursor(13, 1);
    lcd.print((char)223); //ascii number for degrees symbol
    lcd.print("F");
  }
}

void setup () {

  pinMode(piezoPin, OUTPUT);
  pinMode(pot, INPUT_PULLUP);
  pinMode(pump1, OUTPUT);

  Serial.begin(57600);
  lcd.begin(16, 2);
  pwm.begin();

  lcd.setCursor(0, 0);
  lcd.print("Aquarium control 1.0");
  delay(1000);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(1600);

  for (uint8_t t = 0; t < 3; t++) {
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

    uint16_t i = 0;
    uint8_t lastPercent;
    while (i < maxPWMsteps) {
      pwm.setPin(pwmLed, i);
      uint8_t percent = map(i, 0, maxPWMsteps, 0, 100);
      if (percent != lastPercent) {
        sprintf(illuminate, "ILLUMINATING %d%%", percent);
        lcd.setCursor(0, 1);
        lcd.print(illuminate);
      }
      lastPercent = percent;
      i++;
      delay(1);
    }

  } else {
    lcd.setCursor(0, 1);
    lcd.print("NIGHT MODE");
    delay(2000);
    LightOn = false;
    lcd.setBacklight(LOW);
    updateLCD();
    pwm.setPin(pwmLed, 0);
    pwm.sleep();
  }
}

void alarmSound(int x = 890, int y = 400) {

  tone(piezoPin, x);
  delay(y);
  noTone(piezoPin);
  delay(y);
}

void lightfadeOn(uint16_t y, uint16_t z = 4095) {

  char illuminate[8];
  uint16_t i = 0;
  uint8_t lastPercent;
  pwm.wakeup();
  while ( i < z) {
    if (i > 511) {
      lcd.setBacklight(HIGH);
    }
    pwm.setPin(pwmLed, i);
    uint8_t percent = map(i, 0, z, 0, 100);
    if (percent != lastPercent) {
      sprintf(illuminate, "ILLUMINATING %d%", percent);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(illuminate);
    }
    lastPercent = percent;
    i++;
    delay(y); // 30 minutes at 4096 steps / 1800 seconds.
  }
}

void lightfadeOff(uint16_t y, uint16_t z = 4095) {

  char dim[8];
  uint8_t lastPercent;
  uint16_t i = z;
  while ( i > 0) {
    if (i < 512) {
      lcd.setBacklight(LOW);
    }
    pwm.setPin(pwmLed, i);
    uint8_t percent = map(i, 0, z, 0, 100);
    if (percent != lastPercent) {
      sprintf(dim, "DIMMING %d%", percent);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dim);
    }
    lastPercent = percent;
    i--;
    delay(y);
  }
  pwm.sleep();
}

void motorControl() {
  uint8_t potVal = analogRead(pot);
  potVal = map(potVal, 17, 216, 0, 255);
  analogWrite(pump1, potVal);
}

void loop () {

  motorControl();

  DateTime now = rtc.now();

  if (checkDaytime()) {
    if (LightOn == 0) {
      for (uint8_t t = 0; t < 7; t++) {
        alarmSound();
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ILLUMINATING");
      LightOn = 1;
      lightfadeOn(fadeDelay, maxPWMsteps);
    }
  } else {
    if (LightOn > 0) {
      for (uint8_t t = 0; t < 4; t++) {
        alarmSound();
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DIMMING");
      LightOn = 0;
      lightfadeOff(fadeDelay, maxPWMsteps);
    }
  }

  updateLCD(1000);
}
