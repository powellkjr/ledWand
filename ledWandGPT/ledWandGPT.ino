#include <FastLED.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

//     5   P   P   P
//     v   2   1   0 
//     |   |   |   |
//  __ 8 _ 7 _ 6 _ 5 __
// |                   |
//  \                  |
//  /                  |
// |__   _   _   _   __|
//     1   2   3   4
//     |   |   |   |
//     R   P   P   0
//     S   3   4   V

#define LED_PIN     0   // ATTINY85 Pin 5 (PB0)
#define NUM_LEDS    20
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define BUTTON1_PIN 3   // PB3 - Effect selector
#define BUTTON2_PIN 4   // PB4 - Effect trigger

#define EEPROM_ADDRESS 0
#define IDLE_TIMEOUT_MS 60000UL  // 1 minute

int currentEffect = 0;
const int totalEffects = 6;
int lastIdleEffect = -1;
unsigned long lastActivityTime = 0;

void setup() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  currentEffect = EEPROM.read(EEPROM_ADDRESS);
  if (currentEffect >= totalEffects) currentEffect = 0;

  lastActivityTime = millis(); // Start activity timer
}

void loop() {
  showIdleIndicator();

  if (digitalRead(BUTTON1_PIN) == LOW) {
    delay(400);
    currentEffect = (currentEffect + 1) % totalEffects;
    lastIdleEffect = -1;
    lastActivityTime = millis();
  }

  static bool wasHolding = false;
  bool isHolding = digitalRead(BUTTON2_PIN) == LOW;

  if (isHolding) {
    if (!wasHolding) {
      wasHolding = true;
      lastActivityTime = millis();
    }
    runEffectHold(currentEffect);
  } else if (wasHolding) {
    wasHolding = false;
    runEffectRelease(currentEffect);
    clearLeds();
    lastActivityTime = millis();
  }

  if (millis() - lastActivityTime > IDLE_TIMEOUT_MS) {
    EEPROM.update(EEPROM_ADDRESS, currentEffect);
    clearLeds();
    enterSleep();
  }
}

void showIdleIndicator() {
  if (currentEffect != lastIdleEffect) {
    CRGB color;
    switch (currentEffect) {
      case 0: color = CRGB::Blue; break;
      case 1: color = CRGB::Green; break;
      case 2: color = CRGB::Red; break;
      case 3: color = CRGB::Yellow; break;
      case 4: color = CRGB::Purple; break;
      case 5: color = CRGB::White; break;
      default: color = CRGB::White; break;
    }
    leds[0] = color;
    for (int i = 1; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    lastIdleEffect = currentEffect;
  }
}

void runEffectHold(int effectNumber) {
  switch (effectNumber) {
    case 0: solidColor(CRGB::Blue); break;
    case 1: colorWipe(CRGB::Green); break;
    case 2: fireEffect(); break;
    case 3: rainbowCycle(); break;
    case 4: purpleWipe(); break;
    case 5: meteorRain(0x80,0x00,0x80,2,64,true,25); break;
  }
}

void runEffectRelease(int effectNumber) {
  switch (effectNumber) {
    case 0: delay(100); break;
    case 1: delay(100); break;
    case 2: redFadeOut(); break;
    case 3: delay(100); break;
    case 4: fadeToBlack(CRGB::Purple); break;
    case 5: whiteFadeOut(); break;
  }
}

void solidColor(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(1000);
}

void colorWipe(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    FastLED.show();
    delay(50);
  }
}

void fireEffect() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t flicker = random8(120, 255);
    leds[i] = CRGB(flicker, flicker / 4, 0);
  }
  FastLED.show();
  delay(50);
}

void redFadeOut() {
  for (int b = 255; b >= 0; b -= 5) {
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, 0));
    FastLED.show();
    delay(30);
  }
}

void rainbowCycle() {
  static uint8_t hue = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue + (i * 10), 255, 255);
  }
  FastLED.show();
  hue++;
  delay(20);
}

void purpleWipe() {
  static int pos = 0;
  leds[pos] = CRGB::Purple;
  FastLED.show();
  leds[pos] = CRGB::Black;
  pos = (pos - 1) % NUM_LEDS;
  delay(40);
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
 
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
   
   
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
   
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      }
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}

void fadeToBlack(CRGB color) {
  for (int b = 255; b >= 0; b -= 5) {
    fill_solid(leds, NUM_LEDS, color);
    FastLED.setBrightness(b);
    FastLED.show();
    delay(20);
  }
  FastLED.setBrightness(BRIGHTNESS); // restore
}

void whiteFadeOut() {
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(300);
  fadeToBlack(CRGB::White);
}

void clearLeds() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  lastIdleEffect = -1;
}

void enterSleep() {
  GIMSK |= _BV(PCIE);
  PCMSK |= _BV(PCINT3) | _BV(PCINT4);
  ADCSRA &= ~(1 << ADEN);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  cli(); sei();
  sleep_cpu();
  sleep_disable();
  ADCSRA |= (1 << ADEN);
  lastActivityTime = millis();
}

ISR(PCINT0_vect) {
  // Wake from sleep
}
