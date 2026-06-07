/*
   ESP32-C3 Super Mini
   SSD1306 128x64 OLED
   MPU6050_tockn

   Falling Sand Simulator
   ----------------------
   - 180 sand particles
   - Particles stay inside border
   - Particles stack naturally
   - Gravity follows MPU6050 tilt

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

// Border margins
const int MIN_X = 1;
const int MAX_X = 126;
const int MIN_Y = 1;
const int MAX_Y = 62;

const int NUM_PARTICLES = 4000;

struct Grain
{
  int x;
  int y;
};

Grain sand[NUM_PARTICLES];

bool world[128][64];

int gx = 0;
int gy = 1;

void showMessage(String txt)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 28);
  display.println(txt);
  display.display();
}

void clearWorld()
{
  for (int x = 0; x < 128; x++)
  {
    for (int y = 0; y < 64; y++)
    {
      world[x][y] = false;
    }
  }
}

bool inside(int x, int y)
{
  return (x >= MIN_X && x <= MAX_X &&
          y >= MIN_Y && y <= MAX_Y);
}

void setup()
{
  Wire.begin(SDA_PIN, SCL_PIN);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  showMessage("OLED OK");
  delay(1000);

  Wire.beginTransmission(0x68);

  if (Wire.endTransmission() != 0)
  {
    showMessage("MPU FAIL");
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

  randomSeed(micros());

  for (int i = 0; i < NUM_PARTICLES; i++)
  {
    sand[i].x = random(20, 108);
    sand[i].y = random(10, 54);
  }
}

void loop()
{
  mpu6050.update();

  float ax = mpu6050.getAngleX();
  float ay = mpu6050.getAngleY();

  // YOUR ORIENTATION
  if (abs(ax) > abs(ay))
  {
    if (ax > 5)
    {
      gx = -1;
      gy = 0;
    }
    else if (ax < -5)
    {
      gx = 1;
      gy = 0;
    }
  }
  else
  {
    if (ay > 5)
    {
      gx = 0;
      gy = 1;
    }
    else if (ay < -5)
    {
      gx = 0;
      gy = -1;
    }
  }

  clearWorld();

  for (int i = 0; i < NUM_PARTICLES; i++)
  {
    world[sand[i].x][sand[i].y] = true;
  }

  // Random update order
  for (int n = 0; n < NUM_PARTICLES; n++)
  {
    int i = random(NUM_PARTICLES);

    int x = sand[i].x;
    int y = sand[i].y;

    world[x][y] = false;

    int nx = x + gx;
    int ny = y + gy;

    bool moved = false;

    if (inside(nx, ny) && !world[nx][ny])
    {
      sand[i].x = nx;
      sand[i].y = ny;
      moved = true;
    }
    else
    {
      if (gx == 0)
      {
        int sx1 = x - 1;
        int sx2 = x + 1;

        if (random(2))
        {
          if (inside(sx1, ny) && !world[sx1][ny])
          {
            sand[i].x = sx1;
            sand[i].y = ny;
            moved = true;
          }
          else if (inside(sx2, ny) && !world[sx2][ny])
          {
            sand[i].x = sx2;
            sand[i].y = ny;
            moved = true;
          }
        }
        else
        {
          if (inside(sx2, ny) && !world[sx2][ny])
          {
            sand[i].x = sx2;
            sand[i].y = ny;
            moved = true;
          }
          else if (inside(sx1, ny) && !world[sx1][ny])
          {
            sand[i].x = sx1;
            sand[i].y = ny;
            moved = true;
          }
        }
      }
      else
      {
        int sy1 = y - 1;
        int sy2 = y + 1;

        if (random(2))
        {
          if (inside(nx, sy1) && !world[nx][sy1])
          {
            sand[i].x = nx;
            sand[i].y = sy1;
            moved = true;
          }
          else if (inside(nx, sy2) && !world[nx][sy2])
          {
            sand[i].x = nx;
            sand[i].y = sy2;
            moved = true;
          }
        }
        else
        {
          if (inside(nx, sy2) && !world[nx][sy2])
          {
            sand[i].x = nx;
            sand[i].y = sy2;
            moved = true;
          }
          else if (inside(nx, sy1) && !world[nx][sy1])
          {
            sand[i].x = nx;
            sand[i].y = sy1;
            moved = true;
          }
        }
      }
    }

    world[sand[i].x][sand[i].y] = true;
  }

  display.clearDisplay();

  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

  for (int i = 0; i < NUM_PARTICLES; i++)
  {
    display.drawPixel(sand[i].x, sand[i].y, SSD1306_WHITE);
  }

  display.display();

  delay(12);
}