#define BTN1 2
#define BTN2 3
#define BTN3 4
#define BTN4 5

#define LED1 10
#define LED2 11
#define LED3 12
#define LED4 13

#define LED_GREEN 6
#define LED_YELLOW 7
#define LED_RED 8

#define BUZZER 9

const byte ledPins[4] = { 10, 11, 12, 13 };

volatile unsigned long time = 0;
volatile unsigned long lastInterrupt = 0;

byte sequence[64];
byte seqLength = 2;

volatile byte buttonPressed = 0;

byte userIndex = 0;
byte attempts = 0;

unsigned long lastAction = 0;
unsigned long greenTime = 0;

volatile bool redBlink = false;
volatile bool redState = false;

byte showIndex = 0;
bool ledOn = false;

enum GameState {
  INIT,
  SHOW_SEQUENCE,
  USER_INPUT,
  SUCCESS,
  FAIL,
  ERROR
};

GameState state = INIT;

ISR(TIMER1_COMPA_vect) {
  time += 1;

  if (redBlink) {
    static unsigned long lastBlink = 0;

    if (time - lastBlink >= 1000 / (attempts + 1)) {
      redState = !redState;
      digitalWrite(LED_RED, redState);
      lastBlink = time;
    }
  }
}

// Dve vrste prekida zbog manjka pinova
void btn1ISR() {
  if (time - lastInterrupt < 200) return;

  buttonPressed = 1;
  lastInterrupt = time;
}
void btn2ISR() {
  if (time - lastInterrupt < 200) return;

  buttonPressed = 2;
  lastInterrupt = time;
}

ISR(PCINT2_vect) {
  if (time - lastInterrupt < 200) return;

  if (!digitalRead(BTN3)) buttonPressed = 3;
  if (!digitalRead(BTN4)) buttonPressed = 4;

  lastInterrupt = time;
}

void clearLeds() {
  for (byte i = 0; i < 4; i++)
    digitalWrite(ledPins[i], LOW);
}

void lightLed(byte num) {
  for (byte i = 0; i < 4; i++)
    digitalWrite(ledPins[i], (i == num - 1));
}

void setup() {
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  randomSeed(analogRead(A0));

  attachInterrupt(digitalPinToInterrupt(BTN1), btn1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN2), btn2ISR, FALLING);

  PCICR |= (1 << PCIE2);                      //PCIE2 -> Port D
  PCMSK2 |= (1 << PCINT20) | (1 << PCINT21);  //PCINT20 = 4, PCINT21 = 5

  //Timer
  noInterrupts();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 249;

  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10);

  TIMSK1 |= (1 << OCIE1A);

  interrupts();

  Serial.begin(9600);
}

void loop() {
  switch (state) {
    case INIT:

      for (int i = 0; i < seqLength; i++)
        sequence[i] = random(1, 5);

      userIndex = 0;
      attempts = 0;

      state = SHOW_SEQUENCE;
      Serial.println("state: SHOW_SEQUENCE");
      lastAction = time;

      break;

    case SHOW_SEQUENCE:

      if (time - lastAction > 1000) {
        lastAction = time;

        if (ledOn) {
          clearLeds();
        } else {
          lightLed(sequence[showIndex]);
          showIndex++;
        }

        ledOn = !ledOn;

        if (showIndex > seqLength) {
          showIndex = 0;
          userIndex = 0;
          ledOn = false;

          digitalWrite(LED_YELLOW, HIGH);
          state = USER_INPUT;
          Serial.println("state: USER_INPUT");
        }
      }

      break;

    case USER_INPUT:

      if (buttonPressed) {
        lightLed(buttonPressed);
        lastAction = time;
        if (buttonPressed == sequence[userIndex]) {
          userIndex++;

          if (userIndex == seqLength) {
            digitalWrite(LED_YELLOW, LOW);
            state = SUCCESS;
            Serial.println("state: SUCCESS");
            lastAction = time;
          }
        } else {
          digitalWrite(LED_YELLOW, LOW);
          state = FAIL;
          Serial.println("state: FAIL");
        }
        buttonPressed = 0;
      }
      if (time - lastAction > 500) {
        lastAction = time;
        clearLeds();
      }
      break;

    case SUCCESS:

      if (!greenTime) {
        greenTime = time;
        digitalWrite(LED_GREEN, HIGH);
      }
      else if (time - greenTime > 1000) {
        greenTime = 0;
        digitalWrite(LED_GREEN, LOW);

        sequence[seqLength] = random(1, 5);
        seqLength++;

        state = SHOW_SEQUENCE;
        Serial.println("state: SHOW_SEQUENCE");
      }

      break;

    case FAIL:
      clearLeds();
      tone(BUZZER, 400, 500);
      attempts++;

      if (attempts >= 3) {
        redBlink = false;
        digitalWrite(LED_RED, HIGH);
        state = ERROR;
        Serial.println("state: ERROR");
      } else {
        redBlink = true;

        showIndex = 0;
        userIndex = 0;
        digitalWrite(LED_YELLOW, HIGH);
        state = USER_INPUT;  // Show?
        Serial.println("state: USER_INPUT");
        tone(BUZZER, 400, 200);
      }

      break;

    case ERROR:
      //tone(BUZZER, 400, 200);

      break;
  }
  if (time - lastAction > 500) {
    //lastAction = time;
    clearLeds();
  }
}
