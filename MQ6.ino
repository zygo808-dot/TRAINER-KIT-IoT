#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ===== WIFI =====
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ===== MQTT =====
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* topicAnalog  = "mq/TA";
const char* topicDigital = "mq_digital/TA";

// ===== MQ6 =====
#define MQ6_ANALOG 34
#define MQ6_DIGITAL 27

// ===== OUTPUT PIN =====
#define LED_PIN 2
#define BUZZER_PIN 15
#define SERVO_PIN 18

LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo;

unsigned long lastSensorRead = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0;

// ================= BACA GAS =================
int readGasAnalog() {
  return analogRead(MQ6_ANALOG);   // 0–4095
}

int readGasDigital() {
  return digitalRead(MQ6_DIGITAL);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(MQ6_ANALOG, INPUT);
  pinMode(MQ6_DIGITAL, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  lcd.init();
  lcd.backlight();

  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, mqtt_port);
}

// ================= LOOP =================
void loop() {

  // ===== RECONNECT WIFI =====
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt > 5000) {
      lastWiFiAttempt = millis();
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
  }

  // ===== RECONNECT MQTT =====
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    if (millis() - lastMQTTAttempt > 5000) {
      lastMQTTAttempt = millis();
      client.connect("ESP32_MQ6_Client");
    }
  }

  client.loop();

  // ===== BACA SENSOR SETIAP 2 DETIK =====
  if (millis() - lastSensorRead > 2000) {
    lastSensorRead = millis();

    int gasAnalog  = readGasAnalog();   // 0–4095
    int gasDigital = readGasDigital();  // 0 atau 1

    // ===== LOGIKA OUTPUT (ANALOG SEBAGAI ACUAN) =====
    if (gasAnalog > 2000) {   // Threshold bisa kamu ubah
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      myServo.write(90);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      myServo.write(0);
    }

    // ===== MQTT PUBLISH ANALOG =====
    if (client.connected()) {
      String payloadAnalog = String(gasAnalog);
      client.publish(topicAnalog, payloadAnalog.c_str());
    }

    // ===== MQTT PUBLISH DIGITAL =====
    if (client.connected()) {
      String payloadDigital = String(gasDigital);
      client.publish(topicDigital, payloadDigital.c_str());
    }

    // ===== LCD BARIS 1 =====
    lcd.setCursor(0, 0);
    lcd.print("ADC:");
    lcd.print(gasAnalog);
    lcd.print("   ");
  }

  // ===== LCD BARIS 2 =====
  lcd.setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("W:ON ");
  } else {
    lcd.print("W:OFF");
  }
}
