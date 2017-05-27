#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

const unsigned int BUZZER_PIN = PB0;
const unsigned int MOTOR_PIN = PB1;
const unsigned int BUTTON_PIN = PB2;
const unsigned int ACCELEROMETER_IN = PB3;
const unsigned int ACCELEROMETER_POWER = PB4;
const unsigned long DEBOUNCE_TIME = 50;

volatile bool watchDogBarked = false;
bool watchDogEnabled = false;

volatile bool buttonPressed = false;

enum WatchDogTimeout {
  WDT_16ms,
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
  SLEEP,
  GAME
};

PowerState currentState = SLEEP; // Start with sleep as the initial state

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
void goToDeepSleep() {
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
    case SLEEP:
      turnAccelerometerOff();
      // Go to deep sleep until an interrupt occurs
      goToDeepSleep();
      // Proceed to the
      if (buttonPressed) {
        currentState = GAME;
        buttonPressed = false;
      }
      break;
    case GAME:
      // If watchdog is not enabled, then we just arrived from SLEEP state
      if (!watchDogEnabled) {
        turnAccelerometerOn();
        // Set up watchdog to be triggered (once) after the specified time
        setupWatchDogTimeoutOneshot(WDT_1sec);
        watchDogEnabled = true;
        digitalWrite(BUZZER_PIN, HIGH);
      }
      // Go to deep sleep until an interrupt occurs
      goToDeepSleep();
      // We might have gone out of sleep due to another interrupt (i.e. the switch)
      // So make sure that we only get out of this state using the watchdog
      if (watchDogBarked) {
        digitalWrite(BUZZER_PIN, LOW);
        currentState = SLEEP;
        watchDogBarked = false;
        watchDogEnabled = false;
      }
      break;
    default:
      break;
  }
}
