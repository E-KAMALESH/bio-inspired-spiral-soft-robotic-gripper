#include <Wire.h>
#include <AccelAndGyro.h>
#include <oled.h>
#include <ESP32Servo.h>

// ── APDS9960 Registers 
#define APDS9960_ADDR   0x39
#define REG_ID          0x92
#define REG_ENABLE      0x80
#define REG_PPULSE      0x8E
#define REG_CONTROL     0x8F
#define REG_PDATA       0x9C

// ── Hardware 
#define SERVO_PIN       18
#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// ── SG90 Servo Angles 
#define SERVO_OPEN      10
#define SERVO_CLOSE     120
#define SERVO_STEP      3       // FIXED: was 1 (too slow)

// ── Proximity & Tilt 
#define PROX_DETECT     2       // FIXED: was 6, sensor only reaches 2-5
#define TARGET_ANGLE    35.0f   // FIXED: was 20.0, idle noise floor is ~10-19 deg
#define HOLD_TIME_MS    5000UL
#define CURL_TIMEOUT_MS 3000UL
#define MIN_CURL_MS     800UL   // FIXED: don't check tilt for first 800ms of curl

// ── Low-pass filter alpha 
#define TILT_FILTER     0.25f   // FIXED: was 0.15

// ── Objects 
AccelAndGyro mpu;
oLed         oled(OLED_WIDTH, OLED_HEIGHT);
Servo        gripperServo;

// ── State Machine 
enum GripperState { IDLE, OBJECT_DETECTED, CURLING, HOLDING, RELEASING };
GripperState state = IDLE;
const char* STATE_LABEL[] = { "IDLE", "DETECTED", "CURLING", "HOLDING", "RELEASING" };

// ── Runtime Variables
uint8_t       proximityValue  = 0;
float         tiltAngle       = 0.0f;
float         filteredTilt    = 0.0f;
int           servoAngle      = SERVO_OPEN;
unsigned long holdStart       = 0;
unsigned long curlStart       = 0;
unsigned long lastDisplayMs   = 0;
unsigned long lastSerialMs    = 0;

//  APDS9960 raw I2C

uint8_t apdsRead(uint8_t reg) {
  Wire.beginTransmission(APDS9960_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(APDS9960_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

void apdsWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(APDS9960_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

bool initAPDS9960() {
  uint8_t id = apdsRead(REG_ID);
  Serial.print("APDS9960 ID: 0x");
  Serial.println(id, HEX);
  if (id == 0x00 || id == 0xFF) return false;
  apdsWrite(REG_ENABLE,  0x05);
  apdsWrite(REG_PPULSE,  0x87);
  apdsWrite(REG_CONTROL, 0x20);
  delay(100);
  return true;
}


//  OLED

void showSplash(const char* l1, const char* l2) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 20); oled.println(l1);
  oled.setCursor(0, 34); oled.println(l2);
  oled.display();
  delay(900);
}

void updateOLED() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);

  oled.setCursor(0, 0);  oled.println("-Spiral Gripper-");
  oled.setCursor(0, 12); oled.print("Prox : "); oled.println(proximityValue);
  oled.setCursor(0, 22); oled.print("Tilt : "); oled.print(filteredTilt, 1); oled.println(" deg");
  oled.setCursor(0, 32); oled.print("Servo: "); oled.print(servoAngle); oled.println(" deg");
  oled.setCursor(0, 44); oled.print("State: "); oled.println(STATE_LABEL[state]);

  int barPx = map(servoAngle, SERVO_OPEN, SERVO_CLOSE, 0, 124);
  barPx = constrain(barPx, 0, 124);
  oled.drawRect(0, 56, 128, 7, WHITE);
  oled.fillRect(2, 58, barPx, 3, WHITE);

  oled.display();
}


//  State Machine

void runStateMachine() {
  switch (state) {

    case IDLE:
      if (servoAngle != SERVO_OPEN) {
        servoAngle = SERVO_OPEN;
        gripperServo.write(servoAngle);
      }
      if (proximityValue > PROX_DETECT) {
        state = OBJECT_DETECTED;
        Serial.println("[STATE] Object Detected");
      }
      break;

    case OBJECT_DETECTED:
      delay(200);
      if (apdsRead(REG_PDATA) > PROX_DETECT) {
        curlStart = millis();
        state = CURLING;
        Serial.println("[STATE] Curling");
      } else {
        state = IDLE;
        Serial.println("[STATE] False trigger, back to IDLE");
      }
      break;

    case CURLING:
      if (servoAngle < SERVO_CLOSE) {
        servoAngle = min(servoAngle + SERVO_STEP, (int)SERVO_CLOSE);
        gripperServo.write(servoAngle);
        Serial.print("[CURL] Writing servo: "); Serial.println(servoAngle);
      }
      // FIXED: only check tilt after MIN_CURL_MS to avoid idle noise false trigger
      if (millis() - curlStart > MIN_CURL_MS && filteredTilt >= TARGET_ANGLE) {
        holdStart = millis();
        state = HOLDING;
        Serial.println("[STATE] Holding - grip confirmed by tilt");
        break;
      }
      if (servoAngle >= SERVO_CLOSE || millis() - curlStart > CURL_TIMEOUT_MS) {
        holdStart = millis();
        state = HOLDING;
        Serial.println("[STATE] Holding - timeout/servo limit");
      }
      break;

    case HOLDING:
      if (servoAngle != SERVO_CLOSE) {
        servoAngle = SERVO_CLOSE;
        gripperServo.write(servoAngle);
      }
      if (millis() - holdStart > HOLD_TIME_MS) {
        state = RELEASING;
        Serial.println("[STATE] Releasing");
      }
      break;

    case RELEASING:
      if (servoAngle > SERVO_OPEN) {
        servoAngle = max(servoAngle - SERVO_STEP, (int)SERVO_OPEN);
        gripperServo.write(servoAngle);
      } else {
        state = IDLE;
        Serial.println("[STATE] IDLE");
      }
      break;
  }
}


//  SETUP

void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  gripperServo.setPeriodHertz(50);
  gripperServo.attach(SERVO_PIN, 500, 2400);
  gripperServo.write(SERVO_OPEN);
  servoAngle = SERVO_OPEN;
  delay(500);

  if (!oled.begin()) {
    Serial.println("[ERR] OLED"); while (1);
  }
  showSplash("MYOSA Gripper", "Starting...");

  if (!initAPDS9960()) {
    showSplash("APDS9960", "Init Failed!");
    Serial.println("[ERR] APDS9960"); while (1);
  }

  if (!mpu.begin(true)) {
    showSplash("MPU6050", "Init Failed!");
    Serial.println("[ERR] MPU6050"); while (1);
  }

  filteredTilt = abs(mpu.getTiltX(false));

  // ── Servo sweep test at startup 
  Serial.println("[TEST] Servo sweep starting...");
  showSplash("Servo Test", "Sweeping...");
  for (int a = SERVO_OPEN; a <= SERVO_CLOSE; a += 5) {
    gripperServo.write(a);
    delay(30);
  }
  delay(500);
  for (int a = SERVO_CLOSE; a >= SERVO_OPEN; a -= 5) {
    gripperServo.write(a);
    delay(30);
  }
  gripperServo.write(SERVO_OPEN);
  servoAngle = SERVO_OPEN;
  Serial.println("[TEST] Servo sweep done");

  showSplash("System Ready!", "Bring object near");
  Serial.println("[OK] Gripper Ready");
  Serial.println("State | Prox | Tilt | Servo");
}

//  LOOP — 20 Hz

void loop() {
  proximityValue = apdsRead(REG_PDATA);

  float rawTilt = abs(mpu.getTiltX(false));
  filteredTilt  = TILT_FILTER * rawTilt + (1.0f - TILT_FILTER) * filteredTilt;

  runStateMachine();

  if (millis() - lastDisplayMs >= 300) {
    updateOLED();
    lastDisplayMs = millis();
  }

  if (millis() - lastSerialMs >= 300) {
    Serial.print("State: "); Serial.print(STATE_LABEL[state]);
    Serial.print(" | Prox: "); Serial.print(proximityValue);
    Serial.print(" | Tilt: "); Serial.print(filteredTilt, 1);
    Serial.print(" | Servo: "); Serial.println(servoAngle);
    lastSerialMs = millis();
  }

  delay(50);
}
