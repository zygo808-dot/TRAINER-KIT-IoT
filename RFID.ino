#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define SS_PIN 5
#define RST_PIN 27
#define SERVO_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo myServo;

const char* ssid = "rdm";
const char* password = "0987654321";
String serverName = "http://192.168.1.106/savelog.php";

String allowedUID[] = {"77292125", "15B75FE0"};
int totalUID = 2;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  SPI.begin();
  mfrc522.PCD_Init();

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  myServo.attach(SERVO_PIN);
  myServo.write(0); // posisi terkunci

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);
  lcd.clear();
  lcd.print("Tempel Kartu");
}

// ================= LOOP =================
void loop() {

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uid = "";

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }

  uid.toUpperCase();
  Serial.println("UID: " + uid);

  lcd.clear();

  bool accessGranted = false;

  for (int i = 0; i < totalUID; i++) {
    if (uid == allowedUID[i]) {
      accessGranted = true;
      break;
    }
  }

  if (accessGranted) {

    lcd.print("Access Granted");
    myServo.write();   // buka kunci
    sendData(uid, "GRANTED");
    delay(3000);
    myServo.write(0);    // tutup kembali

  } else {

    lcd.print("Access Denied");
    sendData(uid, "DENIED");
    delay(2000);
  }

  lcd.clear();
  lcd.print("Tempel Kartu");
}

// ================= KIRIM DATA =================
void sendData(String uid, String status) {

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data = "uid=" + uid + "&status=" + status;

    int httpResponseCode = http.POST(data);

    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);

    http.end();
  }
}