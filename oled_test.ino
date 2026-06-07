#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting OLED test...");

  // 🔥 Use GPIO 8 (SDA) and GPIO 9 (SCL)
  Wire.begin(8, 9);

  // Try default I2C address
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED not found at 0x3C");
    
    // Try alternate address
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("❌ OLED not found at 0x3D");
      while(1); // stop here
    }
  }

  Serial.println("✅ OLED Found!");

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 10);
  display.println("ESP32-S3");

  display.setCursor(0, 35);
  display.println("OLED OK");

  display.display();
}

void loop() {
  // nothing needed
}
