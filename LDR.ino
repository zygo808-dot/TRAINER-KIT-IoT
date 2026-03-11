#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// ================= WIFI =================
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ================= MQTT =================
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* topic = "ldr/TA";
const char* topicDigital = "ldr_digital/TA";   // TAMBAHAN

// ================= LDR =================
#define LDR_PIN 35   // analog
#define LDR_DIGITAL 26   // TAMBAHAN pin digital

// ================= OUTPUT =================
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

// ================= BACA LDR =================
int readLDR() {
  return analogRead(LDR_PIN);  // 0 - 4095
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_DIGITAL, INPUT);   // TAMBAHAN

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
      client.connect("ESP32_LDR_Client");
    }
  }

  client.loop();

  // ===== BACA SENSOR =====
  if (millis() - lastSensorRead > 2000) {
    lastSensorRead = millis();

    int ldrValue = readLDR();
    int ldrDigital = digitalRead(LDR_DIGITAL);   // TAMBAHAN

    // ===== LOGIKA OUTPUT =====
    if (ldrValue > 2000) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      myServo.write(90);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      myServo.write(0);
    }

    // ===== MQTT ANALOG =====
    if (client.connected()) {
      String payload = String(ldrValue);
      client.publish(topic, payload.c_str());
    }

    // ===== MQTT DIGITAL =====
    if (client.connected()) {
      String payloadDigital = String(ldrDigital);
      client.publish(topicDigital, payloadDigital.c_str());
    }

    // ===== LCD BARIS 1 =====
    lcd.setCursor(0, 0);
    lcd.print("LDR: ");
    lcd.print(ldrValue);
    lcd.print("    ");
  }

  // ===== LCD BARIS 2 =====
  lcd.setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("W:ON  ");
  } else {
    lcd.print("W:OFF ");
  }
}
