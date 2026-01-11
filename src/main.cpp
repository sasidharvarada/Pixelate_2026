#include <Arduino.h>
#include <FastLED.h>

/* ================= LED CONFIG ================= */
#define LED_PIN 14
#define CHIPSET WS2812B
#define COLOR_ORDER GRB

#define MATRIX_W 19
#define MATRIX_H 19
#define NUM_LEDS (MATRIX_W * MATRIX_H)
#define BRIGHTNESS 120

CRGB leds[NUM_LEDS];

/* ================= MATRIX MAP ================= */
uint16_t XY(uint8_t x, uint8_t y) {
  if (y % 2 == 0) return y * MATRIX_W + x;
  return y * MATRIX_W + (MATRIX_W - 1 - x);
}

/* ================= GAME STATES ================= */
enum GameState { IDLE, PLAYING, GAME_OVER };
GameState state = IDLE;

/* ================= GRID ================= */
#define GRID_W 10
#define GRID_H 18
#define GX 4
#define GY 1

uint8_t grid[GRID_H][GRID_W] = {0};
int score = 0;

/* ================= PIECES ================= */
struct Piece {
  uint8_t s[4][4];
  uint8_t w, h;
  CRGB c;
};

Piece pieces[4] = {
  {{{1,1,0,0},{1,1,0,0}},2,2,CRGB::Yellow},
  {{{1,0,0,0},{1,0,0,0},{1,1,0,0}},2,3,CRGB::Orange},
  {{{1,1,0,0},{0,1,1,0}},3,2,CRGB::Red},
  {{{0,1,0,0},{1,1,1,0}},3,2,CRGB::Purple}
};

int curPiece, px, py;
unsigned long gameStartTime;

/* ================= HELPERS ================= */
void clearMatrix() { FastLED.clear(); }

void drawCell(int x, int y, CRGB c) {
  leds[XY(GX + x, GY + y)] = c;
}

/* ================= VISUAL GLAMOUR ================= */
void drawBorder() {
  CRGB bc = CRGB(40, 40, 40);
  for (int y = 0; y < GRID_H; y++) {
    leds[XY(GX - 1, GY + y)] = bc;
    leds[XY(GX + GRID_W, GY + y)] = bc;
  }
  for (int x = 0; x < GRID_W; x++) {
    leds[XY(GX + x, GY - 1)] = bc;
    leds[XY(GX + x, GY + GRID_H)] = bc;
  }
}

void drawBackgroundGrid() {
  CRGB bg = CRGB(3, 3, 3);
  for (int y = 0; y < GRID_H; y++)
    for (int x = 0; x < GRID_W; x++)
      if (!grid[y][x])
        leds[XY(GX + x, GY + y)] += bg;
}

/* ================= COLLISION ================= */
bool collide(int nx, int ny) {
  Piece &p = pieces[curPiece];
  for (int y = 0; y < p.h; y++)
    for (int x = 0; x < p.w; x++)
      if (p.s[y][x]) {
        int gx = nx + x;
        int gy = ny + y;
        if (gx < 0 || gx >= GRID_W || gy >= GRID_H) return true;
        if (gy >= 0 && grid[gy][gx]) return true;
      }
  return false;
}

/* ================= GAME OPS ================= */
void flashLine(int row) {
  for (int i = 0; i < 2; i++) {
    for (int x = 0; x < GRID_W; x++)
      drawCell(x, row, CRGB::White);
    FastLED.show();
    delay(40);
    clearMatrix();
    FastLED.show();
    delay(40);
  }
}

void lockPiece() {
  Piece &p = pieces[curPiece];
  for (int y = 0; y < p.h; y++)
    for (int x = 0; x < p.w; x++)
      if (p.s[y][x])
        grid[py + y][px + x] = 1;
}

void clearLines() {
  for (int y = GRID_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < GRID_W; x++)
      if (!grid[y][x]) full = false;

    if (full) {
      flashLine(y);
      score++;
      Serial.print("SCORE:");
      Serial.println(score);

      for (int ty = y; ty > 0; ty--)
        for (int x = 0; x < GRID_W; x++)
          grid[ty][x] = grid[ty - 1][x];
      memset(grid[0], 0, GRID_W);
      y++;
    }
  }
}

void spawnPiece() {
  curPiece = random(0, 4);
  px = (GRID_W - pieces[curPiece].w) / 2;
  py = 0;
}

/* ================= DRAW ================= */
void drawGrid() {
  for (int y = 0; y < GRID_H; y++)
    for (int x = 0; x < GRID_W; x++)
      if (grid[y][x]) drawCell(x, y, CRGB::Blue);
}

void drawPiece() {
  Piece &p = pieces[curPiece];
  for (int y = 0; y < p.h; y++) {
    for (int x = 0; x < p.w; x++) {
      if (!p.s[y][x]) continue;

      int gx = px + x;
      int gy = py + y;

      drawCell(gx, gy, p.c);

      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++) {
          int nx = gx + dx;
          int ny = gy + dy;
          if (nx >= 0 && nx < GRID_W && ny >= 0 && ny < GRID_H)
            leds[XY(GX + nx, GY + ny)] += p.c / 6;
        }
    }
  }
}

/* ================= GAME OVER EFFECT ================= */
void gameOverEffect() {
  for (int b = 0; b <= 255; b += 5) {
    for (int i = 0; i < NUM_LEDS; i++)
      leds[i] = CRGB(b, 0, 0);
    FastLED.show();
    delay(10);
  }
}

/* ================= SERIAL INPUT ================= */
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'L' && !collide(px - 1, py)) px--;
    if (c == 'R' && !collide(px + 1, py)) px++;
    if (c == 'D' && !collide(px, py + 1)) py++;
    if (c == 'S') {
      memset(grid, 0, sizeof(grid));
      score = 0;
      spawnPiece();
      gameStartTime = millis();
      state = PLAYING;
    }
  }
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(true);
  FastLED.setCorrection(TypicalLEDStrip);
  randomSeed(millis());
}

/* ================= LOOP ================= */
unsigned long lastFall = 0;

void loop() {
  handleSerial();

  if (state == PLAYING) {
    if (millis() - gameStartTime > 30000) {
      Serial.print("GAME_OVER:");
      Serial.println(score);
      gameOverEffect();
      state = GAME_OVER;
    }

    if (millis() - lastFall > 500) {
      if (!collide(px, py + 1)) py++;
      else {
        lockPiece();
        clearLines();
        spawnPiece();
      }
      lastFall = millis();
    }

    clearMatrix();
    drawBackgroundGrid();
    drawBorder();
    drawGrid();
    drawPiece();
    FastLED.show();
  }
}
