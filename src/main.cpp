#include <Arduino.h>
#include <math.h>
#include <time.h>
#include <Adafruit_BNO08x.h>
#include <AceButton.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
using namespace ace_button;

void setReports();
void displayScreen();

#define BNO08X_RESET -1

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int pinD0 = 0;
int pinD1 = 1;
int pinD2 = 2;

AceButton buttonD0(pinD0);
AceButton buttonD1(pinD1);
AceButton buttonD2(pinD2);

enum ScreenMode {
  MODE_STEPS,     // 0
  MODE_DISTANCE,  // 1
  MODE_STRIDE,    // 2
  MODE_ACCEL,     // 3
  MODE_COUNT      // 4
};

ScreenMode curMode = MODE_STEPS;

int stepCount = 0;

float strideLength = 0.75;
float distanceTraveled = 0.0;

float x = 0.0;
float y = 0.0;
float z = 0.0;

unsigned long previousDisplayTime = 0;
long displayTime = 200;

void ChangeMode(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  if (eventType != AceButton::kEventClicked) {
    return;
  }

  if (button == &buttonD0) {
    curMode = (ScreenMode)(((int)curMode + 1) % MODE_COUNT);
    displayScreen();
  } else if (button == &buttonD1 && curMode == MODE_STRIDE) {
    strideLength += 0.05;
    if (strideLength > 2.00) {
      strideLength = 2.00;
    }
  } else if (button == &buttonD2 && curMode == MODE_STRIDE) {
    strideLength -= 0.05;
    if (strideLength < 0.20) {
      strideLength = 0.20;
    }
  }
}

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

void setup() {
  Serial.begin(115200);
  while (!Serial){
    delay(10);
  }
    
    // Try to initialize!
    if (!bno08x.begin_I2C()) {
    // if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte
    // UART buffer! if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
  }

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);


  pinMode(pinD0, INPUT_PULLUP);
  buttonD0.init(pinD0, HIGH);

  pinMode(pinD1, INPUT_PULLDOWN);
  buttonD1.init(pinD1, LOW);

  pinMode(pinD2, INPUT_PULLDOWN);
  buttonD2.init(pinD2, LOW);

  ButtonConfig* buttonConfigD0 = buttonD0.getButtonConfig();

  buttonConfigD0->setEventHandler(ChangeMode);
  buttonConfigD0->setFeature(ButtonConfig::kFeatureClick);

  ButtonConfig* buttonConfigD1 = buttonD1.getButtonConfig();

  buttonConfigD1->setEventHandler(ChangeMode);
  buttonConfigD1->setFeature(ButtonConfig::kFeatureClick);

  ButtonConfig* buttonConfigD2 = buttonD2.getButtonConfig();

  buttonConfigD2->setEventHandler(ChangeMode);
  buttonConfigD2->setFeature(ButtonConfig::kFeatureClick);

  Serial.println("Adafruit BNO08x test!");
  Serial.println("BNO08x Found!");

  setReports();
}

void loop() {
  buttonD0.check();
  buttonD1.check();
  buttonD2.check();

  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  if (sensorValue.sensorId == SH2_ACCELEROMETER) {
    x = sensorValue.un.accelerometer.x;
    y = sensorValue.un.accelerometer.y;
    z = sensorValue.un.accelerometer.z;

    // float alpha = atan2(x, sqrt(y * y + z * z)) * RAD_TO_DEG;
    // float beta = atan2(y, z) * RAD_TO_DEG;
  }

  if (sensorValue.sensorId == SH2_STEP_COUNTER) {
    stepCount = sensorValue.un.stepCounter.steps;

    Serial.print("Steps: ");
    Serial.println(stepCount);
  }

  distanceTraveled = stepCount * strideLength;

  unsigned long currentTime = millis();

  if (currentTime > previousDisplayTime + displayTime) {
    previousDisplayTime = currentTime;
    displayScreen();
  }
}

void displayScreen() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);

  if (curMode == MODE_STEPS) {
    tft.setCursor(10, 15);
    tft.setTextSize(2);
    tft.println("Steps Taken");

    tft.setCursor(10, 55);
    tft.setTextSize(4);
    tft.println(stepCount);
  } else if (curMode == MODE_DISTANCE) {
    tft.setCursor(10, 15);
    tft.setTextSize(2);
    tft.println("Distance");

    tft.setCursor(10, 55);
    tft.setTextSize(3);
    tft.print(distanceTraveled, 2);
    tft.println(" m");
  } else if (curMode == MODE_STRIDE) {
    tft.setCursor(10, 15);
    tft.setTextSize(2);
    tft.println("Stride Length");

    tft.setCursor(10, 55);
    tft.setTextSize(3);
    tft.print(strideLength, 2);
    tft.println(" m");

    tft.setCursor(10, 105);
    tft.setTextSize(1);
    tft.println("D1: Increase D2: Decrease");
  } else if (curMode == MODE_ACCEL) {
    tft.setCursor(10, 5);
    tft.setTextSize(2);
    tft.println("Acceleration");

    tft.setCursor(10, 35);
    tft.print("X: ");
    tft.println(x, 2);

    tft.setCursor(10, 60);
    tft.print("Y: ");
    tft.println(y, 2);

    tft.setCursor(10, 85);
    tft.print("Z: ");
    tft.println(z, 2);
  }

  tft.setCursor(10, 122);
  tft.setTextSize(1);
  tft.println("D0: Next Screen");
}

void setReports(void) {
  Serial.println("Setting desired reports");

  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  } else {
    Serial.println("Set accelerometer report... success!");
  }

  if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
    Serial.println("Could not enable step counter");
  } else {
    Serial.println("Set step counter report... success!");
  }
}