#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

enum WatchDogTimeout {
  WDT_16ms = 0,
  WDT_32ms,
  WDT_64ms,
  WDT_128ms,
  WDT_250ms,
  WDT_500ms,
  WDT_1sec,
  WDT_2sec,
  WDT_4sec,
  WDT_8sec
};

enum PowerState {
  DEEP_SLEEP,
  GAME
};

const unsigned int BUZZER_PIN = PB0;
const unsigned int MOTOR_PIN = PB1;
const unsigned int BUTTON_PIN = PB2;
const unsigned int ACCELEROMETER_IN = PB3;
const unsigned int ACCELEROMETER_POWER = PB4;

const unsigned long DEFAULT_TURN_DURATION = 60000; // The time in milliseconds that game lasts
unsigned long turnDuration = DEFAULT_TURN_DURATION;
const float beepingRatio = 0.95; // Percentage of the remaining time that should we beep
unsigned long timeToBeep = (float) DEFAULT_TURN_DURATION * beepingRatio;
volatile bool watchDogBarked = false;
bool watchDogEnabled = false;
const WatchDogTimeout gameWdt = WDT_128ms; // The game interval


WatchDogTimeout currentWdt = gameWdt;
unsigned long elapsedTime = 0; // How much time has elapsed in the running turn in milliseconds

volatile bool buttonPressed = false;

const unsigned long remindBeepDuration = 50; // How long in milliseconds we should beep to "stress" the user

const unsigned long TIME_TO_PASS_THE_BALL = 3000; // How long in milliseconds the player has in order to pass the ball after immobility is detected
const unsigned int ACC_THRESHOLD = 80; // The threshold (of analogRead values) over which we register a "movement" (experimentally determined)
const unsigned int IMMOBILITY_THRESHOLD = 20; // The threshold of the amount of times the analogRead values were below the acceleration threshold
unsigned int prevAccReading = 0; // The last reading by the accelerometer. We start it by 0 but it should not practically make a big differnece
unsigned int immobilityCounter = 0;
bool fastCountdownMode = false;

uint8_t enabledADCSRA = 0; // Will hold the initial ADCSRA value

PowerState currentState = DEEP_SLEEP; // Start with sleep as the initial state

/**
   Vibrate for the specified amount of time (blocking)
   @param duration milliseconds the motor will vibrate
*/
void vibrateFor(unsigned long duration) {
  startVibrating();
  delay(duration);
  stopVibrating();
}

/**
   Utility method that starts the vibrating motor
*/
void startVibrating() {
  digitalWrite(MOTOR_PIN, HIGH);
}

/**
   Utility method that stops the vibrating motor
*/
void stopVibrating() {
  digitalWrite(MOTOR_PIN, LOW);
}

/**
   Ring the buzzer for the specified amount of time (blocking)
   @param duration milliseconds the buzzer will ring
*/
void ringFor(unsigned long duration) {
  startRinging();
  delay(duration);
  stopRinging();
}

/**
   Utility method that starts ringing the buzzer
*/
void startRinging() {
  digitalWrite(BUZZER_PIN, HIGH);
}

/**
   Utility method that stops ringing the buzzer
*/
void stopRinging() {
  digitalWrite(BUZZER_PIN, LOW);
}

/**
   A utility method to derive a watchdog timeout's duration
   @return the amount of milliseconds corresponding to a watchdog timeout
*/
unsigned long getTimeoutDuration() {
  return 1 << (currentWdt + 4);
}

void setupWatchDogTimeoutOneshot(WatchDogTimeout wdt) {
  // Adopted from InsideGadgets (www.insidegadgets.com)
  byte timeoutVal = wdt & 7;
  if (wdt > 7) {
    timeoutVal |= (1 << 5);
  }
  timeoutVal |= (1 << WDCE);

  MCUSR &= ~(1 << WDRF);
  WDTCR |= (1 << WDCE) | (1 << WDE); // Start timed sequence
  WDTCR = timeoutVal;
  WDTCR |= _BV(WDIE);
  wdt_reset();  // pat the dog
  currentWdt = wdt;
}

/**
   Disables the accelerometer by setting the pin that powers it up to LOW
*/
void turnAccelerometerOff() {
  digitalWrite(ACCELEROMETER_POWER, LOW);
}

/**
   Enables the accelerometer by setting the pin that powers it up to HIGH
*/
void turnAccelerometerOn() {
  digitalWrite(ACCELEROMETER_POWER, HIGH);
}

/**
  Watchdog interrupt routine to be triggered when watchdog times out
*/
ISR(WDT_vect) {
  watchDogBarked = true;
  wdt_disable(); // disable watchdog
}

/**
   Change interrupt routine (e.g. when button presses occur)
*/
ISR (PCINT0_vect) {
  // TO-DO: Investigate ways to debounce the button
  if (digitalRead(BUTTON_PIN) == LOW) {
    buttonPressed = true;
  }
}

/**
   Sets up pin change interrupt for PB2
*/
void setupChangeInterrupt() {
  PCMSK |= bit (PCINT2); // want pin D2 (PB2)
  GIFR |= bit (PCIF); // clear any outstanding interrupts
  GIMSK |= bit (PCIE); // enable pin change interrupts
}

/**
   Sets the MCU into a deep sleep state until a change interrupt is triggered
*/
void goToSleep() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // Disable ADC when we are in deep sleep state, otherwise have it on
  // We are missing the first analogRead in the game state this way, but we can afford
  // it for the sake of code simplicity
  ADCSRA = currentState == DEEP_SLEEP ? 0 : enabledADCSRA;
  power_all_disable(); // power off ADC, Timer 0 and 1, serial interface
  sleep_enable();
  sleep_cpu(); // Sleep here and wait for the interrupt
  sleep_disable();
  power_all_enable(); // power everything back on
}

void setup() {
  // Set up the I/O pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ACCELEROMETER_IN, INPUT);
  pinMode(ACCELEROMETER_POWER, OUTPUT);
  // Log down the value of ADCSRA in order to restore it when necessary
  enabledADCSRA = ADCSRA;
  setupChangeInterrupt();
}

void loop() {
  switch (currentState) {
    case DEEP_SLEEP:
      // Go to deep sleep until an interrupt occurs
      goToSleep();
      // Proceed to the
      if (buttonPressed) {
        // Beep fast twice to indicate the beginning of a game
        ringFor(100);
        delay(100);
        ringFor(100);
        currentState = GAME;
        buttonPressed = false;
        turnAccelerometerOn();
      }
      break;
    case GAME:
      // If watchdog is not enabled, then we either just arrived from SLEEP state
      // or from a watchdog bark. Therefore we need to set another watchdog timer.
      if (!watchDogEnabled) {
        // Set up watchdog to be triggered (once) after the specified time
        setupWatchDogTimeoutOneshot(gameWdt);
        watchDogEnabled = true;
      }
      // Go to deep sleep until an interrupt occurs
      goToSleep();
      // We might have gone out of sleep due to another interrupt (i.e. the switch)
      // So make sure that we know what we are doing depending on who interrupted our sleep
      if (watchDogBarked) {
        watchDogBarked = false;
        watchDogEnabled = false;
        elapsedTime += getTimeoutDuration();
        unsigned long remainingTime = turnDuration - elapsedTime;
        /* --- Main game logic --- */
        if (elapsedTime >= turnDuration) { // End of game
          // Play a sound and vibration sequence to indicate the end of a game due to timeout
          ringFor(50);
          delay(50);
          ringFor(50);
          delay(50);
          startVibrating();
          ringFor(800);
          stopVibrating();
          currentState = DEEP_SLEEP;
          elapsedTime = 0;
          turnDuration = DEFAULT_TURN_DURATION; // Reset the turn duration that might have been changed
          timeToBeep = turnDuration * beepingRatio; // reset the remaining time since the turn is over
          immobilityCounter = 0;
          fastCountdownMode = false;
          turnAccelerometerOff();
        } else if (remainingTime <= timeToBeep) { // Time to beep
          // Ring when we reach half of the remaining time available
          ringFor(remindBeepDuration);
          elapsedTime += remindBeepDuration; // Add the time we spent ringing
          timeToBeep *= beepingRatio;
        }
        // We need to detect if a player is holding the ball for too long in order to start a fast countdown
        // If the ball is not moved during the designated time to pass the ball, then the game ends prematurely
        unsigned int currentAccReading = analogRead(ACCELEROMETER_IN);
        int accDelta = currentAccReading - prevAccReading;
        prevAccReading = currentAccReading;
        // If the ball has not been moved enough (i.e. thrown)
        if (abs(accDelta) <= ACC_THRESHOLD) {
          // Register a touch and check if the ball has been held for too long
          if (++immobilityCounter >= IMMOBILITY_THRESHOLD) {
            // If we are not already in fast countdown mode, start the fast countdown
            if (!fastCountdownMode) {
              unsigned long decreasedTurnDuration = elapsedTime + TIME_TO_PASS_THE_BALL;
              // Go into fast countodown only if the decreased turn duration is actually after when the game
              // is supposed to finish. In that case do not adjust the time so the users do not cheat by prolonging the game
              if (decreasedTurnDuration <= DEFAULT_TURN_DURATION) { // If the turn duration is actually decreased and not extended
                turnDuration = decreasedTurnDuration;
                fastCountdownMode = true;
              }
            }
          }
        } else {
          // Restore the counter if there was a movement but only restore the duration and time to beep
          // if we just were in fast countdown mode
          immobilityCounter = 0;
          if (fastCountdownMode) {
            turnDuration = DEFAULT_TURN_DURATION;
            timeToBeep = (turnDuration - elapsedTime) * beepingRatio;
            fastCountdownMode = false;
          }
        }
      }
      buttonPressed = false;
      break;
    default:
      break;
  }
}
