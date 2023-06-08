/* Libraries */
#include <LCD_I2C.h>

/* Macros */
#define DEBUG  // Toggle for serial debug messages

#ifdef DEBUG
#define PRINT_DEBUG(msg) \
  do { Serial.println(msg); } while (0)
#else
#define PRINT_DEBUG(msg) \
  do { \
  } while (0)
#endif

/* Physical connections (PIN) */
#define MAIN_KNOB_SW 2
#define MAIN_KNOB_DT 3
#define MAIN_KNOB_CLK 4

#define PLUG_KNOB_SW 5
#define PLUG_KNOB_DT 6
#define PLUG_KNOB_CLK 7

/* Physical device addresses (i2c) */
#define LCD1_ADDR 0x26
#define LCD2_ADDR 0x27

/* Constants */
#define MAIN_DISPLAY_HEADER F("  ROTOR  | I | O")
#define PLUG_DISPLAY_HEADER F("  PLUG    I | O ")
#define INITIAL_ANIMATION_DURATION 2000
#define ASCII_A 0x41
#define BUTTON_DELAY 200


/* Global variables */
LCD_I2C mainLcd(LCD2_ADDR, 16, 2);  // Rotors and I/O LCD
LCD_I2C plugLcd(LCD1_ADDR, 16, 2);  // Plug board LCD

String mainDisplay = " ?  ?  ? | ? | ?";
String plugDisplay = "  BOARD   ? | ? ";

/* This is part of the Enigma Machine configuration (cipher) */
char rotors[3] = { 'A', 'A', 'A' };
char rotorsWiring[3][26] = {
  { 'W', 'J', 'N', 'P', 'K', 'Y', 'T', 'E', 'B', 'A', 'S', 'H', 'C', 'Z', 'V', 'O', 'U', 'L', 'Q', 'X', 'I', 'G', 'M', 'D', 'F', 'R' },
  { 'B', 'Y', 'Q', 'V', 'E', 'U', 'M', 'S', 'Z', 'K', 'W', 'A', 'R', 'N', 'T', 'F', 'H', 'J', 'I', 'P', 'X', 'C', 'G', 'L', 'D', 'O' },
  { 'L', 'C', 'J', 'E', 'U', 'Y', 'N', 'A', 'F', 'V', 'Q', 'O', 'Z', 'X', 'I', 'D', 'B', 'R', 'G', 'S', 'T', 'K', 'W', 'M', 'H', 'P' },
  /*
  { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
  { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
  { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' },
  */
};
char reflectorWiring[26] = { 'E', 'Y', 'S', 'T', 'B', 'O', 'M', 'Q', 'Z', 'A', 'W', 'K', 'R', 'N', 'V', 'H', 'F', 'J', 'I', 'P', 'X', 'C', 'G', 'L', 'D', 'U' };
char plugBoard[26];

int mainCursorIndexes[4] = { 1, 4, 7, 11 };
int plugCursorIndexes[2] = { 10, 14 };

int mainCursor = 0;       // Index of char of the selected rotor in the main display
int plugBoardCursor = 0;  // Index of char of the selected char in the plugboard display

int plugBoardInputIndex = 0;  // Index of an input letter of the plug board (0 - 25)

char input = 'A';  // IO of the Enigma Machine
char output = '?';

int lastMainKnobCLK;  // Knobs buttons last clicked time stamp
int lastPlugKnobCLK;

int mainKnobRotation = 0;
int plugKnobRotation = 0;

void getMainKnobRotation() {
  int currentMainKnobCLK = digitalRead(MAIN_KNOB_CLK);

  if (currentMainKnobCLK != lastMainKnobCLK && currentMainKnobCLK == 1) {
    if (digitalRead(MAIN_KNOB_DT) != currentMainKnobCLK) {
      --mainKnobRotation;
    } else {
      ++mainKnobRotation;
    }
  }

  lastMainKnobCLK = currentMainKnobCLK;
}

void getPlugKnobRotation() {
  int currentPlugKnobCLK = digitalRead(PLUG_KNOB_CLK);

  if (currentPlugKnobCLK != lastPlugKnobCLK && currentPlugKnobCLK == 1) {
    if (digitalRead(PLUG_KNOB_DT) != currentPlugKnobCLK) {
      --plugKnobRotation;
    } else {
      ++plugKnobRotation;
    }
  }

  lastPlugKnobCLK = currentPlugKnobCLK;
}

bool mainKnobClicked() {
  bool result = false;
  static unsigned long lastButtonPress = 0;
  int btnState = digitalRead(MAIN_KNOB_SW);

  if (btnState == LOW) {
    if (millis() - lastButtonPress > BUTTON_DELAY) {
      PRINT_DEBUG("Main knob button clicked");
      result = true;
    }
    lastButtonPress = millis();
  }

  return result;
}

bool plugKnobClicked() {
  bool result = false;
  static unsigned long lastButtonPress = 0;
  int btnState = digitalRead(PLUG_KNOB_SW);

  if (btnState == LOW) {
    if (millis() - lastButtonPress > BUTTON_DELAY) {
      PRINT_DEBUG("Plug knob button clicked");
      result = true;
    }
    lastButtonPress = millis();
  }

  return result;
}

void renderMainDisplay() {
  mainLcd.setCursor(0, 0);
  mainLcd.print(MAIN_DISPLAY_HEADER);

  mainDisplay.setCharAt(0, ' ');
  mainDisplay.setCharAt(1, rotors[0]);
  mainDisplay.setCharAt(2, ' ');
  mainDisplay.setCharAt(3, ' ');
  mainDisplay.setCharAt(4, rotors[1]);
  mainDisplay.setCharAt(5, ' ');
  mainDisplay.setCharAt(6, ' ');
  mainDisplay.setCharAt(7, rotors[2]);
  mainDisplay.setCharAt(8, ' ');
  mainDisplay.setCharAt(10, ' ');
  mainDisplay.setCharAt(11, input);
  mainDisplay.setCharAt(12, ' ');
  mainDisplay.setCharAt(15, output);

  mainDisplay.setCharAt(mainCursorIndexes[mainCursor] + 1, ']');
  mainDisplay.setCharAt(mainCursorIndexes[mainCursor] - 1, '[');

  mainLcd.setCursor(0, 1);
  mainLcd.print(mainDisplay);
}

void renderPlugDisplay() {
  plugLcd.setCursor(0, 0);
  plugLcd.print(PLUG_DISPLAY_HEADER);

  plugDisplay.setCharAt(9, ' ');
  plugDisplay.setCharAt(10, ASCII_A + plugBoardInputIndex);
  plugDisplay.setCharAt(11, ' ');
  plugDisplay.setCharAt(13, ' ');
  plugDisplay.setCharAt(14, plugBoard[plugBoardInputIndex]);
  plugDisplay.setCharAt(15, ' ');

  plugDisplay.setCharAt(plugCursorIndexes[plugBoardCursor] + 1, ']');
  plugDisplay.setCharAt(plugCursorIndexes[plugBoardCursor] - 1, '[');

  plugLcd.setCursor(0, 1);
  plugLcd.print(plugDisplay);
}

int indexOf(char arr[], char x) {
  for (int i = 0; i < 26; ++i) {
    if (arr[i] == x) {
      return i;
    }
  }
  return -1;
}

char forwardToRotors(char input) { 
  char result = input;

  // Rotors rotations
  int shift0 = rotors[0] - ASCII_A;
  int shift1 = rotors[1] - ASCII_A;
  int shift2 = rotors[2] - ASCII_A;

  // Forward
  result = rotorsWiring[0][result - ASCII_A + shift0];
  result = rotorsWiring[1][result - ASCII_A + shift1];
  result = rotorsWiring[2][result - ASCII_A + shift2];

  // Reflector
  result = reflectorWiring[result - ASCII_A];

  // Backwards
  result = indexOf(rotorsWiring[2], result) + ASCII_A + shift2;
  result = indexOf(rotorsWiring[1], result) + ASCII_A + shift1;
  result = indexOf(rotorsWiring[0], result) + ASCII_A + shift0;
  //result = rotorsWiring[2][result - ASCII_A + shift2];
  //result = rotorsWiring[1][result - ASCII_A + shift1];
  //result = rotorsWiring[0][result - ASCII_A + shift0];

  return result;
}

void shiftRotors() {
  if (rotors[0] == 'Z') {
    rotors[0] = 'A';
    if (rotors[1] == 'Z') {
      rotors[1] = 'A';
      if (rotors[2] == 'Z') {
        rotors[2] = 'A';
      } else {
        ++rotors[2];
      }
    } else {
      ++rotors[1];
    }
  } else {
    ++rotors[0];
  }
}

void setup() {
  // Debug initialization
#ifdef DEBUG
  Serial.begin(9600);
#endif

  PRINT_DEBUG("Starting setup");

  // Initialize the displays
  mainLcd.begin();
  mainLcd.backlight();
  plugLcd.begin();
  plugLcd.backlight();

  // Initial display animation
  mainLcd.setCursor(0, 0);
  mainLcd.print(F(" Enigma Machine"));
  mainLcd.setCursor(0, 1);
  mainLcd.print(F("      UwU"));

  plugLcd.setCursor(0, 0);
  plugLcd.print(F(" Enigma Machine"));
  plugLcd.setCursor(0, 1);
  plugLcd.print(F("Licence: GPL 0.6"));

  delay(INITIAL_ANIMATION_DURATION);

  // Initialize the knobs
  pinMode(MAIN_KNOB_CLK, INPUT);
  pinMode(MAIN_KNOB_DT, INPUT);
  pinMode(MAIN_KNOB_SW, INPUT_PULLUP);

  pinMode(PLUG_KNOB_CLK, INPUT);
  pinMode(PLUG_KNOB_DT, INPUT);
  pinMode(PLUG_KNOB_SW, INPUT_PULLUP);

  // Initialize global variables
  lastMainKnobCLK = digitalRead(MAIN_KNOB_CLK);
  lastPlugKnobCLK = digitalRead(PLUG_KNOB_CLK);

  for (int i = 0; i < 26; ++i) {
    plugBoard[i] = ASCII_A + i;
  }

  // Clear displays before loop function
  mainLcd.clear();
  plugLcd.clear();

  PRINT_DEBUG("Finished setup");
}

void loop() {
  /* Input handling */
  if (mainKnobClicked()) {
    mainKnobRotation = 0;
    if (mainCursor < 3) {
      ++mainCursor;
    } else {
      // Generate the output of the Enigma Machine
      output = plugBoard[input - ASCII_A];
      output = forwardToRotors(output);
      shiftRotors();
      output = plugBoard[output - ASCII_A];
    }
  }

  if (plugKnobClicked()) {
    plugBoardCursor = plugBoardCursor == 0 ? 1 : 0;
    plugKnobRotation = 0;
  }

  getMainKnobRotation();
  // Map knob rotation
  if (mainKnobRotation > 25) mainKnobRotation = 0;
  if (mainKnobRotation < 0) mainKnobRotation = 25;
  // Knob rotation logic
  if (mainCursor < 3) {
    rotors[mainCursor] = ASCII_A + mainKnobRotation;
  } else {
    input = ASCII_A + mainKnobRotation;
  }

  getPlugKnobRotation();
  // Map knob rotation
  if (plugKnobRotation > 25) plugKnobRotation = 0;
  if (plugKnobRotation < 0) plugKnobRotation = 25;
  // Knob rotation logic
  if (plugBoardCursor == 0) {
    plugBoardInputIndex = plugKnobRotation;
  } else {
    plugBoard[plugBoardInputIndex] = plugKnobRotation + ASCII_A;
  }

  /* Displays rendering */
  renderMainDisplay();
  renderPlugDisplay();

  // Loop function delay
  //delay(1);
}