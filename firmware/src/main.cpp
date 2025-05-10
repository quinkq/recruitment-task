#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Pin definitions
#define BUTTON_PIN 2       // Push button pin
#define RELAY_PIN 3        // Relay control pin
#define LED_PIN 4          // LED indicator pin
#define BATTERY_PIN A0     // Battery voltage monitoring pin

// Constants
#define DEBOUNCE_DELAY 50          // Button debounce delay in ms
#define LONG_PRESS_DURATION 600    // Duration for long press detection in ms
#define LOW_BATTERY_THRESHOLD 3.0  // Low battery threshold in volts
#define SERIAL_BAUD_RATE 115200    // Serial communication baud rate

// Variables for button state
volatile bool buttonInterruptFlag = false;  // Flag for interrupt
bool lastButtonState = HIGH;                // Previous button state (HIGH = not pressed)
bool currentButtonState = HIGH;             // Current button state
unsigned long lastDebounceTime = 0;         // Last time button state changed
unsigned long buttonPressTime = 0;          // When button was pressed
bool isLongPress = false;                   // Flag for long press detection
bool buttonReleased = true;                 // Flag to detect button release

// Variables for device state
bool deviceOn = false;                  // Device OFF initially
bool frequencyIncrementing = true;      // Direction of frequency change
unsigned int toggleFrequency = 3;       // Relay toggle frequency in seconds
unsigned long lastDeviceToggleTime = 0; // Last time device was toggled
unsigned long lastToggleTime = 0;       // Last time relay was toggled
bool relayState = HIGH;                 // Current relay state (LOW = relay energized/ON, HIGH = relay OFF)
bool lowBatteryDetected = false;        // Low battery flag
unsigned long lastBatteryCheckTime = 0; // Last battery check time
unsigned long lastBlinkTime = 0;        // For LED blinking
volatile bool wakeUpFlag = false;       // Flag to indicate waking from sleep

// Declarations
void toggleDevice(bool forceRelayActivation = false);
void toggleRelay();
void handleButton();
void changeFrequency();
void updateLEDIndicator();
void checkBatteryVoltage();
void enterPowerSavingMode();
void exitPowerSavingMode();

// Interrupt service routine - handle button presses for wakeups
void buttonISR() {
  buttonInterruptFlag = true;
  if (!deviceOn) {
    wakeUpFlag = true; // Set flag to indicate we're waking from sleep
  }
  sleep_disable(); // Wake up from sleep immediately
}



void setup() {
  // Initialize serial for debugging
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println(F("Relay Controller Starting..."));
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BATTERY_PIN, INPUT);
  
  // Initialize outputs - Start with device OFF
  digitalWrite(RELAY_PIN, HIGH);  // Relay is OFF when HIGH
  digitalWrite(LED_PIN, LOW);     // LED is OFF initially
  
  // Setup button interrupt
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);
  
  Serial.println(F("System initialized. Device OFF."));
  
  // Enter sleep mode immediately after initialization
  delay(100); // Delay for serial output to complete
  enterPowerSavingMode();
}

void loop() {
  // Check if woken up by interrupt
  if (buttonInterruptFlag) {
    delay(50);  // Another debounce
    buttonInterruptFlag = false;
    
    // If we were in sleep mode and this is a wake-up event
    if (wakeUpFlag) {
      wakeUpFlag = false;
      toggleDevice(true); // Force relay activation on wake-up
      delay(100); // Delay to stabilize
    }
  }
  
  // Process button input
  handleButton();
  
  // If device is on handle relay toggle and battery monitoring
  if (deviceOn) {
    // Check battery voltage every 60 seconds (ORIGINALLY 5s - that's what description says)
    if (millis() - lastBatteryCheckTime >= 60000) {
      checkBatteryVoltage();
      lastBatteryCheckTime = millis();
    }
    
    // Handle relay toggling based on frequency
    if (millis() - lastToggleTime >= (toggleFrequency * 1000)) {
      toggleRelay();
      lastToggleTime = millis();
    }
    
    // Handle LED indicators
    updateLEDIndicator();
    
  // If Device off, go to sleep
  } else { 
    // Only enter sleep mode if device was toggled more than 500ms ago to avoid immediately going back to sleep after wakeup
    if (millis() - lastDeviceToggleTime > 500) {
      enterPowerSavingMode();
    }
  }
}

void handleButton() {
  // Read the button state
  int reading = digitalRead(BUTTON_PIN);
  
  // Check if button state changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // If button state is stable (debounced)
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // If button state has changed
    if (reading != currentButtonState) {
      currentButtonState = reading;
      
      // Button pressed (LOW when pressed due to INPUT_PULLUP)
      if (currentButtonState == LOW && buttonReleased) {
        buttonPressTime = millis();
        buttonReleased = false;
        isLongPress = false;
        
        Serial.println(F("Button pressed"));
      }
      
      // Button released
      else if (currentButtonState == HIGH && !buttonReleased) {
        // Check if it was a short press (for toggling device ON/OFF)
        if (!isLongPress && (millis() - buttonPressTime < LONG_PRESS_DURATION) && !wakeUpFlag) {
          toggleDevice();
          Serial.println(F("Short press detected - Toggling device"));
        } else {
          Serial.println(F("Long press ended"));
        }
        
        buttonReleased = true;
        isLongPress = false;
      }
    }
  }
  
  // Check for long press (for changing frequency)
  if (currentButtonState == LOW && !buttonReleased && !isLongPress) {
    if (millis() - buttonPressTime >= LONG_PRESS_DURATION) {
      isLongPress = true;
      changeFrequency();
      Serial.println(F("Long press detected - Changing frequency"));
    }
  }
  
  // Continue to change frequency with button is held
  if (isLongPress && (millis() - buttonPressTime) % LONG_PRESS_DURATION == 0) {
    changeFrequency();
  }
  
  lastButtonState = reading;
}

// Toggle device state (ON/OFF)
void toggleDevice(bool forceRelayActivation) {
  lastDeviceToggleTime = millis();
  
  deviceOn = !deviceOn;
  
  if (deviceOn) {
    Serial.println(F("Device turned ON"));
    
    // Exit power saving mode
    exitPowerSavingMode();
    
    // Reset timers
    lastBatteryCheckTime = millis();
    checkBatteryVoltage(); // Check battery right away
    
    // Handle relay state on turn-on
    if (forceRelayActivation) {
      // Immediately activate relay when turning on
      relayState = LOW; // LOW = relay ON
      digitalWrite(RELAY_PIN, relayState);
      Serial.println(F("Relay activated on device startup"));
    } else {
      // Start with relay off, will toggle according to timing
      relayState = HIGH; // HIGH = relay OFF
      digitalWrite(RELAY_PIN, relayState);
    }
    
    // Reset toggle timer AFTER setting relay state
    lastToggleTime = millis();
    
  } else {
    Serial.println(F("Device turned OFF"));
    
    // Turn off relay and LED when device is off
    relayState = HIGH;  // HIGH = relay OFF
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println(F("Relay forced OFF"));
  }
}

void changeFrequency() {
  // Flash LED to indicate frequency change
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);  
  
  // Change frequency value (cycling between 1 and 10)
  if (frequencyIncrementing) {
    toggleFrequency++;
    if (toggleFrequency >= 10) {
      frequencyIncrementing = false;
    }
  } else {
    toggleFrequency--;
    if (toggleFrequency <= 1) {
      frequencyIncrementing = true;
    }
  }
  
  Serial.print(F("Toggle frequency changed to: "));
  Serial.print(toggleFrequency);
  Serial.println(F(" second(s)"));
}

void toggleRelay() {
  // Toggle relay state (HIGH = relay OFF, LOW = relay ON)
  relayState = !relayState;
  digitalWrite(RELAY_PIN, relayState);
  
  Serial.print(F("Relay toggled to: "));
  Serial.println(relayState ? F("OFF (Open)") : F("ON (Closed)"));
}

void updateLEDIndicator() {
  // Handle low battery blinking
  if (lowBatteryDetected) {
    // Quick blinking for low battery (200ms on, 200ms off)
    if (millis() - lastBlinkTime >= 200) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastBlinkTime = millis();
    }
  } else {
    // LED follows relay state (LED is ON when relay is ON)
    digitalWrite(LED_PIN, !relayState);
  }
}

void checkBatteryVoltage() {
  // Read battery voltage
  int rawValue = analogRead(BATTERY_PIN);
  
  // Calculate voltage (with voltage divider factor of 2 - 100k/100k ohm divider)
  float voltage = ((rawValue / 1023.0) * 5.0) * 2;
  
  Serial.print(F("Battery voltage: "));
  Serial.print(voltage, 2);
  Serial.println(F("V"));
  
  // Check if battery is low
  if (voltage < LOW_BATTERY_THRESHOLD) {
    if (!lowBatteryDetected) {
      Serial.println(F("WARNING: Low battery detected!"));
      lowBatteryDetected = true;
    }
  } else {
    if (lowBatteryDetected) {
      Serial.println(F("Battery level normal"));
      lowBatteryDetected = false;
    }
  }
}

void enterPowerSavingMode() {
  Serial.println(F("Entering sleep mode..."));
  delay(100); // Give serial time to send
  
  // Disable ADC to save power
  ADCSRA &= ~(1 << ADEN);
  
  // Configure sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  
  // Enable sleep mode
  sleep_enable();
  
  // Put the device to sleep
  sleep_mode();
  
  // Code execution continues here after wake-up
  
  // Disable sleep mode after waking up
  sleep_disable();
  
  Serial.println(F("Woke up from sleep mode"));
}

void exitPowerSavingMode() {
  // Re-enable ADC
  ADCSRA |= (1 << ADEN);
  
  Serial.println(F("Power saving mode exited"));
}