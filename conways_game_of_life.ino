#include <Wire.h>                 // For I2C communication
#include <Adafruit_LIS3DH.h>      // For accelerometer
#include <Adafruit_PixelDust.h>   // For sand simulation
#include <Adafruit_Protomatter.h> // For RGB matrix

#define HEIGHT  64 // Matrix height (pixels) - SET TO 64 FOR 64x64 MATRIX!
#define WIDTH   64 // Matrix width (pixels)
#define MAX_FPS 45 // Maximum redraw rate, frames/second

#if defined(_VARIANT_MATRIXPORTAL_M4_) // MatrixPortal M4
uint8_t rgbPins[]  = {7, 8, 9, 10, 11, 12};
uint8_t addrPins[] = {17, 18, 19, 20, 21};
uint8_t clockPin   = 14;
uint8_t latchPin   = 15;
uint8_t oePin      = 16;
#else // MatrixPortal ESP32-S3
uint8_t rgbPins[]  = {42, 41, 40, 38, 39, 37};
uint8_t addrPins[] = {45, 36, 48, 35, 21};
uint8_t clockPin   = 2;
uint8_t latchPin   = 47;
uint8_t oePin      = 14;
#endif

#if HEIGHT == 16
#define NUM_ADDR_PINS 3
#elif HEIGHT == 32
#define NUM_ADDR_PINS 4
#elif HEIGHT == 64
#define NUM_ADDR_PINS 5
#endif

Adafruit_Protomatter matrix(
  WIDTH, 4, 1, rgbPins, NUM_ADDR_PINS, addrPins,
  clockPin, latchPin, oePin, true);

int old_mat[HEIGHT * WIDTH];
int new_mat[HEIGHT * WIDTH];
//int populated[HEIGHT*WIDTH];
int states[7][HEIGHT*WIDTH];

#define ARBITRARY_CONSTANT 50

#define N_COLORS 8
uint16_t colors[N_COLORS];
uint16_t CUR_COLOR;

int cur_state_index = 0;

void init_states() {
  for (size_t i = 0; i < 7; i++) {
    for (size_t row = 0; row < HEIGHT; row++) {
      for (size_t col = 0; col < WIDTH; col++) {
        states[i][row*HEIGHT+col] = 0;
      }
    }
  }
}


int check_equal() {
  for (size_t i = 0; i < 7; i++) {
    int eq = 1;
    for (size_t row = 0; row < HEIGHT; row++) {
      for (size_t col = 0; col < WIDTH; col++) {
        if (states[i][row*HEIGHT+col] != old_mat[row*HEIGHT+col]) {
          eq = 0;
          break;
        }
      }
      if (!eq) break;
    }
    if (eq) return 1;
  }
  return 0;
}


struct Pixel {
  int x;
  int y;
};

//int arbitary_check() {
//  int arb = 0;
//  for (size_t i = 0; i < HEIGHT; i++) {
//    for (size_t j = 0; j < WIDTH; j++) {
//      if (populated[i*HEIGHT+j] >= 400) {
//        arb = 1;
//        break;
//      }
//    }
//  }
//  return arb;
//}

int color_idx = 0;
void change_cur_color() {
  CUR_COLOR = colors[color_idx++];
  if (color_idx == 8) color_idx = 0;
}
//void clear_populated() {
//  for (size_t i = 0; i < HEIGHT; i++) {
//    for (size_t j = 0; j < WIDTH; j++) {
//      populated[i*HEIGHT+j] = 0;
//    }
//  }
//}

// on init
void draw_conway() {
  struct Pixel pixels[] = {{32,matrix.height()-4},{32, matrix.height()-5},{32, matrix.height() - 8},{32,matrix.height()-10},{32,matrix.height()-11},{32,matrix.height()-12},
  {31,matrix.height()-7},
  {30,matrix.height()-8},
  {33,matrix.height()-6},{33,matrix.height()-7},{33,matrix.height()-8},{33,matrix.height()-9},{33,matrix.height()-12},
  {34,matrix.height()-4},{34,matrix.height()-5},{34,matrix.height()-8},{34,matrix.height()-10},{34,matrix.height()-11},
  {35,matrix.height()-7},
  {36,matrix.height()-6}};
  size_t size = 20;
  for (size_t i = 0; i < size; i++) {
    matrix.drawPixel(pixels[i].x, pixels[i].y, CUR_COLOR);
    old_mat[pixels[i].x * HEIGHT + pixels[i].y] = 1;
  }
}




int play_life(int arg) {
  int neighbors[] = {0,1,-1};
  for (size_t row = 0; row < HEIGHT; row++) {
    for (size_t col = 0; col < WIDTH; col++) {
      new_mat[row * HEIGHT + col] = old_mat[row * HEIGHT + col];
    }
  }

  for (size_t row = 0; row < HEIGHT; row++) {
    for (size_t col = 0; col < WIDTH; col++) {
      int live = 0;
      for (size_t i = 0; i < 3; i++) {
        for (size_t j = 0; j < 3; j++) {
          if (!(neighbors[i] == 0 && neighbors[j] == 0)) {
            int r = (row + neighbors[i]);
            int c = (col + neighbors[j]);
            if ((r < HEIGHT && r >= 0) && (c < WIDTH && c >= 0) && (new_mat[r * HEIGHT + c] == 1)) {
              live += 1;
            }
          }
        }
      }
      if ((new_mat[row * HEIGHT + col] == 1) && (live < 2 || live > 3)) {
        old_mat[row * HEIGHT + col] = 0;
      }
      if (new_mat[row*HEIGHT + col] == 0 && live == 3) {
        old_mat[row*HEIGHT + col] = 1;
//        populated[row*HEIGHT + col] += 1;
      }
    }
  }
  // check equal here
  int eq = check_equal();
  if (eq) return eq;
  // set cur state index to old mat
  for (size_t row = 0; row < HEIGHT; row++) {
    for (size_t col = 0; col < WIDTH; col++) {
      states[cur_state_index][row*HEIGHT+col] = old_mat[row*HEIGHT+col];
    }
  }
  cur_state_index += 1;
  if (cur_state_index == 7) cur_state_index = 0;
  return 0;
}

void update() {
  for (size_t i = 0; i < HEIGHT; i++) {
    for (size_t j = 0; j < WIDTH; j++) {
      if (old_mat[i*HEIGHT+ j] == 1) {
        matrix.drawPixel(i, j, CUR_COLOR);
      } else {
        matrix.drawPixel(i, j, matrix.color565(0, 0, 0));
      }
    }
  }
}


void randomize() {
  for (size_t i = 0; i < HEIGHT; i++) {
    for (size_t j = 0; j < WIDTH; j++) {
       int val = rand() % 100;
       if (val <= 60) old_mat[i*HEIGHT +j] = 1;
       else old_mat[i*HEIGHT + j] = 0;
    }
  }
  for (size_t i = 0; i < HEIGHT; i++) {
    for (size_t j = 0; j < WIDTH; j++) {
      if (old_mat[i*HEIGHT+j] == 1) {
        matrix.drawPixel(i, j, CUR_COLOR);
      } else {
        matrix.drawPixel(i, j, matrix.color565(0,0,0));
      }
    }
  }
}

void setup(void) {
  Serial.begin(9600);

  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    for(;;);
  }

  colors[0] = matrix.color565(64, 64, 64);  // Dark Gray
  colors[1] = matrix.color565(120, 79, 23); // Brown
  colors[2] = matrix.color565(228,  3,  3); // Red
  colors[3] = matrix.color565(255,140,  0); // Orange
  colors[4] = matrix.color565(255,237,  0); // Yellow
  colors[5] = matrix.color565(  0,128, 38); // Green
  colors[6] = matrix.color565(  0, 77,255); // Blue
  colors[7] = matrix.color565(117,  7,135); // Purple

  CUR_COLOR = matrix.color565(255, 255, 255); // White
  
  for (size_t i = 0; i < HEIGHT; i++) {
    for (size_t j = 0; j < WIDTH; j++) {
      old_mat[i * WIDTH + j] = 0;
      new_mat[i * WIDTH + j] = 0;
//      populated[i*WIDTH + j] = 0;
    }
  }
  draw_conway();
  matrix.show(); // Copy data to matrix buffers
  delay(500);
}
void loop(void) {
  Serial.print("Refresh FPS = ~");
  Serial.println(matrix.getFrameCount());
  while (1) {
    int czech = play_life(0);
    update();
    matrix.show();
    if (czech) break;
  }
//  clear_populated();
  init_states();
  change_cur_color();
  randomize();
}
