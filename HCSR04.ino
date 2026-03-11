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
const char* topic = "hcsr04/TA";

// ===== HCSR04 =====
#define TRIG_PIN 4
#define ECHO_PIN 5

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

// ================= BACA JARAK =================
int readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = duration * 0.034 / 2;

  return round(distance);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
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
      client.connect("ESP32_HCSR04_Client");
    }
  }

  client.loop();

  // ===== BACA SENSOR =====
  if (millis() - lastSensorRead > 2000) {
    lastSensorRead = millis();

    int distance = readDistance();

    if (distance > 0 && distance <= 400) {

      // ===== LOGIKA OUTPUT =====
      if (distance <= 20) {
        digitalWrite(LED_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        myServo.write(90);
      } else {
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        myServo.write(0);
      }

      // ===== MQTT PUBLISH (ANGKA SAJA) =====
      if (client.connected()) {
        String payload = String(distance);  // kirim angka murni
        client.publish(topic, payload.c_str());
      }

      // ===== LCD BARIS 1 =====
      lcd.setCursor(0, 0);
      lcd.print("Dist: ");
      lcd.print(distance);
      lcd.print(" cm   ");
    }
  }

  // ===== LCD BARIS 2 =====
  lcd.setCursor(0, 1);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("W:ON ");
  } else {
    lcd.print("W:OFF");
  }
}
