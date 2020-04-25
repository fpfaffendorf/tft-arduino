/* 
 * TFT
 *  
 * Notes: 
 * 
 * - Note is a string indicating note, sustain and octave, something like C#3 while pitch is the MIDI note number.
 *
 */

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <avr/wdt.h>

#include "tft_ili9163c.h" 

#define COLOR_EMPTY                       0x0000 /* -- */
#define COLOR_UP                          0x001F /*  | */
#define COLOR_DOWN                        0xF800 /*  | */
#define COLOR_LEFT                        0x07E0 /*  | */
#define COLOR_RIGHT                       0x07FF /*  | */
#define COLOR_WALL                        0xFFE0 /*  | */ 
#define COLOR_MIRROR_1                    0xF81F /*  | Do not repeat colors */
#define COLOR_MIRROR_2                    0xAC12 /*  | */
#define COLOR_BORDER                      0xCCCC /*  | */
#define COLOR_TEXT_1                      0xFFFF /*  | */
#define COLOR_TEXT_2                      0xEEEE /*  | */
#define COLOR_SELECT                      0x07E0 /*  | */
#define COLOR_EDIT                        0x07FF /*  | */
#define COLOR_NONE                        0x0001 /* -- */
#define PIN_CS                            10
#define PIN_A0                            8
#define PIN_DC                            9
#define PIN_BTN                           5                           /*  | This 2 should match */
#define GPIO_BTN                          ((PIND & B00100000) == 0)   /* -| */
#define PIN_CW                            3                           /*  | This 2 should match */
#define GPIO_CW                           ((PIND & B00001000) == 0)   /* -| */
#define PIN_CWW                           2                           /*  | This 2 should match */
#define GPIO_CWW                          ((PIND & B00000100) == 0)   /* -| */
#define MATRIX_WIDTH                      8
#define MATRIX_HEIGHT                     8
#define MAX_CELLS                         16
#define CELL_PIXELS                       8
#define BAUD_RATES                        115200
#define SCREEN_WIDTH                      128
#define SCREEN_HEIGHT                     128
#define BUTTON_DEBOUNCE                   500 /* milliseconds */
#define OPTION_CELL_ID                    0
#define OPTION_CELL_X                     1
#define OPTION_CELL_TYPE                  2
#define OPTION_CELL_Y                     3
#define OPTION_CELL_OK                    4
#define OPTION_PITCH_X                    5
#define OPTION_MODE_X                     6
#define OPTION_ORDER_X                    7
#define OPTION_PITCH_Y                    8
#define OPTION_MODE_Y                     9
#define OPTION_ORDER_Y                    10
#define OPTION_BPM                        11
#define OPTION_SCREEN                     12
#define OPTION_CHANNEL                    13
#define OPTION_VELOCITY                   14
#define OPTION_DURATION                   15
#define OPTION_COUNT                      16 /* Number of options */
#define DEFAULT_BPM                       120.0 /* Kepp BPM float as it participates in divisions where float point matters */
#define MODE_IONIAN                       0
#define MODE_DORIAN                       1
#define MODE_PHRYGIAN                     2
#define MODE_LYDIAN                       3
#define MODE_MIXOLYDIAN                   4
#define MODE_AEOLIAN                      5
#define MODE_LOCRIAN                      6
#define ORDER_DOWN                        0
#define ORDER_UP                          1
#define ORDER_RANDOM                      2
#define ORDER_OFF                         3
#define MAX_SCREENS                       3
#define ENCODER_TIMEOUT                   250 /* milliseconds */
#define ENCODER_INTERRUPT_DELAY           20  /* milliseconds */
#define REBOOT_TIMEOUT                    10000 /* milliseconds */

TFT_ILI9163C display = TFT_ILI9163C(PIN_CS, PIN_A0, PIN_DC);

unsigned long previousLoopMillis = 0;
byte redrawScreenStep = 0;

volatile int encoderTurning = 0;
volatile int encoderClicks = 0;  
volatile int encoderDirection = 0;
volatile unsigned long encoderLastMillis = 0;
unsigned long buttonLastMillis = 0;
bool buttonCurrentStatus = false;

byte pitchX[MAX_SCREENS] = { 60, 60, 60 };
byte modeX[MAX_SCREENS] = { MODE_IONIAN, MODE_IONIAN, MODE_IONIAN };
byte orderX[MAX_SCREENS] = { ORDER_UP, ORDER_UP, ORDER_UP };

byte pitchY[MAX_SCREENS] = { 72, 72, 72 };
byte modeY[MAX_SCREENS] = { MODE_IONIAN, MODE_IONIAN, MODE_IONIAN };
byte orderY[MAX_SCREENS] = { ORDER_UP, ORDER_UP, ORDER_UP };

float bpm = DEFAULT_BPM; 
byte currentScreen = 0;

byte channel[MAX_SCREENS] = { 0, 0, 0 };
byte velocity[MAX_SCREENS] = { 127, 127, 127 };
float duration[MAX_SCREENS] = { 1, 1, 1 };

byte currentOption = OPTION_BPM;
char currentOptionMode = 'S';

uint16_t previousMatrix[MATRIX_HEIGHT][MATRIX_WIDTH] = { 
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE},
  {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE}
};

uint16_t matrix[MATRIX_HEIGHT][MATRIX_WIDTH] = { 
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY},
  {COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY, COLOR_EMPTY}
};

typedef struct {
  byte x;
  byte y;
  uint16_t type;
} cell;

byte currentCellIndex = 0;
cell previousCurrentCell;
cell currentCell;
cell cells[MAX_SCREENS][MAX_CELLS]; /* Initialized in setup function */

typedef struct {
  byte channel;
  byte pitch;
  byte velocity;
  unsigned long millisOff;
  bool onMessageSent;
} pitchOnOff;

byte pitchesX[MAX_SCREENS][MATRIX_WIDTH];
byte pitchesY[MAX_SCREENS][MATRIX_HEIGHT];
pitchOnOff pitchesOnOff[(MATRIX_WIDTH + MATRIX_HEIGHT) * MAX_SCREENS]; /* Initialized in setup function */

void setup(void) {

  /* Serial initialization */
  Serial.begin(BAUD_RATES);

  /* Cells initialize */
  for(byte s = 0; s < MAX_SCREENS; s++) 
    for(byte c = 0; c < MAX_CELLS; c++) 
      cells[s][c] = {0, 0, COLOR_EMPTY};

  /* Pitches On/Off initialize */
  for(byte x = 0; x < (MATRIX_WIDTH + MATRIX_HEIGHT) * MAX_SCREENS; x++) 
    pitchesOnOff[x] = { 0, 0, false };

  /* Display initialization */
  display.begin();
  display.clearScreen();
  display.setRotation(3);
  display.scroll(0);
  display.setTextSize(1);

  /* Encoder button */
  pinMode(PIN_BTN, INPUT_PULLUP);

  /* Encoder rotation */
  digitalWrite(PIN_CW, LOW); /* Turn off PWM Timer */
  digitalWrite(PIN_CWW, LOW); /* Turn off PWM Timer */
  pinMode(PIN_CW, INPUT_PULLUP);
  pinMode(PIN_CWW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CW), EncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CWW), EncoderInterrupt, CHANGE);

  /* Random initialization */
  randomSeed(analogRead(0));
 
  /* Grid */
  display.drawRect(1, 1, (MATRIX_WIDTH * CELL_PIXELS) + 2, (MATRIX_HEIGHT * CELL_PIXELS) + 2, COLOR_WALL);
  for(byte s = 0; s < MAX_SCREENS; s++) {
    processMode(pitchesX[s], 'X');
    processMode(pitchesY[s], 'Y');    
  }
  printNotesX();
  printNotesY();

  /* Mode X and Y */
  display.setTextColor(COLOR_TEXT_1);  
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 2, 22, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 51, 2, 9, 11, COLOR_BORDER); 
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 12, 30, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 22, 30, 11, COLOR_BORDER);
  display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 53, 3); display.print("x");    
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 38, 22, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 51, 38, 9, 11, COLOR_BORDER); 
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 48, 30, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 30, 58, 30, 11, COLOR_BORDER);
  display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 53, 39); display.print("y");    
  printOption(OPTION_PITCH_X, 'U');
  printOption(OPTION_MODE_X, 'U');
  printOption(OPTION_ORDER_X, 'U');
  printOption(OPTION_PITCH_Y, 'U');
  printOption(OPTION_MODE_Y, 'U');
  printOption(OPTION_ORDER_Y, 'U');

  /* Info */
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 10, (MATRIX_HEIGHT * CELL_PIXELS) + 10, 51, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 10, (MATRIX_HEIGHT * CELL_PIXELS) + 20, 51, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 10, (MATRIX_HEIGHT * CELL_PIXELS) + 30, 51, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 10, (MATRIX_HEIGHT * CELL_PIXELS) + 40, 51, 11, COLOR_BORDER);
  display.drawRect((MATRIX_WIDTH * CELL_PIXELS) + 10, (MATRIX_HEIGHT * CELL_PIXELS) + 50, 51, 11, COLOR_BORDER);
  printOption(OPTION_BPM, 'S');
  printOption(OPTION_SCREEN, 'U');
  printOption(OPTION_CHANNEL, 'U');
  printOption(OPTION_VELOCITY, 'U');
  printOption(OPTION_DURATION, 'U');

  /* Cell */
  display.setTextColor(COLOR_TEXT_1);  
  display.drawRect(2, (MATRIX_HEIGHT * CELL_PIXELS) + 40, 22, 11, COLOR_BORDER);
  display.drawRect(23, (MATRIX_HEIGHT * CELL_PIXELS) + 40, 22, 11, COLOR_BORDER);
  display.drawRect(2, (MATRIX_HEIGHT * CELL_PIXELS) + 50, 22, 11, COLOR_BORDER);
  display.drawRect(23, (MATRIX_HEIGHT * CELL_PIXELS) + 50, 22, 11, COLOR_BORDER);
  display.drawRect(46, (MATRIX_HEIGHT * CELL_PIXELS) + 40, 22, 21, COLOR_BORDER);
  printOption(OPTION_CELL_ID, 'U');
  printOption(OPTION_CELL_TYPE, 'U');
  printOption(OPTION_CELL_X, 'U');
  printOption(OPTION_CELL_Y, 'U');
  printOption(OPTION_CELL_OK, 'U');

  EncoderInterrupt(); /* Keep this call here, otherwise the encoder behaves funny first time it is used */

}

void loop() {
  processPitchesOnOff();
  processEncoder();
  processEncoderButton();
  redrawScreen();
  unsigned long loopMillis = millis();
  /* Encoder timeout, reset direction */
  if ((encoderDirection != 0) && (loopMillis > encoderLastMillis + ENCODER_TIMEOUT)) { cli(); encoderDirection = 0; sei(); }
  /* Bit timeout, process */
  if (previousLoopMillis + (1000 / (bpm / 60)) <= loopMillis ) {
    previousLoopMillis = loopMillis;
    /* Process all screens */
    for(byte s = 0; s < MAX_SCREENS; s++) {
      /* Process movable cells first */
      processMovable(s);  
      processPitchesOnOff();    
      /* Process collisions */
      processCollisions(s);  
      /* Current screen only if encoder rotation is quiet */
      if((currentScreen == s) && (encoderClicks == 0)) {
        /* Process non movable cells in second place to keep them on top so they hide movable cells on a collision */
        processNonMovable();
        /* Render matrix for current screen only */
        renderMatrix();
        /* Focus current cell */
        focusCurrentCell();
      }
    }
  }
}

void pitchToNote(byte pitch, char* note, char* hash, char* octave) {
  /* Out of range */
  if ((pitch < 12) || (pitch > 119)) { *note = ' '; *hash = ' '; *octave = ' '; return; }
  /* Set octave */
  for(byte i = 0; i <= 8; i ++)
    if ((pitch >= 12 * (i + 1)) && (pitch <= 12 * (i + 1) + 11)) *octave = i;
  /* Set note */
  byte octave0Pitch = pitch - (*octave * 12); /* Shift note to octave 0 */
  if ((octave0Pitch == 12) || (octave0Pitch == 13)) *note = 'C';
  else if ((octave0Pitch == 14) || (octave0Pitch == 15)) *note = 'D';
  else if (octave0Pitch == 16) *note = 'E';
  else if ((octave0Pitch == 17) || (octave0Pitch == 18)) *note = 'F';
  else if ((octave0Pitch == 19) || (octave0Pitch == 20)) *note = 'G';
  else if ((octave0Pitch == 21) || (octave0Pitch == 22)) *note = 'A';
  else if (octave0Pitch == 23) *note = 'B';
  /* Set hash */
  *hash = ' ';
  if ((octave0Pitch == 13) || (octave0Pitch == 15) || (octave0Pitch == 18) || (octave0Pitch == 20) || (octave0Pitch == 22)) *hash = '#';
  /* Final */
  *octave += 48; /* Convert to printable char */
}

void printPitchToNote(byte pitch) {
  char note, hash, octave;
  pitchToNote(pitch, &note, &hash, &octave);
  display.print(note);    
  if (hash != ' ') display.print(hash);    
  display.print(octave);    
}

void printNotesY(void) {
  /* Clean */
  display.fillRect((MATRIX_WIDTH * CELL_PIXELS) + 3, 1, 26, (MATRIX_HEIGHT * CELL_PIXELS) + 3, COLOR_EMPTY);
  /* Print */
  display.setTextColor(COLOR_TEXT_2);  
  for (byte i = 0; i < 8; i++) {
    if(i % 2 == 0) processPitchesOnOff(); /* Process pitches meanwhile */
    display.setCursor(71, 2 + (i * 8));  
    printPitchToNote(pitchesY[currentScreen][i]);
  }
}

void printNotesX(void) {
  /* Clean */
  display.fillRect(0, (MATRIX_HEIGHT * CELL_PIXELS) + 3, (MATRIX_WIDTH * CELL_PIXELS) + 3, 26, COLOR_EMPTY);
  /* Print */
  char note, hash, octave;
  display.setTextColor(COLOR_TEXT_2);  
  for (byte i = 0; i < 8; i++) {
    if(i % 2 == 0) processPitchesOnOff(); /* Process pitches meanwhile */
    pitchToNote(pitchesX[currentScreen][i], &note, &hash, &octave);
    display.setCursor(3 + (i * 8), 70); display.print(note);    
    if (hash != ' ') { display.setCursor(3 + (i * 8), 78); display.print(hash); }
    display.setCursor(3 + (i * 8), (hash != ' ') ? 86 : 78); display.print(octave);
  }
}

void printModeToString(byte mode) {
  if (mode == MODE_IONIAN) display.print("ION");
  else if (mode == MODE_DORIAN) display.print("DOR");
  else if (mode == MODE_PHRYGIAN) display.print("PHR");
  else if (mode == MODE_LYDIAN) display.print("LYD");
  else if (mode == MODE_MIXOLYDIAN) display.print("MIX");
  else if (mode == MODE_AEOLIAN) display.print("AEO");
  else if (mode == MODE_LOCRIAN) display.print("LOC");
}

void printOrderToString(byte order) {
  if (order == ORDER_DOWN) display.print("Down");
  else if (order == ORDER_UP) display.print("Up"); 
  else if (order == ORDER_RANDOM) display.print("Rand"); 
  else if (order == ORDER_OFF) display.print("Off"); 
}

void printOption(byte option, char mode) {
  int color = (mode == 'S') ? COLOR_SELECT : (mode == 'U') ? COLOR_EMPTY : COLOR_EDIT;
  if ((option >= OPTION_PITCH_X) && (option <= OPTION_ORDER_X)) goto first_area;
  else if ((option >= OPTION_PITCH_Y) && (option <= OPTION_ORDER_Y)) goto second_area;
  else if ((option >= OPTION_BPM) && (option <= OPTION_DURATION)) goto third_area;
  else if ((option >= OPTION_CELL_ID) && (option <= OPTION_CELL_OK)) goto fourth_area;
  else return;
  first_area:
    display.fillRect((MATRIX_WIDTH * CELL_PIXELS) + 31, 3 + (10 * (option - OPTION_PITCH_X)), (option == OPTION_PITCH_X) ? 20 : 28, 9, color);
    display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
    display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 32, 4 + (10 * (option - OPTION_PITCH_X)));  
    if (option == OPTION_PITCH_X) printPitchToNote(pitchX[currentScreen]);
    else if (option == OPTION_MODE_X) printModeToString(modeX[currentScreen]);
    else if (option == OPTION_ORDER_X) printOrderToString(orderX[currentScreen]);
    return;
  second_area:
    display.fillRect((MATRIX_WIDTH * CELL_PIXELS) + 31, 39 + (10 * (option - OPTION_PITCH_Y)), (option == OPTION_PITCH_Y) ? 20 : 28, 9, color);
    display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
    display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 32, 40 + (10 * (option - OPTION_PITCH_Y)));  
    if (option == OPTION_PITCH_Y) printPitchToNote(pitchY[currentScreen]);
    else if (option == OPTION_MODE_Y) printModeToString(modeY[currentScreen]);
    else if (option == OPTION_ORDER_Y) printOrderToString(orderY[currentScreen]);
    return;
  third_area:
    display.fillRect((MATRIX_WIDTH * CELL_PIXELS) + 11, (MATRIX_HEIGHT * CELL_PIXELS) + 11 + (10 * (option - OPTION_BPM)), 49, 9, color);
    display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_1);  
    display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 12, (MATRIX_HEIGHT * CELL_PIXELS) + 12 + (10 * (option - OPTION_BPM)));  
    if (option == OPTION_BPM) display.print("BPM");    
    else if (option == OPTION_SCREEN) display.print("SCR");    
    else if (option == OPTION_CHANNEL) display.print("CH");    
    else if (option == OPTION_VELOCITY) display.print("VEL");    
    else if (option == OPTION_DURATION) display.print("DUR");    
    display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
    display.setCursor((MATRIX_WIDTH * CELL_PIXELS) + 42, (MATRIX_HEIGHT * CELL_PIXELS) + 12 + (10 * (option - OPTION_BPM)));  
    if (option == OPTION_BPM) display.print((int)bpm);    
    else if (option == OPTION_SCREEN) display.print((int)currentScreen + 1);    
    else if (option == OPTION_CHANNEL) display.print((int)channel[currentScreen] + 1);    
    else if (option == OPTION_VELOCITY) display.print((int)velocity[currentScreen]);    
    else if (option == OPTION_DURATION) display.print(duration[currentScreen], 1);    
    return;
  fourth_area:
    if (option == OPTION_CELL_ID) {
      display.fillRect(3, (MATRIX_HEIGHT * CELL_PIXELS) + 41, 20, 9, color);
      display.setCursor(5, (MATRIX_HEIGHT * CELL_PIXELS) + 41); 
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_1);  
      display.print("c");    
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
      display.setCursor(15, (MATRIX_HEIGHT * CELL_PIXELS) + 42);
      display.print((char)(currentCellIndex + 0x41));
    }
    else if (option == OPTION_CELL_TYPE) {
      display.fillRect(24, (MATRIX_HEIGHT * CELL_PIXELS) + 41, 20, 9, color);
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
      display.setCursor(25, (MATRIX_HEIGHT * CELL_PIXELS) + 42);
      printCellTypeToString(currentCell.type);
    }
    else if (option == OPTION_CELL_X) {
      display.fillRect(3, (MATRIX_HEIGHT * CELL_PIXELS) + 51, 20, 9, color);
      display.setCursor(5, (MATRIX_HEIGHT * CELL_PIXELS) + 51); 
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_1);  
      display.print("x");    
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
      display.setCursor(15, (MATRIX_HEIGHT * CELL_PIXELS) + 52);
      display.print((int)currentCell.x + 1);
    }
    else if (option == OPTION_CELL_Y) {
      display.fillRect(24, (MATRIX_HEIGHT * CELL_PIXELS) + 51, 20, 9, color);
      display.setCursor(25, (MATRIX_HEIGHT * CELL_PIXELS) + 51); 
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_1);  
      display.print("y");    
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_2);  
      display.setCursor(36, (MATRIX_HEIGHT * CELL_PIXELS) + 52);
      display.print((int)currentCell.y + 1);
    }
    else if (option == OPTION_CELL_OK) {
      display.fillRect(47, (MATRIX_HEIGHT * CELL_PIXELS) + 41, 20, 19, color);
      display.setCursor(51, (MATRIX_HEIGHT * CELL_PIXELS) + 47); 
      display.setTextColor(((mode == 'S') || (mode == 'E')) ? COLOR_EMPTY : COLOR_TEXT_1);  
      display.print("OK");    
    }
    return;
}

void writeMidi(unsigned char* data) {
  Serial.write(data, 3); /* Write data */
  Serial.flush();
}

void addPitchOnOff(byte channel, byte pitch, byte velocity, float duration) {
  unsigned long currentMillis = millis();
  for (byte i = 0; i < (MATRIX_WIDTH + MATRIX_HEIGHT) * MAX_SCREENS; i++) {
    if (pitchesOnOff[i].pitch == 0) {
      pitchesOnOff[i].channel = channel;
      pitchesOnOff[i].pitch = pitch;
      pitchesOnOff[i].velocity = velocity;
      pitchesOnOff[i].millisOff = currentMillis + (duration * (1000 / (bpm / 60)));
      pitchesOnOff[i].onMessageSent = false;
      return;    
    }
  }
}

void processPitchesOnOff() {
  unsigned long currentMillis = millis();
  for (byte i = 0; i < (MATRIX_WIDTH + MATRIX_HEIGHT) * MAX_SCREENS; i++) {
    /* Process only if pitch is not zero */
    if (pitchesOnOff[i].pitch != 0) {
      if (!pitchesOnOff[i].onMessageSent) {
        /* Send MIDI pitch ON */
        unsigned char data[3] = {
          0x90 | ((pitchesOnOff[i].channel) & 0x0F),
          pitchesOnOff[i].pitch & 0x7F,
          pitchesOnOff[i].velocity & 0x7F
        };
        writeMidi(data);      
        /* Program MIDI pitch OFF */
        pitchesOnOff[i].onMessageSent = true;
      } else if (pitchesOnOff[i].onMessageSent) {
        /* Send MIDI pitch OFF after timeout */
        if (currentMillis >= pitchesOnOff[i].millisOff) {
          unsigned char data[3] = {
            0x80 | ((pitchesOnOff[i].channel) & 0x0F),
            pitchesOnOff[i].pitch & 0x7F,
            pitchesOnOff[i].velocity & 0x7F
          };
          writeMidi(data);      
          /* Remove entry */
          pitchesOnOff[i].pitch = 0;
        }
      }
    }
  }
}

void processEncoder() {
  /* Get current encoder clicks and reset it */
  cli();
  int8_t clicks = encoderClicks;
  encoderClicks = 0;
  sei();
  /* Encoder rotation */
  if (clicks != 0) {
    if (currentOptionMode == 'S') {
      /* Avoid flickering if top or bottom reached */
      if (((clicks > 0) && (currentOption < OPTION_COUNT - 1)) || ((clicks < 0) && (currentOption > 0))) {
        /* Print current option */
        printOption(currentOption, 'U');
        if ((clicks > 0) && (currentOption < OPTION_COUNT - 1)) currentOption ++;
        else if ((clicks < 0) && (currentOption > 0)) currentOption --;
        printOption(currentOption, 'S');        
      }
    } else {
      if (currentOption == OPTION_CELL_ID) {  
        if ((clicks > 0) && (currentCellIndex < MAX_CELLS - 1)) currentCellIndex ++;
        else if ((clicks < 0) && (currentCellIndex > 0)) currentCellIndex --;
      } 
      if (currentOption == OPTION_CELL_X) {  
        if ((clicks > 0) && (currentCell.x < MATRIX_WIDTH - 1)) currentCell.x ++;
        else if ((clicks < 0) && (currentCell.x > 0)) currentCell.x --;
      } 
      if (currentOption == OPTION_CELL_Y) {  
        if ((clicks > 0) && (currentCell.y < MATRIX_HEIGHT - 1)) currentCell.y ++;
        else if ((clicks < 0) && (currentCell.y > 0)) currentCell.y --;
      } 
      if (currentOption == OPTION_CELL_TYPE) {  
        if (clicks > 0) currentCell.type = getRightCellType(currentCell.type, true);
        else if (clicks < 0) currentCell.type = getRightCellType(currentCell.type, false);
      } 
      if (currentOption == OPTION_PITCH_X) {  
        if ((clicks > 0) && (pitchX[currentScreen] < 107)) pitchX[currentScreen] ++;
        else if ((clicks < 0) && (pitchX[currentScreen] > 12)) pitchX[currentScreen] --;
      } 
      else if (currentOption == OPTION_MODE_X) {  
        if ((clicks > 0) && (modeX[currentScreen] < 6)) modeX[currentScreen] ++;
        else if ((clicks < 0) && (modeX[currentScreen] > 0)) modeX[currentScreen] --;
      } 
      else if (currentOption == OPTION_ORDER_X) {  
        if ((clicks > 0) && (orderX[currentScreen] < 3)) orderX[currentScreen] ++;
        else if ((clicks < 0) && (orderX[currentScreen] > 0)) orderX[currentScreen] --;
      } 
      else if (currentOption == OPTION_PITCH_Y) {  
        if ((clicks > 0) && (pitchY[currentScreen] < 107)) pitchY[currentScreen] ++;
        else if ((clicks < 0) && (pitchY[currentScreen] > 12)) pitchY[currentScreen] --;
      } 
      else if (currentOption == OPTION_MODE_Y) {  
        if ((clicks > 0) && (modeY[currentScreen] < 6)) modeY[currentScreen] ++;
        else if ((clicks < 0) && (modeY[currentScreen] > 0)) modeY[currentScreen] --;
      } 
      else if (currentOption == OPTION_ORDER_Y) {  
        if ((clicks > 0) && (orderY[currentScreen] < 3)) orderY[currentScreen] ++;
        else if ((clicks < 0) && (orderY[currentScreen] > 0)) orderY[currentScreen] --;
      } 
      else if (currentOption == OPTION_BPM) {  
        if ((clicks > 0) && (bpm < 240)) bpm ++;
        else if ((clicks < 0) && (bpm > 0)) bpm --;
      }
      else if (currentOption == OPTION_SCREEN) {  
        if ((clicks > 0) && (currentScreen < MAX_SCREENS - 1)) {
          currentScreen ++; redrawScreenStep = 1;
        }
        else if ((clicks < 0) && (currentScreen > 0)) { 
          currentScreen --; redrawScreenStep = 1;
        }
      }
      else if (currentOption == OPTION_CHANNEL) {  
        if ((clicks > 0) && (channel[currentScreen] < 16 - 1)) channel[currentScreen] ++;
        else if ((clicks < 0) && (channel[currentScreen] > 0)) channel[currentScreen] --;
      }
      else if (currentOption == OPTION_VELOCITY) {  
        if ((clicks > 0) && (velocity[currentScreen] < 127)) velocity[currentScreen] ++;
        else if ((clicks < 0) && (velocity[currentScreen] > 0)) velocity[currentScreen] --;
      }
      else if (currentOption == OPTION_DURATION) {  
        if ((clicks > 0) && (duration[currentScreen] <= 4.9)) duration[currentScreen] += .1;
        else if ((clicks < 0) && (duration[currentScreen] > 0.1)) duration[currentScreen] -= .1;
      }
      printOption(currentOption, 'E');      
    }
  }
}

void processEncoderButton() {
  unsigned long m = millis();
  /* Encoder button pressed */
  if ((GPIO_BTN) && (!buttonCurrentStatus) && (buttonLastMillis + BUTTON_DEBOUNCE <= m)) {
    if (currentOptionMode == 'E') {
      printOption(currentOption, 'S');         
      currentOptionMode = 'S';
      processOption(currentOption);
    } else {
      /* Button like control */
      if (currentOption == OPTION_CELL_OK) {
        processOption(currentOption);
      /* Input like control */
      } else {
        printOption(currentOption, 'E');         
        currentOptionMode = 'E';     
      }
    }
    buttonCurrentStatus = true;
    buttonLastMillis = m;
  } else 
  /* Button sustained */
  if ((GPIO_BTN) && (buttonCurrentStatus)) {
    if (buttonLastMillis + REBOOT_TIMEOUT < millis()) reboot();
  } else {
    buttonCurrentStatus = false;    
  }
}

void processMode(byte* pitches, char axis) { 
  byte mask[] = { 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1 };
  bool b = (axis == 'X');
  byte pitch = b ? pitchX[currentScreen] : pitchY[currentScreen];
  byte mode = b ? modeX[currentScreen] : modeY[currentScreen];
  byte order = b ? orderX[currentScreen] : orderY[currentScreen];
  /* If order OFF set all pitches to 0 */
  if (order == ORDER_OFF) {
    for (byte i = 0; i <= 7; i++) { pitches[i] = 0; }
  } else {
    pitches[0] = pitch;
    for (byte i = 1; i <= 7; i++) { pitches[i] = pitches[i - 1] + mask[i + mode - 1]; }
    if (order == ORDER_DOWN) reversePitches(pitches);
    if (order == ORDER_RANDOM) randomizePitches(pitches);   
  }
}

void processOption(byte option) {
  if ((option == OPTION_PITCH_X) || (option == OPTION_MODE_X) || (option == OPTION_ORDER_X)) {
    processMode(pitchesX[currentScreen], 'X');
    printNotesX();
  } else 
  if ((option == OPTION_PITCH_Y) || (option == OPTION_MODE_Y) || (option == OPTION_ORDER_Y)) {
    processMode(pitchesY[currentScreen], 'Y');
    printNotesY();
  } else 
  if ((option == OPTION_CELL_OK)) {
    previousMatrix[cells[currentScreen][currentCellIndex].y][cells[currentScreen][currentCellIndex].x] = COLOR_NONE;
    matrix[cells[currentScreen][currentCellIndex].y][cells[currentScreen][currentCellIndex].x] = COLOR_EMPTY;
    cells[currentScreen][currentCellIndex].x = currentCell.x;
    cells[currentScreen][currentCellIndex].y = currentCell.y;
    cells[currentScreen][currentCellIndex].type = currentCell.type;
    if (currentCellIndex < MAX_CELLS) {
      currentCellIndex ++;
      printOption(OPTION_CELL_ID, 'U');     
    } 
  }
}

void reversePitches(byte* pitches) {
  for (byte i = 0; i <= 3; i++) {
    byte aux = pitches[i];
    pitches[i] = pitches[7 - i];
    pitches[7 - i] = aux;
  }
}

void randomizePitches(byte* pitches) {
  for (byte i = 0; i < 8; i++) { /* Shufle 8 times */
    byte r1 = random(0, 8); /* Random number from 0 to 7 */
    byte r2 = r1;
    while(r1 == r2) r2 = random(0, 8); /* Random number from 0 to 7 */
    byte aux = pitches[r1];
    pitches[r1] = pitches[r2];
    pitches[r2] = aux;
  }
}

void processMovable(byte screen) {
  for(byte i = 0; i < MAX_CELLS; i++) {
    processEncoderButton();
    /* Skip empty and non movable */
    if ((cells[screen][i].type != COLOR_UP) && (cells[screen][i].type != COLOR_DOWN) && (cells[screen][i].type != COLOR_LEFT) && (cells[screen][i].type != COLOR_RIGHT)) continue;
    /* Clear previous cell in matrix */
    if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = COLOR_EMPTY;
    /* Cells moving up */
    if (cells[screen][i].type == COLOR_UP) {
      if (cells[screen][i].y > 0) { /* Check border collision */
        cells[screen][i].y--;
        if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = COLOR_UP;         
      } else { /* Collision then change direction */
        addPitchOnOff(channel[screen], pitchesX[screen][cells[screen][i].x], velocity[screen], duration[screen]);
        cells[screen][i].type = COLOR_DOWN;         
        cells[screen][i].y++;
      }      
    } else 
    /* Cells moving down */
    if (cells[screen][i].type == COLOR_DOWN) {
      if (cells[screen][i].y < MATRIX_HEIGHT - 1) { /* Check border collision */
        cells[screen][i].y++;
        if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = COLOR_DOWN;         
      } else { /* Collision then change direction */
        addPitchOnOff(channel[screen], pitchesX[screen][cells[screen][i].x], velocity[screen], duration[screen]);
        cells[screen][i].type = COLOR_UP;         
        cells[screen][i].y--;
      }      
    } else 
    /* Cells moving left */
    if (cells[screen][i].type == COLOR_LEFT) {
      if (cells[screen][i].x > 0) { /* Check border collision */
        cells[screen][i].x--;
        if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = COLOR_LEFT;         
      } else { /* Collision then change direction */
        addPitchOnOff(channel[screen], pitchesY[screen][cells[screen][i].y], velocity[screen], duration[screen]);
        cells[screen][i].type = COLOR_RIGHT;         
        cells[screen][i].x++;
      }      
    } else 
    /* Cells moving right */
    if (cells[screen][i].type == COLOR_RIGHT) {
      if (cells[screen][i].x < MATRIX_WIDTH - 1) { /* Check border collision */
        cells[screen][i].x++;
        if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = COLOR_RIGHT;         
      } else { /* Collision then change direction */
        addPitchOnOff(channel[screen], pitchesY[screen][cells[screen][i].y], velocity[screen], duration[screen]);
        cells[screen][i].type = COLOR_LEFT;         
        cells[screen][i].x--;
      }      
    }      
    /* Print current cell in matrix */ 
    if (currentScreen == screen) matrix[cells[screen][i].y][cells[screen][i].x] = cells[screen][i].type;             
  }
}

void processCollisions(byte screen) {
  bool collisionProcessedCells[MAX_CELLS] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
  /* Process cell-to-mirror/wall collisions first as they have priority over cell-to-cell collisions */
  for(byte i1 = 0; i1 < MAX_CELLS; i1++) {
    /* Skip non movable cells as they cannot colide with others */
    if ((cells[screen][i1].type == COLOR_EMPTY) || (cells[screen][i1].type == COLOR_WALL) || (cells[screen][i1].type == COLOR_MIRROR_1) || (cells[screen][i1].type == COLOR_MIRROR_2)) continue;
    processEncoderButton();
    for(byte i2 = 0; i2 < MAX_CELLS; i2++) {
      /* There is no collision with empty cells, skip */
      if (cells[screen][i2].type == COLOR_EMPTY) continue; 
      /* Only process cell-to-mirror/wall collisions, otherwise skip */
      if ((cells[screen][i2].type != COLOR_MIRROR_1) && (cells[screen][i2].type != COLOR_MIRROR_2) && (cells[screen][i2].type != COLOR_WALL)) continue; 
      if (collisionProcessedCells[i1]) continue; /* If this cell was already processed skip */
      if (i1 == i2) continue; /* Same cell, skip */
      /* Detect collision */
      if ((cells[screen][i1].x == cells[screen][i2].x) && (cells[screen][i1].y == cells[screen][i2].y)) {
        if (cells[screen][i2].type == COLOR_MIRROR_1) {
          if (cells[screen][i1].type == COLOR_LEFT) cells[screen][i1].type = COLOR_DOWN;
          else if (cells[screen][i1].type == COLOR_RIGHT) cells[screen][i1].type = COLOR_UP;
          else if (cells[screen][i1].type == COLOR_DOWN) cells[screen][i1].type = COLOR_LEFT;
          else if (cells[screen][i1].type == COLOR_UP) cells[screen][i1].type = COLOR_RIGHT;
          collisionProcessedCells[i1] = true;
        } else 
        if (cells[screen][i2].type == COLOR_MIRROR_2) {
          if (cells[screen][i1].type == COLOR_LEFT) cells[screen][i1].type = COLOR_UP;
          else if (cells[screen][i1].type == COLOR_RIGHT) cells[screen][i1].type = COLOR_DOWN;
          else if (cells[screen][i1].type == COLOR_DOWN) cells[screen][i1].type = COLOR_RIGHT;
          else if (cells[screen][i1].type == COLOR_UP) cells[screen][i1].type = COLOR_LEFT;
          collisionProcessedCells[i1] = true;
        } else 
        if (cells[screen][i2].type == COLOR_WALL) {
          if (cells[screen][i1].type == COLOR_LEFT) cells[screen][i1].type = COLOR_RIGHT;
          else if (cells[screen][i1].type == COLOR_RIGHT) cells[screen][i1].type = COLOR_LEFT;
          else if (cells[screen][i1].type == COLOR_DOWN) cells[screen][i1].type = COLOR_UP;
          else if (cells[screen][i1].type == COLOR_UP) cells[screen][i1].type = COLOR_DOWN;
          collisionProcessedCells[i1] = true;
        }
      }
    }
  }    
  /* Process cell-to-cell collisions second place as they have less priority */
  for(byte i1 = 0; i1 < MAX_CELLS; i1++) {
    /* Skip non movable cells as they cannot colide with others */
    if ((cells[screen][i1].type == COLOR_EMPTY) || (cells[screen][i1].type == COLOR_WALL) || (cells[screen][i1].type == COLOR_MIRROR_1) || (cells[screen][i1].type == COLOR_MIRROR_2)) continue;
    processEncoderButton();
    for(byte i2 = 0; i2 < MAX_CELLS; i2++) {
      /* There is no collision with empty cells, skip */
      if (cells[screen][i2].type == COLOR_EMPTY) continue; 
      /* Only process cell-to-cell collisions, otherwise skip */
      if ((cells[screen][i2].type != COLOR_UP) && (cells[screen][i2].type != COLOR_DOWN) && (cells[screen][i2].type != COLOR_LEFT) && (cells[screen][i2].type != COLOR_RIGHT)) continue; 
      if (collisionProcessedCells[i1]) continue; /* If this cell was already processed skip */
      if (i1 == i2) continue; /* Same cell, skip */
      /* Detect collision */
      if ((cells[screen][i1].x == cells[screen][i2].x) && (cells[screen][i1].y == cells[screen][i2].y)) {
        if (cells[screen][i1].type == COLOR_LEFT) cells[screen][i1].type = COLOR_RIGHT;
        else if (cells[screen][i1].type == COLOR_RIGHT) cells[screen][i1].type = COLOR_LEFT;
        else if (cells[screen][i1].type == COLOR_UP) cells[screen][i1].type = COLOR_DOWN;
        else if (cells[screen][i1].type == COLOR_DOWN) cells[screen][i1].type = COLOR_UP;
        collisionProcessedCells[i1] = true;
      }
    }
  }
}

void processNonMovable(void) {
  for(byte i = 0; i < MAX_CELLS; i++) {      
    processEncoderButton();
    /* Skip empty cells right away */
    if (cells[currentScreen][i].type == COLOR_EMPTY) continue;
    if (cells[currentScreen][i].type == COLOR_WALL) {
      matrix[cells[currentScreen][i].y][cells[currentScreen][i].x] = COLOR_WALL;
    } else 
    if (cells[currentScreen][i].type == COLOR_MIRROR_1) {
      matrix[cells[currentScreen][i].y][cells[currentScreen][i].x] = COLOR_MIRROR_1;
    } else 
    if (cells[currentScreen][i].type == COLOR_MIRROR_2) {
      matrix[cells[currentScreen][i].y][cells[currentScreen][i].x] = COLOR_MIRROR_2;
    }      
  } 
}

void renderMatrix(void) {
  for(byte x = 0; x < MATRIX_WIDTH; x++) {
    for(byte y = 0; y < MATRIX_HEIGHT; y++) {
      processEncoderButton();
      /* If empty is needed and previous value is empty then skip this cell render */
      if ((matrix[y][x] == COLOR_EMPTY) && (previousMatrix[y][x] == matrix[y][x])) continue;
      /* Render cell */
      display.fillRect(x * CELL_PIXELS + 2, y * CELL_PIXELS + 2, CELL_PIXELS, CELL_PIXELS, matrix[y][x]);   
      previousMatrix[y][x] = matrix[y][x];   
    }
  }     
}

void redrawScreen() {
  if (redrawScreenStep == 0) return;
  else if (redrawScreenStep == 1) { 
    /* Clear matrix */
    for(byte x = 0; x < MATRIX_WIDTH; x++) {
      for(byte y = 0; y < MATRIX_HEIGHT; y++) {
        previousMatrix[y][x] = COLOR_NONE;
        matrix[y][x] = COLOR_EMPTY;
      }
    }
  }
  else if (redrawScreenStep == 2) { printNotesY(); }
  else if (redrawScreenStep == 3) { printNotesX(); }  
  else if (redrawScreenStep == 4) { printOption(OPTION_PITCH_X, 'U'); }
  else if (redrawScreenStep == 5) { printOption(OPTION_MODE_X, 'U'); }
  else if (redrawScreenStep == 6) { printOption(OPTION_ORDER_X, 'U'); }
  else if (redrawScreenStep == 7) { printOption(OPTION_PITCH_Y, 'U'); }
  else if (redrawScreenStep == 8) { printOption(OPTION_MODE_Y, 'U'); }
  else if (redrawScreenStep == 9) { printOption(OPTION_ORDER_Y, 'U'); }
  /* -- No need to print BPM -- */
  /* -- No need to print screen -- */
  else if (redrawScreenStep == 10) { printOption(OPTION_CHANNEL, 'U'); }
  else if (redrawScreenStep == 11) { printOption(OPTION_VELOCITY, 'U'); }
  else if (redrawScreenStep == 12) { printOption(OPTION_DURATION, 'U'); redrawScreenStep = 0; }
  /* Increment step */
  if (redrawScreenStep != 0) redrawScreenStep++;
}

void EncoderInterrupt(void) {  
  cli();
  /* Determine the current CW and CWW pio status by averaging a good number of samples */
  byte gpioCW[2] = {0, 0};
  byte gpioCWW[2] = {0, 0};
  for (byte i = 0; i < 40; i++) {
    gpioCW[GPIO_CW]++;
    gpioCWW[GPIO_CWW]++;
  }
  bool gpioCwFinal = (gpioCW[1] > gpioCW[0]);
  bool gpioCwwFinal = (gpioCWW[1] > gpioCWW[0]);  
  if (gpioCwFinal && gpioCwwFinal) {  
    if (encoderTurning) {
      if (encoderLastMillis + ENCODER_INTERRUPT_DELAY < millis()) {
        encoderClicks = encoderDirection;        
      }
    }
    encoderTurning = 0; 
    encoderDirection = 0;
  } else
  if ((gpioCwFinal == 0) && (gpioCwwFinal == 0)) {
    encoderTurning = 1;
  } else
  if (encoderTurning == 0) {
    if (gpioCwFinal) { encoderDirection = -1; encoderClicks = encoderDirection; } 
    else if (gpioCwwFinal) { encoderDirection = 1; encoderClicks = encoderDirection; }
  }
  encoderLastMillis = millis();   
  sei();
}

int getRightCellType(int type, bool forward) {
  if (forward) {
    if (type == COLOR_EMPTY) return COLOR_UP;
    else if (type == COLOR_UP) return COLOR_DOWN;
    else if (type == COLOR_DOWN) return COLOR_LEFT;
    else if (type == COLOR_LEFT) return COLOR_RIGHT;
    else if (type == COLOR_RIGHT) return COLOR_WALL;
    else if (type == COLOR_WALL) return COLOR_MIRROR_1;
    else if (type == COLOR_MIRROR_1) return COLOR_MIRROR_2;
  } else {
    if (type == COLOR_MIRROR_2) return COLOR_MIRROR_1;
    else if (type == COLOR_MIRROR_1) return COLOR_WALL;
    else if (type == COLOR_WALL) return COLOR_RIGHT;
    else if (type == COLOR_RIGHT) return COLOR_LEFT;
    else if (type == COLOR_LEFT) return COLOR_DOWN;
    else if (type == COLOR_DOWN) return COLOR_UP;
    else if (type == COLOR_UP) return COLOR_EMPTY;
  }
}

void printCellTypeToString(int type) {
  if (type == COLOR_EMPTY) display.print("EMP");
  else if (type == COLOR_UP) display.print("UP ");
  else if (type == COLOR_DOWN) display.print("DWN");
  else if (type == COLOR_LEFT) display.print("LFT");
  else if (type == COLOR_RIGHT) display.print("RGT");
  else if (type == COLOR_WALL) display.print("WAL");
  else if (type == COLOR_MIRROR_1) display.print("MR1");
  else if (type == COLOR_MIRROR_2) display.print("MR2");
}

void focusCurrentCell() {
  /* Focus current cell being edited */
  if (cells[currentScreen][currentCellIndex].type != COLOR_EMPTY) {
    display.fillRect(cells[currentScreen][currentCellIndex].x * CELL_PIXELS + 2 + 2, 
                     cells[currentScreen][currentCellIndex].y * CELL_PIXELS + 2 + 2, 
                     CELL_PIXELS - 4, 
                     CELL_PIXELS - 4,
                     COLOR_EMPTY);      
  }
  if ((previousCurrentCell.x != currentCell.x) || (previousCurrentCell.y != currentCell.y)) {
    /* Focus cell to be x, y */
    display.fillRect(previousCurrentCell.x * CELL_PIXELS + 2 + 2, 
                     previousCurrentCell.y * CELL_PIXELS + 2 + 2, 
                     CELL_PIXELS - 4, 
                     CELL_PIXELS - 4,
                     COLOR_EMPTY);      
  }
  /* Focus cell to be x, y */
  display.fillRect(currentCell.x * CELL_PIXELS + 2 + 2, 
                   currentCell.y * CELL_PIXELS + 2 + 2, 
                   CELL_PIXELS - 4, 
                   CELL_PIXELS - 4,
                   COLOR_BORDER);      
  previousCurrentCell.x = currentCell.x;
  previousCurrentCell.y = currentCell.y;  
}

void reboot(void) {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (true);
}