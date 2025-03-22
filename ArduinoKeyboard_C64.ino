/*
Arduino Pro Micro based USB keyboard driver

Commodore 64 Version

*/

#include <Arduino.h>
#include <Keyboard.h>

// global variables
#define SERIAL_BAUDRATE     9600

#define NUM_ROWS 9
#define NUM_COLUMNS 9

#define DEBOUNCE_SCANS 5

// 9 row drivers - outputs
const uint8_t  row0 = 9;
const uint8_t  row1 = 8;
const uint8_t  row2 = 7;
const uint8_t  row3 = 6;
const uint8_t  row4 = 5;
const uint8_t  row5 = 4;
const uint8_t  row6 = 3;
const uint8_t  row7 = 2;
const uint8_t  row8 = 0;

uint8_t  rowPins[NUM_ROWS] = {row0, row1, row2, row3, row4, row5, row6, row7, row8};

// 9 column inputs - pulled high
const uint8_t  columnA = 10;
const uint8_t  columnB = 16;
const uint8_t  columnC = 14;
const uint8_t  columnD = 15;
const uint8_t  columnE = 18;
const uint8_t  columnF = 19;
const uint8_t  columnG = 20;
const uint8_t  columnH = 21;
const uint8_t  columnI = 1;

uint8_t  columnPins[NUM_COLUMNS] = {columnA, columnB, columnC, columnD, columnE, columnF, columnG, columnH, columnI};

const uint8_t  resetRow = row3;
const uint8_t  resetColumn = columnA;

char keyMap[NUM_ROWS][NUM_COLUMNS] = {
  {'1','3','5','7','9','-',KEY_INSERT,KEY_BACKSPACE,'\0'},
  {'`','w','r','y','i','p',']',KEY_RETURN,'\0'},
  {KEY_TAB,'a','d','g','j','l','\'',KEY_RIGHT_ARROW,'\0'},
  {KEY_ESC,KEY_LEFT_SHIFT,'x','v','n',',','/',KEY_DOWN_ARROW,'\0'},
  {' ','z','c','b','m','.',KEY_RIGHT_SHIFT,KEY_F1,'\0'},
  {KEY_LEFT_CTRL,'s','f','h','k',';','\\',KEY_F3,'\0'},
  {'q','e','t','u','o','[',KEY_DELETE,KEY_F5,'\0'},
  {'2','4','6','8','0','=',KEY_HOME,KEY_F7,'\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0',KEY_PAGE_UP},
};


// alternate shifted key state not used in this project
const uint8_t shiftKeyRow = 3;
const uint8_t shiftKeyColumn = 2;

char keyMapShifted[NUM_ROWS][NUM_COLUMNS] = {
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
  {'\0','\0','\0','\0','\0','\0','\0','\0','\0'},
};

// class definitions

class KeyboardKey {

  public:

    int row;
    int column;
    int lastState;
    int debounceCount;
    char keyCode;
    char keyCodeShifted;

    // default constructor
    KeyboardKey(){}

    // init constructor;
    KeyboardKey(int rowNum, int colNum){
      // log matrix position
      row = rowNum;
      column = colNum;
      // get keycode from keyMap array
      keyCode = keyMap[row][column];
      keyCodeShifted = keyMapShifted[row][column];
      // initialise state and debounce
      lastState = 0;
      debounceCount = 0;
    } 

    void updateKey(KeyboardKey shiftKey){
      // row driver set by matrix handler
      // scan key
      int thisState = digitalRead(columnPins[column]);
      int shiftKeyState;
      if(thisState != lastState) {
        // state changing
        debounceCount ++;
        // is debounce finished
        if (debounceCount >= DEBOUNCE_SCANS){
          // key state has changed
          lastState = thisState;
          debounceCount = 0;
          // high = not pressed, low = pressed
          if(thisState == 1){
            //key released
            releaseKeys();
            //Serial.println(keyCode);
            //delay(10);
          }
          else {
            // key pressed
            // decide which key to press based on shift key state
            shiftKeyState = shiftKey.lastState;
            if((shiftKeyState == 1) || (keyCodeShifted == '\0')) {
              // no shift
              Keyboard.press(keyCode);
            }
            else {
              // shift key pressed - press shift keycode
              // release shift key
              shiftKey.releaseKeys();
              Keyboard.press(keyCodeShifted);
            }
            
            //Serial.println(keyCode);
            //delay(10);
          }
        }
      }
      else {
        // no change in state
        // make sure debounce reset
        debounceCount = 0;
      }

    }

    void releaseKeys(){
      Keyboard.release(keyCode);
      Keyboard.release(keyCodeShifted);
    }

}; // end class KeyboardKey

// global key handler array
// initialised in setup
KeyboardKey keyHandlers[NUM_ROWS][NUM_COLUMNS];

class MatrixDriver{

  public:

    MatrixDriver() {}

    void scanMatrix() {
      int row;
      int column;

      for(row = 0; row < NUM_ROWS; row ++){
        // trun on this row line
        activateRowLine(row);
        // do column keys
        for(column = 0; column < NUM_COLUMNS; column ++){
          keyHandlers[row][column].updateKey(keyHandlers[shiftKeyRow][shiftKeyColumn]);
        }
      }

    }

    void activateRowLine(int rowNum){
      // rowNum is zero based to match arrays
      int row;
      for(row = 0; row < NUM_ROWS; row ++){
        if(row == rowNum){
          // turn on this row
          digitalWrite(rowPins[row], LOW);
        }
        else {
          // turn off this row
          digitalWrite(rowPins[row], HIGH);
        }
      }
    }

}; // end class Matrix Driver

// global matrix driver instance
MatrixDriver matrix = MatrixDriver();

void setup() {
  int row;
  int column;
  String message;

  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);

  // reset button
  pinMode(resetColumn, INPUT_PULLUP);
  pinMode(resetRow, OUTPUT);
  digitalWrite(row1, LOW);
  delay(10);
  // wait for start command on newline button
  Serial.println("waiting for start");
  while(digitalRead(resetColumn)){
    delay(100);
  }

  Serial.println("run/stop button pressed");
  delay(10);

  // start keyboard
  Keyboard.begin();

  // init columns
  pinMode(columnA, INPUT_PULLUP);
  pinMode(columnB, INPUT_PULLUP);
  pinMode(columnC, INPUT_PULLUP);
  pinMode(columnD, INPUT_PULLUP);
  pinMode(columnE, INPUT_PULLUP);
  pinMode(columnF, INPUT_PULLUP);
  pinMode(columnG, INPUT_PULLUP);
  pinMode(columnH, INPUT_PULLUP);
  pinMode(columnI, INPUT_PULLUP);

  // init rows as output and set high - inactive
  pinMode(row0, OUTPUT);
  digitalWrite(row0, HIGH);
  pinMode(row1, OUTPUT);
  digitalWrite(row1, HIGH);
  pinMode(row2, OUTPUT);
  digitalWrite(row2, HIGH);
  pinMode(row3, OUTPUT);
  digitalWrite(row3, HIGH);
  pinMode(row4, OUTPUT);
  digitalWrite(row4, HIGH);
  pinMode(row5, OUTPUT);
  digitalWrite(row5, HIGH);
  pinMode(row6, OUTPUT);
  digitalWrite(row6, HIGH);
  pinMode(row7, OUTPUT);
  digitalWrite(row7, HIGH);
  pinMode(row8, OUTPUT);
  digitalWrite(row8, HIGH);

  // setup key array
  for(row = 0; row < NUM_ROWS; row ++){
    for(column = 0; column < NUM_COLUMNS; column ++){
      keyHandlers[row][column] = KeyboardKey(row, column);
    }
  }

  Serial.println("finished setup");
  delay(10);

}

void loop() {
  // scan key matrix every 1 ms (+ execution time)
  //Serial.println("scanning");
  matrix.scanMatrix();
  delay(1);
}
