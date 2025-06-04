#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);


const int irSensor1 = 2;   
const int irSensor2 = 3;   
const int fanPin = 4;      
const int pumpPin = 5; 

const unsigned long DRYER_DURATION = 6000;  
const unsigned long SANITIZER_DURATION = 300;
const unsigned long SEQUENCE_TIMEOUT = 2000;  

bool sensor1Detected = false;
bool sensor2Detected = false;
unsigned long detectionTime1 = 0;
unsigned long detectionTime2 = 0;
unsigned long deviceStartTime = 0;
bool dryerRunning = false;
bool sanitizerRunning = false;
bool pendingCommand = false;
int pendingDevice = 0;

void setup() {

  pinMode(irSensor1, INPUT);
  pinMode(irSensor2, INPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  
  digitalWrite(fanPin, LOW);
  digitalWrite(pumpPin, LOW);
  
  initializeLCD();
  displayInstructions();

  Serial.begin(9600);
}

void loop() {

  if (!dryerRunning && !sanitizerRunning) {
    checkSensors();
  }
  
  if (dryerRunning && (millis() - deviceStartTime >= DRYER_DURATION)) {
    stopDryer();
    checkPendingCommand();
  }
  
  if (sanitizerRunning && (millis() - deviceStartTime >= SANITIZER_DURATION)) {
    stopSanitizer();
    checkPendingCommand();
  }
  
  delay(50);
}

void checkSensors() {

  if (digitalRead(irSensor1) == LOW) {
    if (!sensor1Detected) {
      sensor1Detected = true;
      detectionTime1 = millis();
      Serial.println("Left sensor triggered");
    }
  }
  
  if (digitalRead(irSensor2) == LOW) {
    if (!sensor2Detected) {
      sensor2Detected = true;
      detectionTime2 = millis();
      Serial.println("Right sensor triggered");
    }
  }
  

  if (sensor1Detected && sensor2Detected) {
    if (detectionTime1 < detectionTime2 && 
        (detectionTime2 - detectionTime1) <= SEQUENCE_TIMEOUT) {
 
      if (!dryerRunning && !sanitizerRunning) {
        startDryer();
      } else {
        pendingCommand = true;
        pendingDevice = 1;
        lcd.clear();
        lcd.print("Command Queued:");
        lcd.setCursor(0,1);
        lcd.print("Dryer after finish");
        delay(1000);
      }
    } 
    else if (detectionTime2 < detectionTime1 && 
             (detectionTime1 - detectionTime2) <= SEQUENCE_TIMEOUT) {

      if (!dryerRunning && !sanitizerRunning) {
        startSanitizer();
      } else {
        pendingCommand = true;
        pendingDevice = 2;
        lcd.clear();
        lcd.print("Command Queued:");
        lcd.setCursor(0,1);
        lcd.print("Sanitizer after");
        delay(1000);
      }
    }
    resetSensors();
  }
  

  if ((sensor1Detected || sensor2Detected) && 
      (millis() - max(detectionTime1, detectionTime2) > SEQUENCE_TIMEOUT)) {
    resetSensors();
  }
}

void checkPendingCommand() {
  if (pendingCommand) {
    pendingCommand = false;
    if (pendingDevice == 1) {
      startDryer();
    } else if (pendingDevice == 2) {
      startSanitizer();
    }
    pendingDevice = 0;
  } else {
    displayInstructions();
  }
}

void initializeLCD() {

  for (int i = 0; i < 3; i++) {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("Initializing...");
    delay(500);
    if (lcd.print("") == 0) {
      return;
    }
  }

  pinMode(LED_BUILTIN, OUTPUT);
  while(true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

void displayInstructions() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Swipe R->L Dryer");
  lcd.setCursor(0, 1);
  lcd.print("Swipe L->R Hand Wash");
}

void startDryer() {
  digitalWrite(fanPin, HIGH);
  dryerRunning = true;
  deviceStartTime = millis();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DRYER RUNNING");
  lcd.setCursor(0, 1);
  lcd.print("6 seconds...");
  
  resetSensors();
}

void stopDryer() {
  digitalWrite(fanPin, LOW);
  dryerRunning = false;
}

void startSanitizer() {
  digitalWrite(pumpPin, HIGH);
  sanitizerRunning = true;
  deviceStartTime = millis();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing");

  
  resetSensors();
}

void stopSanitizer() {
  digitalWrite(pumpPin, LOW);
  sanitizerRunning = false;
}

void resetSensors() {
  sensor1Detected = false;
  sensor2Detected = false;
}