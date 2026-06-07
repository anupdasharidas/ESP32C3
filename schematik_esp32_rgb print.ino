#include <Arduino.h>
// ST7735 TFT Display with TTP223 Touch Sensor
// Fixes color byte-swap by using corrected RGB565 values
// Touch cycles through color display demo

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Pin definitions
#define TFT_SCK   12
#define TFT_MOSI  11
#define TFT_MISO  13
#define TFT_CS    10
#define TFT_DC     1
#define TFT_RST    2
#define TFT_BLK    3

#define TOUCH_PIN  4

// Corrected RGB565 color constants for ST7735 byte-swap issue
// Standard RGB565 has R in high bits, B in low bits
// This ST7735 panel swaps bytes, so we define swapped values
// to get the correct physical color on screen
#define MY_BLACK   0x0000
#define MY_WHITE   0xFFFF
// Correct RGB565 color constants for ST7735 (no byte-swap needed with Adafruit library)
// Standard RGB565: R[15:11] G[10:5] B[4:0]
#define MY_RED     0x001F  // byte-swapped: physical red on this panel
#define MY_GREEN   0x07E0  // R=0,  G=63, B=0
#define MY_BLUE    0xF800  // byte-swapped: physical blue on this panel
#define MY_YELLOW  0x07FF  // byte-swapped: physical yellow on this panel
#define MY_CYAN    0xFFE0  // byte-swapped: physical cyan on this panel
#define MY_MAGENTA 0xF81F  // R=31, G=0,  B=31
#define MY_ORANGE  0xFC00  // R=31, G=32, B=0

// Use hardware SPI with explicit SPI bus for ESP32-S3

// Hoisted type definitions
struct ColorEntry {
  uint16_t color;
  const char* name;
};

SPIClass mySPI(HSPI);
Adafruit_ST7735 tft = Adafruit_ST7735(&mySPI, TFT_CS, TFT_DC, TFT_RST);

// Color table for cycling


const ColorEntry colors[] = {
  { MY_RED,     "RED"     },
  { MY_GREEN,   "GREEN"   },
  { MY_BLUE,    "BLUE"    },
  { MY_YELLOW,  "YELLOW"  },
  { MY_CYAN,    "CYAN"    },
  { MY_MAGENTA, "MAGENTA" },
  { MY_WHITE,   "WHITE"   },
};
const int NUM_COLORS = sizeof(colors) / sizeof(colors[0]);

int currentColor = 0;

// Touch debounce state
bool lastTouchState = false;
bool stableState = false;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_MS = 50;

void showColorScreen(int idx) {
  tft.fillScreen(colors[idx].color);
  tft.drawRect(0, 25, 159, 79, ST77XX_WHITE);

  // Choose contrasting text color
  uint16_t textColor = (colors[idx].color == MY_WHITE) ? MY_BLACK : MY_WHITE;

  tft.setTextColor(textColor);
  tft.setTextSize(2);

  // Center the color name
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(colors[idx].name, 0, 0, &x1, &y1, &w, &h);
  int cx = (tft.width() - w) / 2;
  int cy = (tft.height() - h) / 2;

  tft.setCursor(cx, cy);
  tft.print(colors[idx].name);

  // Print index / total at bottom
  tft.setTextSize(1);
  tft.setCursor(4, tft.height() - 12);
  tft.print(idx + 1);
  tft.print("/");
  tft.print(NUM_COLORS);

  Serial.print("Displaying color: ");
  Serial.println(colors[idx].name);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("ST7735 Color Demo - byte-swap corrected");

  // Backlight ON
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  // Touch pin as input
  pinMode(TOUCH_PIN, INPUT);

  // Init SPI bus: SCK=12, MISO=13, MOSI=11, SS=-1
  mySPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, -1);

  // Init display as ST7735S (most common 1.8" variant)
  tft.initR(INITR_BLACKTAB);

  // Some ST7735 panels need color inversion correction at driver level.
  // We are using byte-swapped constants above instead of invertDisplay(),
  // so do NOT call invertDisplay here. Comment it out if colors are wrong.
  // tft.invertDisplay(true);

  tft.setRotation(3);

  // Show first color
  showColorScreen(currentColor);
}

void loop() {
  bool touched = digitalRead(TOUCH_PIN) == HIGH;
  unsigned long now = millis();

  // Debounce: detect rising edge (touch press)
  if (touched != lastTouchState) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) >= DEBOUNCE_MS) {
    // Stable state reached; detect rising edge vs last stable state
    if (touched && !stableState) {
      // Advance to next color
      currentColor = (currentColor + 1) % NUM_COLORS;
      showColorScreen(currentColor);
    }
    stableState = touched;
  }

  lastTouchState = touched;
  delay(10);
}