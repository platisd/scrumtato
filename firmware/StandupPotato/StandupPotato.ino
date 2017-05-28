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

const unsigned long TURN_DURATION = 120000; // The time in milliseconds that each person can talk
unsigned long halfOfRemainingTime = TURN_DURATION / 2;
volatile bool watchDogBarked = false;
bool watchDogEnabled = false;
const WatchDogTimeout gameWdt = WDT_1sec;
const WatchDogTimeout buzzerWdt = WDT_128ms;
const WatchDogTimeout motorWdt = WDT_500ms;

WatchDogTimeout currentWdt = gameWdt;
unsigned long elapsedTime = 0; // How much time has elapsed in the running turn in milliseconds

volatile bool buttonPressed = false;

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
  ADCSRA = 0; // turn off ADC
  power_all_disable (); // power off ADC, Timer 0 and 1, serial interface
  sleep_enable();
  sleep_cpu(); // Sleep here and wait for the interrupt
  sleep_disable();
  power_all_enable(); // power everything back on
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(ACCELEROMETER_IN, INPUT);
  pinMode(ACCELEROMETER_POWER, OUTPUT);
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
        unsigned long remainingTime = TURN_DURATION - elapsedTime;
        /* --- Main game logic --- */
        if (elapsedTime >= TURN_DURATION) {
          // Play a sound and vibration sequence to indicate the end of a game due to timeout
          ringFor(100);
          delay(100);
          ringFor(100);
          delay(300);
          startVibrating();
          ringFor(800);
          stopVibrating();
          currentState = DEEP_SLEEP;
          elapsedTime = 0;
          halfOfRemainingTime = TURN_DURATION / 2; // reset the remaining time since the turn is over
        } else if (remainingTime <= halfOfRemainingTime) {
          // Ring when we reach half of the remaining time available
          ringFor(150);
          halfOfRemainingTime /= 2;
        }
      }
      if (buttonPressed) {
        buttonPressed = false;
        // Ensure that a button press did not occur right after the turn finished
        // (e.g. while playing the timeout ringing sequence) and just before going to sleep
        if (currentState != DEEP_SLEEP) {
          // If the user pressed the button before their time was up, it means that the turn can finish before the time is up
          // Play a sound sequence to indicate the end of a game due to button press
          ringFor(50);
          delay(100);
          ringFor(50);
          delay(100);
          ringFor(50);
          // We don't need the watchdog running anymore, so disable it and reset the related variables
          wdt_disable(); // disable watchdog
          watchDogBarked = false;
          watchDogEnabled = false;
          elapsedTime = 0;
          halfOfRemainingTime = TURN_DURATION / 2; // reset the remaining time since the turn is over
          // Go back to the sleep state
          currentState = DEEP_SLEEP;
        }
      }
      break;
    default:
      break;
  }
}
