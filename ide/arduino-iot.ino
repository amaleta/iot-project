#include <Servo.h>
#include <TimerOne.h>

// Pin Definitions
const int MOTION_SENSOR_PIN = 2;
const int ALARM_LED_PIN = 4;
const int LED_PIN = 13;
const int LDR_PIN = A1;
const int RELAY_PIN = 7;
const int RELAY_PIN2 = 8;
const int SERVO_PIN = 5;
const int TEMP_PIN = A0;

// Constants
const unsigned long TIMEOUT = 10000;
const float TEMP_THRESHOLD = 17.0;

// Global Variables
bool motionDetected = false;
unsigned long lastMotionTime = 0;
bool emailSent = false;
bool notify = false;
bool autoMode = false;
bool motionSensorEnabled = false;
bool lampControlDisabled = false;
Servo myServo;

// Function Prototypes
void processInput(String inputAction);
void handleLDR();
void handleTemperature();
float readTemperature();
int readLight();
void handleMotionSensor();
void sendEmailNotification();
void timerISR();

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(RELAY_PIN2, OUTPUT);
  myServo.attach(SERVO_PIN);
  Serial.begin(9600);
  Timer1.initialize(10000000); // 10 seconds
  Timer1.attachInterrupt(timerISR);
}

void loop() {
  handleMotionSensor();

  if (Serial.available() > 0) {
    String action = Serial.readString();
    processInput(action);
  }

  if (autoMode && !Serial.available()) {
    handleLDR();
  }

  handleTemperature();
  delay(1000);
}

void timerISR() {
  float temp = readTemperature();
  Serial.println(temp);
  int illum = readLight();
  Serial.println(illum);
  if (notify) {
    Serial.println("NOTIFICATION");
    notify = false;
  }
}

void processInput(String inputAction) {
  if (inputAction == "ON") {
    autoMode = false;
    digitalWrite(ledPin, HIGH);
    Serial.print("The LIGHT is turned ON : ");
    Serial.println(ledPin);
  } else if (inputAction == "OFF") {
    autoMode = false;
    digitalWrite(ledPin, LOW);
    Serial.print("The LIGHT is turned OFF : ");
    Serial.println(ledPin);
  } else if (inputAction == "AUTO") {
    autoMode = true;
    Serial.println("AUTOMODE");
  } else if (inputAction == "motion_on") {
    motionSensorEnabled = true;
  } else if (inputAction == "motion_off") {
    motionSensorEnabled = false;
    digitalWrite(alarmLedPin, LOW);
  } else if (inputAction == "lamp_on") {
    lampControlDisabled = true;
    digitalWrite(relayPin, LOW);
    digitalWrite(relayPin2, HIGH);
  } else if (inputAction == "lamp_off") {
    lampControlDisabled = true;
    digitalWrite(relayPin, HIGH);
  } else if (inputAction == "lamp_auto") {
    lampControlDisabled = false;
  } else if (inputAction == "cooling_on") {
    lampControlDisabled = true;
    digitalWrite(relayPin, HIGH);
    digitalWrite(relayPin2, LOW);
  } else if (inputAction == "cooling_off") {
    lampControlDisabled = true;
    digitalWrite(relayPin2, HIGH);
  } else if (inputAction == "cooling_auto") {
    lampControlDisabled = false;
  }
}

void handleLDR() {
  int value = readLight();
  if (value <= 300) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(1000);
}

void handleTemperature() {
  if (lampControlDisabled) {
     return;
  }

  float temperature = readTemperature();

  if (temperature < TEMP_THRESHOLD) {
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(RELAY_PIN2, HIGH);
  } else if (temperature >= 23) {
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(RELAY_PIN2, LOW);
    myServo.write(180);
    delay(1000);
    myServo.write(0);
    delay(1000);
  }
}

float readTemperature() {
  int analogValue = analogRead(TEMP_PIN);
  float milliVolts = (analogValue / 1023.0) * 5000;
  float celsius = milliVolts / 10;
  return celsius;
}

int readLight() {
  return analogRead(LDR_PIN);
}

void handleMotionSensor() {
  if (!motionSensorEnabled) {
    return;
  }
  if (digitalRead(MOTION_SENSOR_PIN) == HIGH) {
    if (!motionDetected)
    {
      motionDetected = true;
      emailSent = false;
      Serial.println("Motion detected");
      digitalWrite(ALARM_LED_PIN, HIGH);
      lastMotionTime = millis();
    }
  }

  if (motionDetected && (millis() - lastMotionTime > TIMEOUT)) {
    motionDetected = false;
    digitalWrite(ALARM_LED_PIN, LOW);
    if (!emailSent) {
      sendEmailNotification();
      emailSent = true;
    }
  }
}

void sendEmailNotification() {
  notify = true;
}
