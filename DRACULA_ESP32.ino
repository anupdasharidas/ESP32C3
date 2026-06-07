// cubiec.in

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===========================================================
// BACKGROUND STATE
// ===========================================================

// --- Matrix Rain ---
#define MATRIX_COLS 21          // 128 / 6 columns
uint8_t matrixY[MATRIX_COLS];   // current drop row for each column
uint8_t matrixSpd[MATRIX_COLS]; // speed (1 = every frame, 2 = every other)

// --- Radar ---
float radarAngle = 0.0;
#define MAX_BLIPS 6
struct Blip { int8_t x; int8_t y; uint8_t life; };
Blip blips[MAX_BLIPS];

// --- Oscilloscope ---
float oscPhase = 0.0;
float oscFreq  = 1.0;

// --- TV Static ---
// No persistent state needed — drawn fully random each frame

// --- Hex Grid ---
float hexPulse = 0.0;

// --- Warp Stars ---
#define NUM_STARS 40
struct Star { float x; float y; float z; };
Star stars[NUM_STARS];

// ===========================================================
// LYRIC EFFECT ENUM
// ===========================================================
enum Effect {
  EFFECT_NONE = 0,
  EFFECT_POP,
  EFFECT_SHAKE,
  EFFECT_INVERT,
  EFFECT_GLITCH,
  EFFECT_ZOOM_IN
};

// ===========================================================
// LYRIC DATA  (timings identical to original)
// ===========================================================
struct Lyric {
  unsigned long delayBeforeMs;
  unsigned long durationMs;
  const char*   word;
  Effect        effect;
  uint8_t       size;
  unsigned long calculatedStartTime;
};

Lyric lyrics[] = {
  // Phrase 1
  { 0,   300, "The",     EFFECT_NONE,   2, 0 },
  { 0,   300, "morning", EFFECT_POP,    2, 0 },
  { 0,   300, "light",   EFFECT_NONE,   2, 0 },
  { 0,   300, "is",      EFFECT_NONE,   2, 0 },
  { 0,   300, "turning", EFFECT_NONE,   2, 0 },
  { 100, 500, "BLUE",    EFFECT_INVERT, 3, 0 },

  // Phrase 2
  { 0,   300, "the",     EFFECT_NONE,   2, 0 },
  { 0,   300, "feeling", EFFECT_POP,    2, 0 },
  { 0,   300, "is",      EFFECT_NONE,   2, 0 },
  { 0,   800, "BIZARRE", EFFECT_GLITCH, 2, 0 },

  // Phrase 3
  { 200, 300, "The",     EFFECT_NONE,   2, 0 },
  { 0,   300, "night",   EFFECT_POP,    2, 0 },
  { 0,   300, "is",      EFFECT_NONE,   2, 0 },
  { 0,   300, "almost",  EFFECT_NONE,   2, 0 },
  { 0,   500, "over,",   EFFECT_NONE,   2, 0 },

  // Phrase 4
  { 200, 300, "I",       EFFECT_NONE,   2, 0 },
  { 0,   300, "still",   EFFECT_POP,    2, 0 },
  { 0,   300, "don't",   EFFECT_NONE,   2, 0 },
  { 0,   300, "know",    EFFECT_NONE,   2, 0 },
  { 0,   200, "where",   EFFECT_NONE,   2, 0 },
  { 0,   200, "you",     EFFECT_NONE,   2, 0 },
  { 0,   600, "ARE",     EFFECT_SHAKE,  3, 0 },

  // Phrase 5
  { 200, 300, "The",      EFFECT_NONE,   2, 0 },
  { 0,   400, "shadows,", EFFECT_SHAKE,  2, 0 },
  { 0,   300, "yeah,",    EFFECT_NONE,   2, 0 },
  { 0,   300, "they",     EFFECT_NONE,   2, 0 },
  { 0,   300, "keep",     EFFECT_NONE,   2, 0 },
  { 0,   300, "me",       EFFECT_NONE,   2, 0 },
  { 0,   300, "pretty",   EFFECT_POP,    2, 0 },
  { 0,   300, "like",     EFFECT_NONE,   2, 0 },
  { 0,   300, "a",        EFFECT_NONE,   2, 0 },
  { 0,   300, "movie",    EFFECT_NONE,   2, 0 },
  { 0,   800, "STAR",     EFFECT_INVERT, 3, 0 },

  // Phrase 6
  { 300, 500, "Daylight", EFFECT_POP,  2, 0 },
  { 0,   300, "makes",    EFFECT_NONE, 2, 0 },
  { 0,   300, "me",       EFFECT_NONE, 2, 0 },
  { 0,   300, "feel",     EFFECT_NONE, 2, 0 },
  { 0,   200, "like",     EFFECT_NONE, 2, 0 },

  // THE DROP
  { 100, 2500, "DRACULA", EFFECT_ZOOM_IN, 3, 0 }
};
const int numLyrics = sizeof(lyrics) / sizeof(Lyric);

unsigned long startTime     = 0;
unsigned long TOTAL_LOOP_TIME = 0;
int lastLyricIdx = -1;

// ===========================================================
// SETUP
// ===========================================================
void setup() {
  Serial.begin(115200);
  Wire.begin(20, 21); // ESP32 SDA=20, SCL=21

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found! Check wiring.");
    for (;;) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  // Init Matrix rain columns
  for (int i = 0; i < MATRIX_COLS; i++) {
    matrixY[i]   = random(SCREEN_HEIGHT / 8);
    matrixSpd[i] = random(1, 4);
  }

  // Init Radar blips (all inactive)
  for (int i = 0; i < MAX_BLIPS; i++) blips[i].life = 0;

  // Init Warp stars (spread randomly in 3D space)
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-640, 641);
    stars[i].y = random(-320, 321);
    stars[i].z = random(1, 256);
  }

  // Pre-calculate all lyric start times
  unsigned long t = 0;
  for (int i = 0; i < numLyrics; i++) {
    t += lyrics[i].delayBeforeMs;
    lyrics[i].calculatedStartTime = t;
    t += lyrics[i].durationMs;
  }
  TOTAL_LOOP_TIME = t + 3000;

  startTime = millis();
  Serial.println("Dracula OLED v2 ready!");
}

// ===========================================================
// MAIN LOOP
// ===========================================================
void loop() {
  unsigned long now = millis() - startTime;

  // Loop the whole sequence
  if (now > TOTAL_LOOP_TIME) {
    startTime    = millis();
    now          = 0;
    lastLyricIdx = -1;
  }

  display.clearDisplay();

  // Determine active lyric
  int currentLyricIdx = -1;
  for (int i = 0; i < numLyrics; i++) {
    if (now >= lyrics[i].calculatedStartTime &&
        now <= lyrics[i].calculatedStartTime + lyrics[i].durationMs) {
      currentLyricIdx = i;
      break;
    }
  }

  Effect currentEffect = EFFECT_NONE;
  bool   lyricActive   = false;
  float  progress      = 0.0;
  bool   invertScreen  = false;

  if (currentLyricIdx != -1) {
    lyricActive = true;
    const Lyric& l = lyrics[currentLyricIdx];
    currentEffect   = l.effect;
    progress = (float)(now - l.calculatedStartTime) / l.durationMs;
    lastLyricIdx = currentLyricIdx;
  } else {
    lastLyricIdx = -1;
  }

  // ----------------------------------------------------------
  // 1. DRAW BACKGROUND (effect drives which animation runs)
  // ----------------------------------------------------------
  switch (currentEffect) {
    case EFFECT_POP:     drawRadar(lyricActive);       break;
    case EFFECT_SHAKE:   drawOscilloscope(lyricActive); break;
    case EFFECT_INVERT:  drawTVStatic();                break;
    case EFFECT_GLITCH:  drawHexGrid();                 break;
    case EFFECT_ZOOM_IN: drawWarpStars(true);           break;
    default:             drawMatrixRain(lyricActive);   break;
  }

  // ----------------------------------------------------------
  // 2. DRAW LYRIC TEXT ON TOP
  // ----------------------------------------------------------
  if (currentLyricIdx != -1) {
    const Lyric& l = lyrics[currentLyricIdx];
    int textSize = l.size;
    int offsetX  = 0;
    int offsetY  = 0;

    // Per-effect text styling
    if (l.effect == EFFECT_POP) {
      if (progress < 0.15) textSize = l.size + 1;

    } else if (l.effect == EFFECT_SHAKE) {
      offsetX = random(-3, 4);
      offsetY = random(-3, 4);

    } else if (l.effect == EFFECT_INVERT) {
      invertScreen = true;
      if (random(10) > 5) { offsetX = random(-2, 3); offsetY = random(-2, 3); }

    } else if (l.effect == EFFECT_GLITCH) {
      if (random(10) > 6) {
        offsetX = random(-6, 6);
        // Extra glitch scan-line slashes
        for (int g = 0; g < 2; g++) {
          display.fillRect(0, random(SCREEN_HEIGHT), SCREEN_WIDTH, random(1, 4), WHITE);
        }
      }
      if (random(10) > 8) invertScreen = true;

    } else if (l.effect == EFFECT_ZOOM_IN) {
      if      (progress < 0.05) textSize = max(1, l.size - 1);
      else if (progress < 0.10) textSize = l.size;
      else                       textSize = l.size + 1;
      if (progress > 0.4) { offsetX = random(-4, 5); offsetY = random(-4, 5); }
      if (progress > 0.3 && random(10) > 7) invertScreen = true;
    }

    // Measure text
    display.setTextSize(textSize);
    int16_t  bx, by;
    uint16_t bw, bh;
    display.getTextBounds(l.word, 0, 0, &bx, &by, &bw, &bh);

    // Safety: if too wide, shrink
    if (bw > SCREEN_WIDTH - 8) {
      textSize = 2;
      display.setTextSize(textSize);
      display.getTextBounds(l.word, 0, 0, &bx, &by, &bw, &bh);
    }

    int drawX = (SCREEN_WIDTH  - bw) / 2 + offsetX;
    int drawY = (SCREEN_HEIGHT - bh) / 2 + offsetY - 5;

    // Solid black backing box so text is always readable over any background
    display.fillRoundRect(drawX - 4, drawY - 4, bw + 8, bh + 8, 3, BLACK);
    display.drawRoundRect(drawX - 4, drawY - 4, bw + 8, bh + 8, 3, WHITE);

    // Glitch occasionally inverts the box itself
    if (l.effect == EFFECT_GLITCH && random(10) > 8) {
      display.setTextColor(BLACK, WHITE);
      display.fillRoundRect(drawX - 2, drawY - 2, bw + 4, bh + 4, 3, WHITE);
    } else {
      display.setTextColor(WHITE);
    }

    display.setCursor(drawX, drawY);
    display.print(l.word);
  }

  display.invertDisplay(invertScreen);
  display.display();
  yield(); // Feed ESP32 watchdog
}

// ===========================================================
// BACKGROUND ANIMATION FUNCTIONS
// ===========================================================

// ----------------------------------------------------------
// MATRIX RAIN  (default idle + EFFECT_NONE)
// Falling columns of 0/1 characters, each at its own speed
// ----------------------------------------------------------
void drawMatrixRain(bool active) {
  static uint8_t frameCount = 0;
  frameCount++;

  for (int col = 0; col < MATRIX_COLS; col++) {
    if (frameCount % matrixSpd[col] != 0) continue;

    int x = col * 6;
    int y = matrixY[col] * 8;

    // Bright head character
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(x, y);
    display.print((char)('0' + random(2))); // '0' or '1'

    // Dim trailing character one row above (simulate fade)
    if (matrixY[col] > 1) {
      display.setCursor(x, y - 8);
      // Draw a dark pixel row to fade the previous char slightly
      display.fillRect(x, y - 8, 5, 7, BLACK);
      display.setCursor(x, y - 8);
      display.setTextColor(WHITE);
      // Print dimmed previous char (use smaller random symbol)
      if (random(3) > 0) display.print((char)('0' + random(2)));
    }

    matrixY[col]++;

    // Reset column when it falls off screen
    if (matrixY[col] >= SCREEN_HEIGHT / 8 + 2) {
      matrixY[col]   = 0;
      matrixSpd[col] = random(1, 4);
    }
  }

  // Horizontal scan-line effect
  if (active) {
    int scanY = (millis() / 30) % SCREEN_HEIGHT;
    for (int x = 0; x < SCREEN_WIDTH; x += 2) {
      display.drawPixel(x, scanY, WHITE);
    }
  }
}

// ----------------------------------------------------------
// RADAR SWEEP  (EFFECT_POP)
// Rotating sweep line + random blip dots that fade out
// ----------------------------------------------------------
void drawRadar(bool active) {
  int cx = SCREEN_WIDTH  / 2;
  int cy = SCREEN_HEIGHT / 2;
  int r  = 28; // radar radius

  // Outer circle and cross-hairs
  display.drawCircle(cx, cy, r,       WHITE);
  display.drawCircle(cx, cy, r / 2,   WHITE);
  display.drawLine  (cx, cy - r, cx, cy + r, WHITE);
  display.drawLine  (cx - r, cy, cx + r, cy, WHITE);

  // Sweep line
  int sx = cx + (int)(cos(radarAngle) * r);
  int sy = cy + (int)(sin(radarAngle) * r);
  display.drawLine(cx, cy, sx, sy, WHITE);

  // Slightly trailing line (dim sweep)
  float trailAng = radarAngle - 0.25;
  int tx = cx + (int)(cos(trailAng) * r);
  int ty = cy + (int)(sin(trailAng) * r);
  display.drawLine(cx, cy, tx, ty, WHITE);

  // Spawn a new blip near sweep tip occasionally
  if (random(8) == 0) {
    for (int i = 0; i < MAX_BLIPS; i++) {
      if (blips[i].life == 0) {
        blips[i].x    = cx + (int)(cos(radarAngle) * (r * (0.3 + (float)random(100)/100.0 * 0.7)));
        blips[i].y    = cy + (int)(sin(radarAngle) * (r * (0.3 + (float)random(100)/100.0 * 0.7)));
        blips[i].life = 12 + random(8);
        break;
      }
    }
  }

  // Draw & decay blips
  for (int i = 0; i < MAX_BLIPS; i++) {
    if (blips[i].life > 0) {
      display.drawPixel(blips[i].x,     blips[i].y,     WHITE);
      display.drawPixel(blips[i].x + 1, blips[i].y,     WHITE);
      display.drawPixel(blips[i].x,     blips[i].y + 1, WHITE);
      blips[i].life--;
    }
  }

  radarAngle += active ? 0.14 : 0.06;
  if (radarAngle > 2 * PI) radarAngle -= 2 * PI;
}

// ----------------------------------------------------------
// OSCILLOSCOPE  (EFFECT_SHAKE)
// Two sine waves with shifting frequency/amplitude
// ----------------------------------------------------------
void drawOscilloscope(bool active) {
  // Horizontal centre line
  display.drawLine(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2, WHITE);

  // Primary wave
  int prevY = SCREEN_HEIGHT / 2;
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    float amp  = active ? 18.0 : 8.0;
    float wave = sin((float)x / SCREEN_WIDTH * 2 * PI * oscFreq + oscPhase) * amp;
    wave      += sin((float)x / SCREEN_WIDTH * 2 * PI * oscFreq * 3 + oscPhase * 1.7) * amp * 0.25;
    int y = SCREEN_HEIGHT / 2 + (int)wave;
    y = constrain(y, 0, SCREEN_HEIGHT - 1);
    display.drawLine(x, prevY, x + 1, y, WHITE);
    prevY = y;
  }

  // Secondary ghost wave (offset)
  prevY = SCREEN_HEIGHT / 2;
  for (int x = 0; x < SCREEN_WIDTH; x += 2) {
    float amp  = active ? 10.0 : 4.0;
    float wave = sin((float)x / SCREEN_WIDTH * 2 * PI * oscFreq * 0.5 + oscPhase * 0.8 + 1.0) * amp;
    int y = SCREEN_HEIGHT / 2 + (int)wave;
    y = constrain(y, 0, SCREEN_HEIGHT - 1);
    display.drawPixel(x, y, WHITE);
  }

  oscPhase += active ? 0.22 : 0.08;
  oscFreq   = 1.5 + sin(oscPhase * 0.1) * 1.2;
}

// ----------------------------------------------------------
// TV STATIC NOISE  (EFFECT_INVERT)
// Random bright pixel blocks filling the screen
// ----------------------------------------------------------
void drawTVStatic() {
  // Draw ~200 random white pixels/blocks each frame
  for (int i = 0; i < 200; i++) {
    int x = random(SCREEN_WIDTH);
    int y = random(SCREEN_HEIGHT);
    if (random(3) > 0) {
      display.drawPixel(x, y, WHITE);
    } else {
      display.fillRect(x, y, 2, 1, WHITE);
    }
  }

  // Horizontal "roll bar" scan that sweeps down
  static uint8_t rollY = 0;
  display.fillRect(0, rollY, SCREEN_WIDTH, 3, BLACK);
  rollY = (rollY + 3) % SCREEN_HEIGHT;

  // Occasional full-row bright flash
  if (random(12) == 0) {
    int fy = random(SCREEN_HEIGHT);
    display.fillRect(0, fy, SCREEN_WIDTH, 1, WHITE);
  }
}

// ----------------------------------------------------------
// HEX GRID  (EFFECT_GLITCH)
// Pulsing hexagonal lattice with wave ripple from centre
// ----------------------------------------------------------
void drawHexGrid() {
  // Hex parameters
  const float S  = 7.0;          // hex radius
  const float W  = S * 1.732;    // horizontal spacing
  const float H  = S * 1.5;      // vertical spacing
  const int   cx = SCREEN_WIDTH  / 2;
  const int   cy = SCREEN_HEIGHT / 2;

  for (int row = -1; row <= (int)(SCREEN_HEIGHT / H) + 1; row++) {
    for (int col = -1; col <= (int)(SCREEN_WIDTH / W) + 1; col++) {
      float hx = col * W + (row & 1) * (W / 2.0);
      float hy = row * H;

      float dist = sqrt((hx - cx) * (hx - cx) + (hy - cy) * (hy - cy));
      float wave = sin(dist / 10.0 - hexPulse);

      // Only draw hex edges when wave is positive (creates pulsing pattern)
      if (wave < 0.0) continue;

      // Draw 6 edges of the hexagon
      for (int v = 0; v < 6; v++) {
        float a1 = PI / 180.0 * (60 * v - 30);
        float a2 = PI / 180.0 * (60 * (v + 1) - 30);
        int x1 = (int)(hx + (S - 1) * cos(a1));
        int y1 = (int)(hy + (S - 1) * sin(a1));
        int x2 = (int)(hx + (S - 1) * cos(a2));
        int y2 = (int)(hy + (S - 1) * sin(a2));

        // Skip edges outside screen
        if (x1 < 0 && x2 < 0)           continue;
        if (x1 >= SCREEN_WIDTH  && x2 >= SCREEN_WIDTH)  continue;
        if (y1 < 0 && y2 < 0)           continue;
        if (y1 >= SCREEN_HEIGHT && y2 >= SCREEN_HEIGHT) continue;

        display.drawLine(x1, y1, x2, y2, WHITE);
      }
    }
  }

  hexPulse += 0.18;
}

// ----------------------------------------------------------
// WARP STARS  (EFFECT_ZOOM_IN / THE DROP)
// 3D starfield hyper-drive — stars streak from centre outward
// ----------------------------------------------------------
void drawWarpStars(bool hyperSpeed) {
  const int cx = SCREEN_WIDTH  / 2;
  const int cy = SCREEN_HEIGHT / 2;
  const float speed = hyperSpeed ? 14.0 : 4.0;

  for (int i = 0; i < NUM_STARS; i++) {
    // Project from 3D to 2D screen
    float sx = (stars[i].x / stars[i].z) * 80.0 + cx;
    float sy = (stars[i].y / stars[i].z) * 48.0 + cy;

    // Advance star toward viewer
    stars[i].z -= speed;

    // Reset star when it flies past the viewer
    if (stars[i].z <= 0) {
      stars[i].x = random(-640, 641);
      stars[i].y = random(-320, 321);
      stars[i].z = 255;
    }

    // Project new position after movement (for streak)
    float sx2 = (stars[i].x / stars[i].z) * 80.0 + cx;
    float sy2 = (stars[i].y / stars[i].z) * 48.0 + cy;

    // Skip if off screen
    if (sx2 < 0 || sx2 >= SCREEN_WIDTH || sy2 < 0 || sy2 >= SCREEN_HEIGHT) continue;

    // Draw streak: short line from old to new position
    int x1 = constrain((int)sx,  0, SCREEN_WIDTH  - 1);
    int y1 = constrain((int)sy,  0, SCREEN_HEIGHT - 1);
    int x2 = constrain((int)sx2, 0, SCREEN_WIDTH  - 1);
    int y2 = constrain((int)sy2, 0, SCREEN_HEIGHT - 1);
    display.drawLine(x1, y1, x2, y2, WHITE);

    // Bright pixel at the head
    display.drawPixel(x2, y2, WHITE);
  }
}
