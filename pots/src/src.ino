#include <TM1637Display.h>

#define CLK 51
#define DIO 53

#define PIN_BTN                           33                          

#define PIN_CW                            2                           /*  | This 2 should match */
#define GPIO_CW                           ((PINE & B00010000) == 0)   /* -| */
#define PIN_CWW                           3                           /*  | This 2 should match */
#define GPIO_CWW                          ((PINE & B00100000) == 0)   /* -| */

#define ENCODER_TIMEOUT                   40  /* milliseconds */
#define ENCODER_INTERRUPT_DELAY           20  /* milliseconds */

#define BUTTON_DEBOUNCE                   500 /* milliseconds */

#define VALUE_BLINK_TIMEOUT               300 /* milliseconds */

#define MAX_PAGE                          5

#define POT_TIMEOUT                       20 /* milliseconds */

#define POT_HISTERESYS                    2

TM1637Display display = TM1637Display(CLK, DIO);

volatile int encoderTurning = 0;
volatile int encoderClicks = 0;  
volatile int encoderDirection = 0;
volatile unsigned long encoderLastMillis = 0;

unsigned long buttonLastMillis = 0;
bool buttonCurrentStatus = false;

unsigned char channel = 1;
unsigned char page = 1;
unsigned char pot = 1;

unsigned char currentSelectedValue = 0;

unsigned long potLastMillis = 0;
int currentPotValues[16][MAX_PAGE][8];

bool currentPotEnableBottom[8] = { false, false, false, false, false, false, false, false };
bool currentPotEnableTop[8] = { false, false, false, false, false, false, false, false };

void setup() {

  Serial.begin(115200);

  display.clear();
  display.setBrightness(3);

  const uint8_t full[] = { 0xFF, 0xFF, 0xFF, 0xFF };
  display.setSegments(full);

  randomSeed(analogRead(A8));

  /* Encoder button */
  pinMode(PIN_BTN, INPUT_PULLUP);

  /* Encoder rotation */
  digitalWrite(PIN_CW, LOW); /* Turn off PWM Timer */
  digitalWrite(PIN_CWW, LOW); /* Turn off PWM Timer */
  pinMode(PIN_CW, INPUT_PULLUP);
  pinMode(PIN_CWW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_CW), EncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_CWW), EncoderInterrupt, CHANGE);

  for(byte x = 0; x < 16; x++)
    for(byte y = 0; y < MAX_PAGE; y++)
      for(byte z = 0; z < 8; z++)
        currentPotValues[x][y][z] = -1;

}

void loop() {

  unsigned long loopMillis = millis();

  /* Encoder timeout */
  if ((encoderDirection != 0) && (loopMillis > encoderLastMillis + ENCODER_TIMEOUT)) 
  { 
    cli(); 
    encoderDirection = 0; 
    encoderTurning = 0; 
    int8_t clicks = encoderClicks; 
    encoderClicks = 0;
    if (clicks != 0) {
      if (currentSelectedValue == 0) {
        if ((clicks > 0) && (channel < 16)) channel++;
        else if ((clicks < 0) && (channel > 1)) channel--;      
      } else 
      if (currentSelectedValue == 1) {
        if ((clicks > 0) && (page < MAX_PAGE)) page++;
        else if ((clicks < 0) && (page > 1)) page--;      
      }
      for(byte x = 0; x < 8; x++) {
        currentPotEnableBottom[x] = false;
        currentPotEnableTop[x] = false;
      }
    }  
    sei(); 
  }

  /* Button pressed */
  if ((!digitalRead(PIN_BTN)) && (!buttonCurrentStatus) && (buttonLastMillis + BUTTON_DEBOUNCE <= loopMillis)) {
    buttonCurrentStatus = true;
    buttonLastMillis = loopMillis;
    if (currentSelectedValue == 0) currentSelectedValue = 1;
    else currentSelectedValue = 0;
  } else {
    buttonCurrentStatus = false;        
  }

  /* Display current channel */
  if (loopMillis > buttonLastMillis + VALUE_BLINK_TIMEOUT) {
    display.showNumberDec(channel, false, 2, 0);
    display.showNumberDec(page, false, 2, 2);
  } else {
    const uint8_t empty[] = { 0x00, 0x00 };
    if (currentSelectedValue == 0) display.setSegments(empty, 2, 0);
    else if (currentSelectedValue == 1) display.setSegments(empty, 2, 2);
  }

  /* Read POTs */
  if (loopMillis > potLastMillis + POT_TIMEOUT) {
    /* Get last and current value pot current pot */
    int lastPotValue = currentPotValues[channel - 1][page - 1][pot - 1];
    unsigned char currentPotValue = (unsigned char)map(analogRead(pot - 1), 1023, 0, 0, 127);
    if (currentPotValue < POT_HISTERESYS) currentPotValue = 0;
    if (currentPotValue > 127 - POT_HISTERESYS) currentPotValue = 127;
    /* Enable current pot if last pot value is equal to current (+/- histeresys), otherwise pot is disabled and never pushed through MIDI */
    if (currentPotValue < lastPotValue + POT_HISTERESYS) currentPotEnableBottom[pot - 1] = true;
    if (currentPotValue > lastPotValue - POT_HISTERESYS) currentPotEnableTop[pot - 1] = true;   
    /* Send MIDI if current pot is enabled and there is a new value set */
    if (currentPotEnableBottom[pot - 1] && currentPotEnableTop[pot - 1]) {
      if ((currentPotValue < lastPotValue - POT_HISTERESYS) || (currentPotValue > lastPotValue + POT_HISTERESYS))
      {
        unsigned char data[3] = {
          0xB0 | (channel - 1),
          40 + (pot - 1) + ((page - 1) * 8),
          currentPotValue
        };
        writeMidi(data);   
        currentPotValues[channel - 1][page - 1][pot - 1] = currentPotValue;
      }
    }
    /* Increment pot for next run */
    pot++; if(pot > 8) pot = 1;
    potLastMillis = loopMillis;
  }
  
}

void EncoderInterrupt(void) {  
  cli();
  unsigned long current_millis = millis();
  /* Determine the current CW and CWW pio status by averaging a good number of samples */
  byte gpioCW[2] = {0, 0};
  byte gpioCWW[2] = {0, 0};
  for (byte i = 0; i < 10; i++) {
    gpioCW[GPIO_CW]++;
    gpioCWW[GPIO_CWW]++;
  }
  bool gpioCwFinal = (gpioCW[1] > gpioCW[0]);
  bool gpioCwwFinal = (gpioCWW[1] > gpioCWW[0]);  
  if (gpioCwFinal && gpioCwwFinal) {  
    if (encoderTurning) {
      if (encoderLastMillis + ENCODER_INTERRUPT_DELAY < current_millis) {
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
  if (encoderLastMillis + ENCODER_TIMEOUT < current_millis ) {
    encoderLastMillis = current_millis;   
  }
  sei();
}

void writeMidi(unsigned char* data) {
  Serial.write(data, 3); /* Write data */
  Serial.flush();
}

