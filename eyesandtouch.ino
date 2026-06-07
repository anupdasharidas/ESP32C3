#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include <Otto9.h>

SSD1306AsciiWire oled;
Otto9 Otto;

// Pin Definitions
#define PIN_Touch 2       // Touch sensor signal
#define PIN_YL 8          
#define PIN_YR 9          
#define PIN_RL 10         
#define PIN_RR 11         
#define PIN_Buzzer 13     

// Variables to track mood
int moodCounter = 0;
bool touched = false;

void setup() {
  Wire.begin();
  delay(1000);

  // Initialize display
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(System5x7);
  oled.clear();

  // Initialize Touch Pin
  pinMode(PIN_Touch, INPUT);

  // Initialize Otto
  Otto.init(PIN_YL, PIN_YR, PIN_RL, PIN_RR, true, A6, PIN_Buzzer, -1, -1);
  Otto.sing(S_connection);
  
  updateMood(); // Show first mood
}

void loop() {
  // Check for touch
  if (digitalRead(PIN_Touch) == HIGH && !touched) {
    touched = true;
    moodCounter++;
    if (moodCounter > 3) moodCounter = 0; // Reset after 4 moods
    
    Otto.sing(S_buttonPushed);
    updateMood();
    delay(300); // Debounce delay
  }
  
  if (digitalRead(PIN_Touch) == LOW) {
    touched = false;
  }

  // Otto's behavior based on mood
  if (moodCounter == 0) { // HAPPY
    Otto.walk(1, 1000, 1);
  } 
  else if (moodCounter == 1) { // ANGRY
    Otto.jump(1, 500);
  }
  else if (moodCounter == 2) { // DIZZY
    Otto.turn(1, 600, 1);
  }
  else if (moodCounter == 3) { // LOVE
    Otto.bend(1, 1000, 1);
    Otto.bend(1, 1000, -1);
  }
}

void updateMood() {
  oled.clear();
  oled.set1X();
  
  switch (moodCounter) {
    case 0: // HAPPY ROUND EYES
      oled.setCursor(35, 2); oled.print(" (00)    (00)"); 
      oled.setCursor(33, 3); oled.print("(0000)  (0000)"); 
      oled.setCursor(35, 4); oled.print(" (00)    (00)");
      break;
      
    case 1: // ANGRY
      oled.setCursor(35, 2); oled.print("\\\\      //"); 
      oled.setCursor(35, 3); oled.print("XXXX    XXXX");
      break;
      
    case 2: // DIZZY
      oled.setCursor(35, 2); oled.print(" x        x "); 
      oled.setCursor(35, 3); oled.print("x x      x x");
      oled.setCursor(35, 4); oled.print(" x        x ");
      break;

    case 3: // LOVE
      oled.setCursor(35, 2); oled.print("<3      <3");
      oled.setCursor(35, 3); oled.print(" <3    <3 ");
      break;
  }
}
