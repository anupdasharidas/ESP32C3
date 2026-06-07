#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/i2s.h>

// ===== I2C BUS =====
TwoWire I2C_1 = TwoWire(0);
TwoWire I2C_2 = TwoWire(1);

// ===== OLED =====
Adafruit_SSD1306 leftDisplay(128,64,&I2C_1,-1);
Adafruit_SSD1306 rightDisplay(128,64,&I2C_2,-1);

// ===== PINS =====
#define TOUCH_PIN 4
#define SPEAKER_PIN 18

#define I2S_WS 39
#define I2S_SD 41
#define I2S_SCK 40

#define SAMPLE_RATE 16000

unsigned long lastAnim = 0;
bool showingText = false;

// ===== EYE DRAW =====
void drawEyes(int offset){
  leftDisplay.clearDisplay();
  rightDisplay.clearDisplay();

  leftDisplay.fillCircle(64+offset,32,18,WHITE);
  leftDisplay.fillCircle(64+offset,32,8,BLACK);

  rightDisplay.fillCircle(64-offset,32,18,WHITE);
  rightDisplay.fillCircle(64-offset,32,8,BLACK);

  leftDisplay.display();
  rightDisplay.display();
}

// ===== ANIMATION =====
void animateEyes(){
  int move = random(-6,6);
  drawEyes(move);
}

// ===== HEART =====
void drawHeart(){
  leftDisplay.clearDisplay();
  rightDisplay.clearDisplay();

  // left
  leftDisplay.fillCircle(50,30,10,WHITE);
  leftDisplay.fillCircle(70,30,10,WHITE);
  leftDisplay.fillTriangle(40,30,80,30,60,55,WHITE);

  // right
  rightDisplay.fillCircle(50,30,10,WHITE);
  rightDisplay.fillCircle(70,30,10,WHITE);
  rightDisplay.fillTriangle(40,30,80,30,60,55,WHITE);

  leftDisplay.display();
  rightDisplay.display();
}

// ===== TEXT =====
void showText(String txt){
  leftDisplay.clearDisplay();
  rightDisplay.clearDisplay();

  leftDisplay.setCursor(0,0);
  rightDisplay.setCursor(0,0);

  leftDisplay.setTextSize(1);
  rightDisplay.setTextSize(1);

  leftDisplay.println(txt);
  rightDisplay.println(txt);

  leftDisplay.display();
  rightDisplay.display();
}

// ===== MIC INIT =====
void initMic(){
  i2s_config_t cfg={
    .mode=(i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
    .sample_rate=SAMPLE_RATE,
    .bits_per_sample=I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format=I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format=I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count=8,
    .dma_buf_len=512
  };

  i2s_pin_config_t pin={
    .bck_io_num=I2S_SCK,
    .ws_io_num=I2S_WS,
    .data_in_num=I2S_SD
  };

  i2s_driver_install(I2S_NUM_0,&cfg,0,NULL);
  i2s_set_pin(I2S_NUM_0,&pin);
}

// ===== SPEECH DETECT =====
bool detectSpeech(){
  int32_t raw[64];
  size_t bytes;

  i2s_read(I2S_NUM_0,raw,sizeof(raw),&bytes,portMAX_DELAY);

  int maxVal=0;

  for(int i=0;i<bytes/4;i++){
    int v = abs(raw[i]>>14);
    if(v>maxVal) maxVal=v;
  }

  return maxVal>2000;
}

// ===== FAKE STT =====
String getSpeech(){
  return "Hello Anup!";
}

// ===== SETUP =====
void setup(){
  Serial.begin(115200);

  pinMode(TOUCH_PIN,INPUT);
  pinMode(SPEAKER_PIN,OUTPUT);

  I2C_1.begin(8,9);
  I2C_2.begin(16,17);

  leftDisplay.begin(SSD1306_SWITCHCAPVCC,0x3C);
  rightDisplay.begin(SSD1306_SWITCHCAPVCC,0x3C);

  initMic();

  randomSeed(analogRead(0));
}

// ===== LOOP =====
void loop(){

  // ❤️ TOUCH
  if(digitalRead(TOUCH_PIN)==HIGH){
    drawHeart();
    delay(1000);
    return;
  }

  // 🎤 SPEECH
  if(detectSpeech()){
    showingText = true;

    String txt = getSpeech();
    Serial.println("Heard: " + txt);

    showText(txt);
    delay(2000);

    showingText = false;
  }

  // 👀 ANIMATION
  if(!showingText && millis()-lastAnim>500){
    animateEyes();
    lastAnim = millis();
  }
}
