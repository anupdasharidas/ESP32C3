/*
   VENOM JAR
   ESP32-C3 Super Mini
   SSD1306 128x64 OLED
   MPU6050_tockn

   Effect:
   - Bright "glass jar"
   - Black living symbiote
   - Blob follows MPU6050 tilt
   - Edge wriggles continuously
   - Small droplets break off and rejoin

   Libraries:
   Adafruit GFX
   Adafruit SSD1306
   MPU6050_tockn
*/

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

float blobX = 64;
float blobY = 32;

unsigned long tick = 0;

struct Drop
{
  float x;
  float y;
  float vx;
  float vy;
};

#define NUM_DROPS 10

Drop d[NUM_DROPS];

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(800000);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  mpu6050.begin();
  delay(500);
  mpu6050.calcGyroOffsets(true);

  randomSeed(micros());

  for (int i = 0; i < NUM_DROPS; i++)
  {
    d[i].x = blobX;
    d[i].y = blobY;
    d[i].vx = random(-10, 10) / 10.0;
    d[i].vy = random(-10, 10) / 10.0;
  }
}

void drawBlob(int cx, int cy)
{
  for (int a = 0; a < 360; a += 6)
  {
    float r = 16;

    r += 2.5 * sin((a + tick * 3) * 0.05);
    r += 2.0 * sin((a * 2 + tick * 2) * 0.04);
    r += 1.5 * cos((a * 4 + tick * 5) * 0.03);

    int x = cx + cos(a * 0.0174533) * r;
    int y = cy + sin(a * 0.0174533) * r;

    display.drawLine(cx, cy, x, y, SSD1306_BLACK);
  }

  display.fillCircle(cx, cy, 14, SSD1306_BLACK);

  for (int i = 0; i < 8; i++)
  {
    int tx = cx + sin((tick + i * 25) * 0.08) * 10;
    int ty = cy + cos((tick + i * 17) * 0.06) * 10;

    int ex = tx + sin((tick + i * 40) * 0.15) * 8;
    int ey = ty + cos((tick + i * 35) * 0.14) * 8;

    display.drawLine(tx, ty, ex, ey, SSD1306_BLACK);
  }
}

void loop()
{
  tick++;

  mpu6050.update();

  float angleX = mpu6050.getAngleX();
  float angleY = mpu6050.getAngleY();

  int targetX = map((int)angleX, -45, 45, 122, 6);
  int targetY = map((int)angleY, -45, 45, 6, 58);

  blobX = blobX * 0.96 + targetX * 0.04;
  blobY = blobY * 0.96 + targetY * 0.04;

  display.clearDisplay();

  // bright jar
  display.fillRect(1, 1, 126, 62, SSD1306_WHITE);

  // border
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

  // animate droplets
  for (int i = 0; i < NUM_DROPS; i++)
  {
    float dx = blobX - d[i].x;
    float dy = blobY - d[i].y;

    d[i].vx += dx * 0.002;
    d[i].vy += dy * 0.002;

    d[i].vx += random(-10, 11) * 0.003;
    d[i].vy += random(-10, 11) * 0.003;

    d[i].x += d[i].vx;
    d[i].y += d[i].vy;

    if (d[i].x < 2) d[i].x = 2;
    if (d[i].x > 125) d[i].x = 125;

    if (d[i].y < 2) d[i].y = 2;
    if (d[i].y > 61) d[i].y = 61;

    display.fillCircle((int)d[i].x,
                       (int)d[i].y,
                       2,
                       SSD1306_BLACK);

    display.drawLine(blobX,
                     blobY,
                     d[i].x,
                     d[i].y,
                     SSD1306_BLACK);
  }

  drawBlob((int)blobX, (int)blobY);

  // internal motion
  for (int i = 0; i < 25; i++)
  {
    int rx = blobX + random(-10, 11);
    int ry = blobY + random(-10, 11);

    display.drawPixel(rx, ry, SSD1306_WHITE);
  }

  display.display();
}