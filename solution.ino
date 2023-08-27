#include "funshield.h"

constexpr byte EMPTY_GLYPH = 0b11111111;

using ul = unsigned long;

//Diodes
constexpr int diodePins[] = { led1_pin, led2_pin, led3_pin, led4_pin };
constexpr int count_of_diodes = sizeof(diodePins) / sizeof(diodePins[0]);

//Display
constexpr int displayPins[] = { latch_pin, data_pin, clock_pin };
constexpr int pinsCount = sizeof(displayPins) / sizeof(displayPins[0]);
constexpr byte positionByte = 0b00000001;
constexpr int displayPositions = 4;
constexpr byte digitsGlyph[] = { 0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001, 0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000 };
constexpr int DmaxPosition = 2;

//Buttons
constexpr int buttonPins[] = { button1_pin, button2_pin, button3_pin };
constexpr int buttonsCount = sizeof(buttonPins) / sizeof(buttonPins[0]);
constexpr int normalRandomNumber = 0;
constexpr int configCountDice = 1;
constexpr int configDiceNum = 2;

// DiceRoller
constexpr int diceSizes[] = { 4, 6, 8, 10, 12, 20, 100 };
constexpr int countDices = sizeof(diceSizes) / sizeof(diceSizes[0]);

// RandomGenerator
constexpr int primeNumbers[] = {3,	5, 7, 11,	13,	17,	19,	23,	29, 31};
constexpr int countPrimes = sizeof(primeNumbers) / sizeof(primeNumbers[0]);


constexpr int base = 10;

// messages at the initialize of the display
constexpr char startMessage[displayPositions] = { 'D', 'I', 'C', 'E' };

//Timer
constexpr ul runningBeadsTime = 300;
constexpr ul startRandomGeneratorTime = 137; // to add more randomness

enum class Mode { started,
            normal,
            configuration };
enum class ButtonState { off,
                   isTriggered,
                   held,
                   stopped };



constexpr byte LETTER_GLYPH[]{
  0b10001000,  // A //
  0b10000011,  // b
  0b11000110,  // C
  0b10100001,  // d
  0b10000110,  // E
  0b10001110,  // F
  0b10000010,  // G
  0b10001001,  // H.   HELLo WorLd
  0b11111001,  // I
  0b11100001,  // J
  0b10000101,  // K
  0b11000111,  // L
  0b11001000,  // M
  0b10101011,  // n
  0b10100011,  // o
  0b10001100,  // P
  0b10011000,  // q
  0b10101111,  // r
  0b10010010,  // S
  0b10000111,  // t
  0b11000001,  // U
  0b11100011,  // v
  0b10000001,  // W
  0b10110110,  // ksi
  0b10010001,  // Y
  0b10100100,  // Z
};


int getPower(int position, const int base) {
  int number = 1;
  for (int i = 0; i < position; ++i) {
    number *= base;
  }
  return number;
}



class Diode {

private:

  int diodeNumber;
  bool currentState;

public:

  void initialize(int diode_number) {

    diodeNumber = diode_number;
    pinMode(diodePins[diodeNumber], OUTPUT);

    currentState = false;  // turn off diodes
    change(currentState);
  }

  void change(bool new_state) {

    currentState = new_state;
    digitalWrite(diodePins[diodeNumber], currentState ? LOW : HIGH);
  }
};


class Timer {

private:

  ul previousTime;

public:

  ul initializedTime_;

  void initialize(ul currentTime) {
    previousTime = currentTime;
    initializedTime_ = currentTime;
  }

  bool timeElapse(ul currentTime, ul periodLength) {

    ul elapsedTime;

    if (currentTime < previousTime) {  // checking overflow
      elapsedTime = (~((ul)0) - previousTime + 1) + currentTime;
    } else {
      elapsedTime = currentTime - previousTime;
    }

    if (elapsedTime >= periodLength) {
      previousTime = previousTime + periodLength;
      return true;
    }

    else {
      return false;
    }
  }

  ul initializedTime() {
    return initializedTime_;
  }

};


class RunningBead {

private:

  bool state;
  Diode diodes[count_of_diodes];
  int direction;
  int currentDiode;
  Timer timer;

public:

  void initialize() {

    direction = 1;
    currentDiode = 0;
    state = false;

    for (int i = 0; i < count_of_diodes; i++) {
      diodes[i].initialize(i);
    }
  }

  int current_diode() {
    return currentDiode;
  }

  void change_diode() {

    if ((direction == 1) && (currentDiode == count_of_diodes - 1)) {
      direction = -1;
    }
    if ((direction == -1) && (currentDiode == 0)) {
      direction = 1;
    }
    currentDiode += direction;
  }

  void moveBead() {
    diodes[current_diode()].change(false);  //turn off 0
    change_diode();                         //change 1
    diodes[current_diode()].change(true);
  }

  void turnOffBeads() {  // turning all off
    if (state == true) {
      state = false;
      for (int i = 0; i < count_of_diodes; i++) {
        diodes[i].change(false);
      }
    }
  }

  void controlBeads(unsigned long currentTime) {
    if (state == false) {
      timer.initialize(currentTime);
      state = true;
    } else {
      if (timer.timeElapse(currentTime, runningBeadsTime)) {
        moveBead();
      }
    }
  }
};




class Display {

public:

  void initialize() {

    for (int i = 0; i < pinsCount; ++i) {
      pinMode(displayPins[i], OUTPUT);
    }
  }

  void showChar(byte glyph, byte glyphPosition) {

    digitalWrite(latch_pin, LOW);                    // turning it off
    shiftOut(data_pin, clock_pin, MSBFIRST, glyph);  //
    shiftOut(data_pin, clock_pin, MSBFIRST, glyphPosition);
    digitalWrite(latch_pin, HIGH);  // turning it on
  }

  void showDigit(int digit_, int position) {  // number is the counter value

    byte digit = digitsGlyph[digit_];


    showChar(digit, getPositionByte(position));
  }

  void showLetter(char symbol, int position) {

    byte glyph = EMPTY_GLYPH;

    if (isAlpha(symbol)) {
      glyph = LETTER_GLYPH[symbol - (isUpperCase(symbol) ? 'A' : 'a')];
    }

    showChar(glyph, getPositionByte(position));
  }

  byte getPositionByte(int position) {
    byte digitPosition = positionByte << (displayPositions - 1 - position);
    return digitPosition;
  }
  
};


class TextDisplay : public Display {

private:

  Mode mode;
  bool displayActive;
  int currentPosition;
  const char* string;
  bool initialized;
  int currentOrder;
  int Dposition;


public:

  void initialize() {

    Display::initialize();
    mode = Mode::started;
    displayActive = true;
    currentPosition = 0;
    initialized = true;
    currentOrder = 1;
    setString(startMessage);

  }

  void displayTurnOn() {  // when we touch any of this buttons it will turn off
    initialized = false;
    displayActive = true;
  }

  void displayTurnOff() {
    displayActive = false;
  }

  void setString(const char* string_) {
    string = string_;
    displayActive = true;
  }

  void changeMode(Mode newMode) {
    mode = newMode;
  }

  void update(long randomNumber, int numberDices, int range) {


    if (!displayActive) {
      showChar(EMPTY_GLYPH, getPositionByte(currentPosition));  // I will come back
    } else {

      if (mode == Mode::started) {  // Game has not been initialized
        char symbol = string[displayPositions - (currentPosition + 1)];
        showLetter(symbol, currentPosition);
      }

      if (mode == Mode::normal) {  // Normal mode else clause is in the case the game hasnt started
        if (randomNumber / currentOrder > 0) {
          int digit = (randomNumber / currentOrder) % base;
          showDigit(digit, currentPosition);
        } else {
          showChar(EMPTY_GLYPH, getPositionByte(currentPosition));
        }
      }

      if (mode == Mode::configuration) {  // we cannot just have a single showFunction because we need to display different numbers

        if (range / currentOrder > 0 && currentPosition < DmaxPosition) {
          Dposition = currentPosition;  // we need the position of d to be flexible
          int digit = (range / currentOrder) % base;
          showDigit(digit, currentPosition);
        }

        else if (currentPosition == (Dposition + 1)) {
          showLetter('d', currentPosition);
        }

        else if (currentPosition == (Dposition + 2)) {
          showDigit(numberDices, currentPosition);
        }

        else {
          showChar(EMPTY_GLYPH, getPositionByte(currentPosition));
        }
      }
    }

    currentPosition++;
    currentOrder = currentOrder * base;

    if (currentPosition == displayPositions) {
      currentPosition = 0;
      currentOrder = 1;
    }
  }
};


class Button {

private:

  int buttonNumber;
  bool currentState;
  Timer time;
  ButtonState state;

  bool pressed() {
    return (digitalRead(buttonPins[buttonNumber]) == LOW);
  }

public:

  void initialize(int button_number) {

    buttonNumber = button_number;
    currentState = false;
    state = ButtonState::off;
    pinMode(buttonPins[buttonNumber], INPUT);
  }

  bool triggered() {

    bool newState = pressed();
    if (newState != currentState) {
      currentState = newState;
      return currentState;
    } else {
      return false;
    }
  }

  void updateButtonState() {
    if (state == ButtonState::stopped) {
      state = ButtonState::off;
    }
  }

  ul timebuttonInitialized() {
    return time.initializedTime();
  }

  bool checkstate(ul currentTime) {
    if (pressed()) {
      if (triggered()) {
        state = ButtonState::isTriggered;
        currentState = true;
        time.initialize(currentTime);
      }
      if (isButtonTriggered() && time.timeElapse(currentTime, startRandomGeneratorTime)) {
        state = ButtonState::held;
      }
      return true;
    } else {
      if (currentState && state == ButtonState::held) {
        state = ButtonState::stopped;
      } else {
        state = ButtonState::off;
      }
      currentState = false;
      return false;
    }
  }

  bool isButtonTriggered() {
    return (state == ButtonState::isTriggered);
  }

  bool isButtonStopped() {
    return (state == ButtonState::stopped);
  }

  bool isButtonHeld() {
    return (state == ButtonState::held);
  }
};

class GeneratorRandomNumbers {
  
  int highestFactor; 
  int getRandom(int range, int increasedFactor_, int primeNumber) {
    int output = ((primeNumber ^ increasedFactor_) % range) + 1;
    return output;
  }

public:

  void initialize() {
    highestFactor = getPower(displayPositions, base) / base;
  }

  int getRandomNumber(int count, int range, ul increasedFactor) {

    int increasedFactor_ = increasedFactor % highestFactor;
    int randomNumber = 0;

    for (int i = 0; i < count; ++i) {

      int currentPrime = primeNumbers[getRandom(countPrimes, increasedFactor_, currentPrime)];
      int singleRandomNumber = getRandom(range, increasedFactor_, currentPrime);
      int newRandomNumber = ((currentPrime * singleRandomNumber) % range) + 1;
      randomNumber += singleRandomNumber;

    }
    return randomNumber;
  }
  
};


class DiceRoller {

private:

  GeneratorRandomNumbers RandomNumber;

  Mode mode;
  ul heldTime;

  int randomNumber;
  int countOfDices;
  int dicesNumber;
  int position;



public:

  void initialized() {

    RandomNumber.initialize();
    mode = Mode::started;
    randomNumber = 0;
    countOfDices = 1;
    position = 0;
    dicesNumber = diceSizes[position];
    heldTime = 0;
  }

  void updateCountOfDices() {
    countOfDices = (countOfDices + 1) % base;
    if (countOfDices == 0) {
      countOfDices = 1;
    }
  }

  void updateDiceNumber() {
    position = (position + 1) % countDices;
    dicesNumber = diceSizes[position];
  }

  void checkNormalMode() {
    mode = Mode::normal;
  }

  void updateNormalMode(ul initializedTime, ul currentTime) {
    heldTime = currentTime - initializedTime;
  }

  void generateRandomNumber() {

    randomNumber = RandomNumber.getRandomNumber(countOfDices, dicesNumber, heldTime);
  }

  void changeConfigMode(int buttonConfig) {

    if (mode != Mode::configuration) {
      mode = Mode::configuration;
    } else {
      if (buttonConfig == configCountDice) {
        updateCountOfDices();
      } else {
        updateDiceNumber();
      }
    }
  }

  int getRandomNumber() {
    return randomNumber;
  }

  int getCountDices() {
    return countOfDices;
  }

  int getDiceNumber() {
    return dicesNumber;
  }

  Mode getMode() {
    return mode;
  }
};


Button button[displayPositions];
TextDisplay display;
DiceRoller diceRoller;
RunningBead runningBead;


void setup() {

  display.initialize();
  diceRoller.initialized();
  for (int i = 0; i < buttonsCount; ++i) {
    button[i].initialize(i);
  }
  runningBead.initialize();
}

void loop() {

  unsigned long currentTime = millis();

  if (button[normalRandomNumber].checkstate(currentTime)) {
    if (button[normalRandomNumber].isButtonTriggered()) { 
      diceRoller.checkNormalMode();
      display.changeMode(diceRoller.getMode());

    }
    if (button[normalRandomNumber].isButtonHeld()) {
      diceRoller.updateNormalMode(button[normalRandomNumber].timebuttonInitialized(), currentTime);
    }
  }

  if (button[configCountDice].triggered()) {
    diceRoller.changeConfigMode(configCountDice);
    display.changeMode(diceRoller.getMode());
  }

  if (button[configDiceNum].triggered()) {
    diceRoller.changeConfigMode(configDiceNum);
    display.changeMode(diceRoller.getMode());
  }

  if (button[normalRandomNumber].isButtonHeld()) {
    display.displayTurnOff();
    runningBead.controlBeads(currentTime);
  }

  if (button[normalRandomNumber].isButtonStopped()) {
    button[normalRandomNumber].updateButtonState();
    diceRoller.generateRandomNumber();
    display.displayTurnOn();
    runningBead.turnOffBeads();
  }

  display.update(diceRoller.getRandomNumber(), diceRoller.getCountDices(), diceRoller.getDiceNumber());
}