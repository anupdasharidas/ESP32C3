#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050_tockn.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define SDA_PIN 8
#define SCL_PIN 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MPU6050 mpu6050(Wire);

float boxX = 64;
float boxY = 32;

void showMessage(String msg)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 28);
  display.println(msg);
  display.display();
}

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(500);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    while (1);
  }

  showMessage("OLED OK");
  delay(1000);

  Wire.beginTransmission(0x68);
  if (Wire.endTransmission() != 0)
  {
    showMessage("MPU NOT FOUND");
    while (1);
  }

  showMessage("MPU FOUND");
  delay(1000);

  mpu6050.begin();

  showMessage("CALIBRATING");
  delay(1000);

  mpu6050.calcGyroOffsets(true);

  showMessage("READY");
  delay(1000);
}

void loop()
{
  mpu6050.update();

  // Read tilt angles
// Read tilt angles
float angleX = mpu6050.getAngleX();
float angleY = mpu6050.getAngleY();

// Correct axis orientation
int targetX = map((int)angleX, -45, 45, 122, 0);
int targetY = map((int)angleY, -45, 45, 0, 58);
  // Smooth movement
  boxX = boxX * 0.85 + targetX * 0.15;
  boxY = boxY * 0.85 + targetY * 0.15;

  display.clearDisplay();

  // Draw border
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

  // Draw center marker
  display.drawLine(62, 32, 66, 32, SSD1306_WHITE);
  display.drawLine(64, 30, 64, 34, SSD1306_WHITE);

  // Draw moving square
  display.fillRect((int)boxX, (int)boxY, 6, 6, SSD1306_WHITE);

  display.display();

  delay(20);
}