/*
  Drizzle-Dome: Automated Clothes Retrieval System

  This sketch controls an automated clothesline retraction system using an Arduino Uno.

*/

// --- Pin Definitions ---
// L298D Motor Driver connections - both motors work together
const int motor1A = 7;        // IN1 on L298D - Motor 1 Direction Pin 1
const int motor1B = 6;        // IN2 on L298D - Motor 1 Direction Pin 2
const int motor1Enable = 9;   // ENA on L298D - Motor 1 Speed (PWM)

const int motor2A = 5;        // IN3 on L298D - Motor 2 Direction Pin 1  
const int motor2B = 4;        // IN4 on L298D - Motor 2 Direction Pin 2
const int motor2Enable = 10;  // ENB on L298D - Motor 2 Speed (PWM)

const int rainSensorPin = 2;  // Digital pin for rain sensor
const int statusLED = 13;     // Built-in LED for status indication

// --- System Variables ---
unsigned long retractionDelay = 10000;  // Wait 10s after rain stops before extending
unsigned long motorRunTime = 5000;      // Time needed for full retraction/extension
unsigned long lastRainTime = 0;         // Last time rain was detected
unsigned long motorStartTime = 0;       // When motors started running
bool isRetracted = false;               // Current clothesline state
bool motorsRunning = false;             // Are motors currently active?

// System states
enum SystemState {
  EXTENDED,     // Clothesline is out and ready
  RETRACTING,   // Motors pulling clothesline in
  RETRACTED,    // Clothesline is safely inside
  EXTENDING     // Motors pushing clothesline out
};

SystemState currentState = EXTENDED;

void setup() {
  // Configure motor driver pins
  pinMode(motor1A, OUTPUT);
  pinMode(motor1B, OUTPUT);
  pinMode(motor1Enable, OUTPUT);
  pinMode(motor2A, OUTPUT);
  pinMode(motor2B, OUTPUT);
  pinMode(motor2Enable, OUTPUT);
  
  // Configure sensor and status pins
  pinMode(rainSensorPin, INPUT);
  pinMode(statusLED, OUTPUT);

  // Set motor speeds to 150 for quieter operation (~59% speed)
  analogWrite(motor1Enable, 150);
  analogWrite(motor2Enable, 150);

  // Ensure motors start stopped
  stopMotors();
  
  Serial.begin(9600);
  Serial.println("Drizzle-Dome System Initialized ");
  Serial.println("Clothesline in EXTENDED position");
  
  digitalWrite(statusLED, HIGH); // LED on = extended
}

void loop() {
  // Read rain sensor
  int rainStatus = digitalRead(rainSensorPin);
  bool rainDetected = (rainStatus == HIGH);
  
  // Update last rain time if rain is detected
  if (rainDetected) {
    lastRainTime = millis();
  }
  
  // Main state machine - NON-BLOCKING
  switch (currentState) {
    case EXTENDED:
      handleExtendedState(rainDetected);
      break;
      
    case RETRACTING:
      handleRetractingState(rainDetected);
      break;
      
    case RETRACTED:
      handleRetractedState(rainDetected);
      break;
      
    case EXTENDING:
      handleExtendingState(rainDetected);
      break;
  }
  
  // Update status LED
  updateStatusLED();
  
  // Small delay for stability
  delay(100);
}

// Handle EXTENDED state - waiting for rain
void handleExtendedState(bool rainDetected) {
  if (rainDetected) {
    Serial.println("Rain detected! Starting retraction...");
    startRetractionMotors();
    currentState = RETRACTING;
    motorStartTime = millis();
    isRetracted = false;
  }
}

// Handle RETRACTING state - motors pulling clothesline in
void handleRetractingState(bool rainDetected) {
  // Check if retraction time is complete
  if (millis() - motorStartTime >= motorRunTime) {
    stopMotors();
    currentState = RETRACTED;
    isRetracted = true;
    Serial.println("Clothesline fully retracted and safe!");
  }
  // Continue running motors until time is up
}

// Handle RETRACTED state - waiting for rain to stop
void handleRetractedState(bool rainDetected) {
  // Only extend if no rain for the specified delay period
  if (!rainDetected && (millis() - lastRainTime >= retractionDelay)) {
    Serial.println("No rain detected for safe period. Starting extension...");
    startExtensionMotors();
    currentState = EXTENDING;
    motorStartTime = millis();
  }
}

// Handle EXTENDING state - motors pushing clothesline out
void handleExtendingState(bool rainDetected) {
  // EMERGENCY: If rain detected during extension, immediately retract!
  if (rainDetected) {
    Serial.println("EMERGENCY: Rain detected during extension! Retracting immediately!");
    startRetractionMotors();
    currentState = RETRACTING;
    motorStartTime = millis();
    return;
  }
  
  // Check if extension time is complete
  if (millis() - motorStartTime >= motorRunTime) {
    stopMotors();
    currentState = EXTENDED;
    isRetracted = false;
    Serial.println("Clothesline fully extended and ready for use!");
  }
}

// Start both motors in RETRACTION direction (both motors pull together)
void startRetractionMotors() {
  // Both motors rotate in same direction to retract clothesline
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, LOW);
  motorsRunning = true;
  Serial.println("Motors: RETRACTING clothesline...");
}

// Start both motors in EXTENSION direction (both motors push together)  
void startExtensionMotors() {
  // Both motors rotate in opposite direction to extend clothesline
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, HIGH);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, HIGH);
  motorsRunning = true;
  Serial.println("Motors: EXTENDING clothesline...");
}

// Stop all motors
void stopMotors() {
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, LOW);
  motorsRunning = false;
  Serial.println("Motors: STOPPED");
}

// Update status LED based on current state
void updateStatusLED() {
  switch (currentState) {
    case EXTENDED:
      digitalWrite(statusLED, HIGH);  // Solid ON when ready
      break;
    case RETRACTED:
      digitalWrite(statusLED, LOW);   // OFF when retracted
      break;
    case RETRACTING:
    case EXTENDING:
      // Blink rapidly during motor operation
      digitalWrite(statusLED, (millis() / 200) % 2);
      break;
  }
}
