/*
 * Smart Fully Automated Vertical Hydroponic System
 * Author : Samuel Nathan Bobai
 * Platform: ESP32
 * Description:
 *   Monitors pH, water level, and nutrient concentration.
 *   Autonomously controls dosing pumps and grow-light relay
 *   to maintain optimal growing conditions.
 *
 * Wiring:
 *   pH Sensor (analog)     → GPIO34
 *   Water Level (ultrasonic)→ Trig GPIO26 / Echo GPIO27
 *   TDS/Nutrient Sensor    → GPIO35
 *   Dosing Pump A (pH Up)  → GPIO18  (relay IN1)
 *   Dosing Pump B (pH Dn)  → GPIO19  (relay IN2)
 *   Dosing Pump C (Nutrient)→ GPIO21  (relay IN3)
 *   Grow Light Relay       → GPIO22  (relay IN4)
 *   DHT22 Temp/Humidity    → GPIO4
 *   I2C LCD 16x2           → SDA GPIO21 / SCL GPIO22
 *
 * Libraries required:
 *   DHT sensor library (Adafruit)
 *   LiquidCrystal_I2C
 */

#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ── Pin Definitions ──────────────────────────────────────
#define PH_PIN          34
#define TRIG_PIN        26
#define ECHO_PIN        27
#define TDS_PIN         35
#define DHT_PIN          4
#define DHT_TYPE        DHT22

#define PUMP_PH_UP      18
#define PUMP_PH_DOWN    19
#define PUMP_NUTRIENT   21
#define RELAY_GROW_LIGHT 22

// ── Thresholds ────────────────────────────────────────────
#define PH_MIN          5.8f
#define PH_MAX          6.5f
#define TDS_MIN         800     // ppm  (nutrient target)
#define TDS_MAX         1200    // ppm
#define WATER_LOW_CM    20      // trigger refill below 20 cm
#define LIGHT_ON_HOUR    6      // 06:00 lights on  (use RTC for real deployment)
#define LIGHT_OFF_HOUR  20      // 20:00 lights off

// ── Calibration constants (adjust per sensor lot) ────────
#define PH_OFFSET        0.0f   // volts offset from calibration solution
#define TDS_FACTOR       0.5f   // conversion factor

// ── Timing ────────────────────────────────────────────────
#define SENSOR_INTERVAL  5000UL   // read sensors every 5 s
#define PUMP_DOSE_MS     1000UL   // run dosing pump for 1 s per correction
#define LIGHT_CHECK_MS  60000UL   // check light schedule every 60 s

// ── Objects ───────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ── State ─────────────────────────────────────────────────
unsigned long lastSensorRead  = 0;
unsigned long lastLightCheck  = 0;
float         currentPH       = 0;
float         currentTDS      = 0;
float         currentTemp     = 0;
float         currentHumidity = 0;
float         waterLevelCm    = 0;
bool          lightOn         = false;

// ── Function Prototypes ───────────────────────────────────
float  readPH();
float  readTDS();
float  readWaterLevel();
void   regulatePH(float ph);
void   regulateNutrients(float tds);
void   controlGrowLight();
void   pulsePump(int pin, unsigned long duration);
void   updateLCD();
void   logToSerial();

// ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("=== Hydroponic System Booting ===");

  // Relay pins: HIGH = OFF (active-low relay board)
  pinMode(PUMP_PH_UP,      OUTPUT); digitalWrite(PUMP_PH_UP,      HIGH);
  pinMode(PUMP_PH_DOWN,    OUTPUT); digitalWrite(PUMP_PH_DOWN,    HIGH);
  pinMode(PUMP_NUTRIENT,   OUTPUT); digitalWrite(PUMP_NUTRIENT,   HIGH);
  pinMode(RELAY_GROW_LIGHT,OUTPUT); digitalWrite(RELAY_GROW_LIGHT,HIGH);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();

  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0); lcd.print("Hydro System v1");
  lcd.setCursor(0,1); lcd.print("Initializing...");
  delay(2000);

  Serial.println("System ready.");
}

// ─────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // ── Read sensors on interval ──
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;

    currentPH       = readPH();
    currentTDS      = readTDS();
    currentTemp     = dht.readTemperature();
    currentHumidity = dht.readHumidity();
    waterLevelCm    = readWaterLevel();

    // ── CoT decision chain ──
    regulatePH(currentPH);
    regulateNutrients(currentTDS);
    updateLCD();
    logToSerial();
  }

  // ── Light schedule ──
  if (now - lastLightCheck >= LIGHT_CHECK_MS) {
    lastLightCheck = now;
    controlGrowLight();
  }
}

// ─────────────────────────────────────────────────────────
// pH reading: ESP32 ADC → voltage → pH via linear map
float readPH() {
  int   raw     = analogRead(PH_PIN);
  float voltage = raw * (3.3f / 4095.0f) + PH_OFFSET;
  // Typical probe: pH = -5.70 * V + 21.34 (calibrate with buffers)
  float ph = -5.70f * voltage + 21.34f;
  return constrain(ph, 0.0f, 14.0f);
}

// TDS reading: analog voltage → ppm
float readTDS() {
  int   raw        = analogRead(TDS_PIN);
  float voltage    = raw * (3.3f / 4095.0f);
  float tdsValue   = (133.42f * voltage * voltage * voltage
                    - 255.86f * voltage * voltage
                    + 857.39f * voltage) * TDS_FACTOR;
  return tdsValue;
}

// Ultrasonic water level (returns cm from sensor to water surface)
float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  return (duration == 0) ? 999.0f : duration * 0.034f / 2.0f;
}

// ─────────────────────────────────────────────────────────
// Chain-of-Thought pH regulation
void regulatePH(float ph) {
  if (ph < PH_MIN) {
    Serial.printf("[pH] %.2f < %.2f → dosing pH UP\n", ph, PH_MIN);
    pulsePump(PUMP_PH_UP, PUMP_DOSE_MS);
  } else if (ph > PH_MAX) {
    Serial.printf("[pH] %.2f > %.2f → dosing pH DOWN\n", ph, PH_MAX);
    pulsePump(PUMP_PH_DOWN, PUMP_DOSE_MS);
  } else {
    Serial.printf("[pH] %.2f in range [%.1f–%.1f] → no action\n", ph, PH_MIN, PH_MAX);
  }
}

// Chain-of-Thought nutrient regulation
void regulateNutrients(float tds) {
  if (waterLevelCm > WATER_LOW_CM) {
    Serial.println("[Nutrient] Water level LOW — skipping dose to avoid overdose");
    return;
  }
  if (tds < TDS_MIN) {
    Serial.printf("[Nutrient] TDS %.0f ppm < %d ppm → dosing nutrients\n", tds, TDS_MIN);
    pulsePump(PUMP_NUTRIENT, PUMP_DOSE_MS);
  } else if (tds > TDS_MAX) {
    Serial.printf("[Nutrient] TDS %.0f ppm > %d ppm → dilution needed\n", tds, TDS_MAX);
    // In a full system, trigger a fresh-water valve here
  } else {
    Serial.printf("[Nutrient] TDS %.0f ppm in range [%d–%d] → no action\n", tds, TDS_MIN, TDS_MAX);
  }
}

// Simulated hour-based light control (replace with DS3231 RTC for production)
void controlGrowLight() {
  // Placeholder: millis()-based fake hour (1 real min = 1 fake hour for testing)
  int fakeHour = (millis() / 60000UL) % 24;
  bool shouldBeOn = (fakeHour >= LIGHT_ON_HOUR && fakeHour < LIGHT_OFF_HOUR);
  if (shouldBeOn != lightOn) {
    lightOn = shouldBeOn;
    digitalWrite(RELAY_GROW_LIGHT, lightOn ? LOW : HIGH); // active-low
    Serial.printf("[Light] Grow light %s\n", lightOn ? "ON" : "OFF");
  }
}

// Activate a pump for a set duration then turn it off
void pulsePump(int pin, unsigned long duration) {
  digitalWrite(pin, LOW);   // active-low: ON
  delay(duration);
  digitalWrite(pin, HIGH);  // OFF
}

// ─────────────────────────────────────────────────────────
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("pH:%.2f TDS:%dppm", currentPH, (int)currentTDS);
  lcd.setCursor(0, 1);
  lcd.printf("T:%.1fC H:%.0f%%", currentTemp, currentHumidity);
}

void logToSerial() {
  Serial.println("─────────────────────────");
  Serial.printf("pH        : %.2f\n",  currentPH);
  Serial.printf("TDS       : %.0f ppm\n", currentTDS);
  Serial.printf("Temp      : %.1f °C\n", currentTemp);
  Serial.printf("Humidity  : %.0f %%\n", currentHumidity);
  Serial.printf("Water lvl : %.1f cm\n", waterLevelCm);
  Serial.printf("Light     : %s\n",    lightOn ? "ON" : "OFF");
  Serial.println("─────────────────────────");
}
