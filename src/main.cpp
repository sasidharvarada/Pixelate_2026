#include <Arduino.h>
#include <FastLED.h>

// LED matrix configuration
#define LED_PIN     6
#define WIDTH       32
#define HEIGHT      32
#define NUM_LEDS    (WIDTH * HEIGHT)
#define COLOR_ORDER GRB
#define CHIPSET     WS2812B
#define BRIGHTNESS  120

CRGB leds[NUM_LEDS];

// Convert x,y to linear index for a serpentine 32x32 matrix
uint16_t XY(uint8_t x, uint8_t y) {
  if (x >= WIDTH) x = WIDTH - 1;
  if (y >= HEIGHT) y = HEIGHT - 1;
  uint16_t i;
  if (y % 2 == 0) {
    i = y * WIDTH + x;
  } else {
    i = y * WIDTH + (WIDTH - 1 - x);
  }
  return i;
}

inline void setPixel(int x, int y, const CRGB &c) {
  if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
  leds[XY(x, y)] = c;
}

// Bresenham line (integer)
void drawLine(int x0, int y0, int x1, int y1, const CRGB &c) {
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    setPixel(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// Filled ellipse rasterization for simple eye shapes
void drawFilledEllipse(int cx, int cy, int rx, int ry, const CRGB &c) {
  for (int y = cy - ry; y <= cy + ry; ++y) {
    for (int x = cx - rx; x <= cx + rx; ++x) {
      float nx = (float)(x - cx) / (float)rx;
      float ny = (float)(y - cy) / (float)ry;
      if (nx * nx + ny * ny <= 1.0f) setPixel(x, y, c);
    }
  }
}

// Draw a simple stylized Spiderman face/logo centered on 32x32
void drawSpideyLogo() {
  // Base: full red background
  for (int i = 0; i < NUM_LEDS; ++i) leds[i] = CRGB::Red;

  // Draw eyes (two white ellipses)
  drawFilledEllipse(10, 13, 6, 9, CRGB::White);
  drawFilledEllipse(21, 13, 6, 9, CRGB::White);

  // Eye outlines
  drawLine(6, 9, 14, 9, CRGB::Black);
  drawLine(6, 9, 8, 15, CRGB::Black);
  drawLine(26, 9, 18, 9, CRGB::Black);
  drawLine(26, 9, 24, 15, CRGB::Black);

  // Spider body (vertical)
  for (int y = 14; y <= 23; ++y) setPixel(16, y, CRGB::Black);

  // Spider legs (simple stylized lines)
  drawLine(16, 16, 10, 10, CRGB::Black);
  drawLine(16, 16, 22, 10, CRGB::Black);
  drawLine(16, 18, 8, 14, CRGB::Black);
  drawLine(16, 18, 24, 14, CRGB::Black);
  drawLine(16, 20, 7, 20, CRGB::Black);
  drawLine(16, 20, 25, 20, CRGB::Black);

  // Add subtle webbing lines (curved-ish using short segments)
  for (int i = 0; i < 6; ++i) {
    int y = 8 + i * 3;
    drawLine(6 + i, y, 26 - i, y, CRGB::DarkRed);
  }
}

void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  // Pulse brightness and redraw logo
  uint8_t pulse = beatsin8(10, 40, BRIGHTNESS);
  FastLED.setBrightness(pulse);
  drawSpideyLogo();
  FastLED.show();
  delay(30);
}