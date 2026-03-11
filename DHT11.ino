#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <DHT.h>   // TAMBAHAN

// ================= WIFI =================
const char* ssid = "SSID";
const char* password = "PASSWORD";

// ================= MQTT =================
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
const char* topic = "dht11/TA";

// ================= DHT22 =================
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

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

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  lcd.init();
  lcd.backlight();

  dht.begin();

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
      client.connect("ESP32_DHT11_Client");
    }
  }

  client.loop();

  // ===== BACA SENSOR TIAP 2 DETIK =====
  if (millis() - lastSensorRead > 2000) {
    lastSensorRead = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Gagal baca DHT11");
      return;
    }

    // ===== LOGIKA OUTPUT (BERDASARKAN SUHU) =====
    if (temperature > 32) {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      myServo.write(90);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      myServo.write(0);
    }

    // ===== MQTT FORMAT CSV =====
    if (client.connected()) {
      String payload = String(temperature) + "," + String(humidity);
      client.publish(topic, payload.c_str());
    }

    // ===== LCD =====
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("H:");
    lcd.print(humidity);
    lcd.print("%   ");
  }
}
