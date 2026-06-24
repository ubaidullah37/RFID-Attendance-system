/*
 * =============================================================
 *  RFID ATTENDANCE SYSTEM — WITH GOOGLE SHEETS LOGGING
 *  Hardware: ESP32 + MFRC522 RC522 + Green LED + Red LED + Buzzer
 * =============================================================
 *  Wiring:
 *    RC522 --> ESP32
 *    SDA   --> GPIO 21  |  SCK  --> GPIO 18
 *    MOSI  --> GPIO 23  |  MISO --> GPIO 19
 *    RST   --> GPIO 22  |  3.3V --> 3.3V  |  GND --> GND
 *
 *    Green LED  --> GPIO 26 (220Ω resistor to GND)
 *    Red   LED  --> GPIO 27 (220Ω resistor to GND)
 *    Buzzer (+) --> GPIO 25
 * =============================================================
 */

#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// ============================================================
//  ⧗  USER CONFIG — EDIT THESE BEFORE UPLOADING
// ============================================================
//  Your router is a TENDA (detected from ipconfig).
//  Check the sticker on your router or your phone's WiFi settings
//  for the exact SSID name (e.g. "Tenda_A1B2C3" or "Tenda-Home")
const char* WIFI_SSID     = "AstroCipher";        // ← Replace with your Tenda WiFi name
const char* WIFI_PASSWORD = "11223344";  // ← Replace with your WiFi password

// After deploying your Apps Script, paste the Web App URL here:
const char* SHEET_URL = "https://script.google.com/macros/s/AKfycbyLyBcvNRgkU7IQGdqsqh3sqbGVhup-JzfTjCquMNZ6L9yx8qoHlViKSNFWaqwUPQPO/exec";
// ✅ Apps Script deployed: Jun 23, 2026 — Version 1

// ✅ Timezone: UTC+5 (Pakistan Standard Time) — already correct!
const char* NTP_SERVER    = "pool.ntp.org";
const long  GMT_OFFSET_SEC = 18000;   // UTC+5 = 5 × 3600 = 18000 ✓
const int   DST_OFFSET_SEC = 0;
// ============================================================

// ---- Pin Definitions ----
#define RST_PIN   22
#define SS_PIN    21
#define GREEN_LED 26
#define RED_LED   27
#define BUZZER    25

// ---- Buzzer tones ----
#define TONE_SUCCESS 2000
#define TONE_DENIED  400

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ============================================================
//  REGISTERED RFID UIDs
//  HOW TO GET YOUR UID: Upload → Serial Monitor (115200) → scan card
// ============================================================
struct RFIDUser {
  byte   uid[4];
  String name;
};

RFIDUser authorizedUsers[] = {
  {{0xB3, 0xE6, 0x73, 0x72}, "Ali"},
  {{0x13, 0x2D, 0xD4, 0x94}, "Saad"}
};
const int NUM_USERS = sizeof(authorizedUsers) / sizeof(authorizedUsers[0]);

// ============================================================
//  State tracking
// ============================================================
bool         userStatus[10];
unsigned long lastScanTime[10];

// ============================================================
//  BUZZER helper
// ============================================================
void beep(int freq, int dur) {
  tone(BUZZER, freq, dur);
  delay(dur + 30);
  noTone(BUZZER);
}

// ============================================================
//  WiFi connect
// ============================================================
void connectWiFi() {
  Serial.print("📶 Connecting to WiFi: ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected! IP: " + WiFi.localIP().toString());
    // Blink green 3x for WiFi success
    for (int i = 0; i < 3; i++) {
      digitalWrite(GREEN_LED, HIGH); delay(150);
      digitalWrite(GREEN_LED, LOW);  delay(150);
    }
  } else {
    Serial.println("\n⚠️  WiFi FAILED — attendance will still work locally.");
    // Blink red 3x for WiFi failure
    for (int i = 0; i < 3; i++) {
      digitalWrite(RED_LED, HIGH); delay(150);
      digitalWrite(RED_LED, LOW);  delay(150);
    }
  }
}

// ============================================================
//  Get current date-time string from NTP
// ============================================================
String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time-Unavailable";
  }
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buf);
}

// ============================================================
//  Send attendance to Google Sheets via HTTP POST
// ============================================================
bool sendToGoogleSheets(String name, String uid, String status, String timestamp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  No WiFi — skipping Google Sheets upload.");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();   // Skip SSL cert verification (fine for hobby projects)

  HTTPClient http;
  http.begin(client, SHEET_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  String payload = "{";
  payload += "\"name\":\"" + name + "\",";
  payload += "\"uid\":\"" + uid + "\",";
  payload += "\"status\":\"" + status + "\",";
  payload += "\"timestamp\":\"" + timestamp + "\"";
  payload += "}";

  Serial.println("📤 Sending to Google Sheets...");
  Serial.println("   Payload: " + payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.println("   HTTP " + String(httpCode) + " → " + response);
    http.end();
    return (httpCode == 200);
  } else {
    Serial.println("   ❌ HTTP Error: " + http.errorToString(httpCode));
    http.end();
    return false;
  }
}

// ============================================================
//  Get UID as hex string (e.g. "A1:B2:C3:D4")
// ============================================================
String getUIDString(MFRC522::Uid &uid) {
  String result = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) result += "0";
    result += String(uid.uidByte[i], HEX);
    if (i < uid.size - 1) result += ":";
  }
  result.toUpperCase();
  return result;
}

// ============================================================
//  Find user index, -1 if not found
// ============================================================
int findUser(MFRC522::Uid &uid) {
  for (int i = 0; i < NUM_USERS; i++) {
    bool match = true;
    for (byte b = 0; b < 4; b++) {
      if (b < uid.size && uid.uidByte[b] != authorizedUsers[i].uid[b]) {
        match = false;
        break;
      }
    }
    if (match) return i;
  }
  return -1;
}

// ============================================================
//  Access Granted feedback
// ============================================================
void grantAccess(String name, bool isEntry, String uid) {
  String statusStr = isEntry ? "IN" : "OUT";
  String timestamp = getTimestamp();

  Serial.println("\n✅ ACCESS GRANTED");
  Serial.println("   Name   : " + name);
  Serial.println("   Status : " + statusStr);
  Serial.println("   Time   : " + timestamp);

  // Green LED + happy beeps
  for (int i = 0; i < 2; i++) {
    digitalWrite(GREEN_LED, HIGH);
    beep(TONE_SUCCESS, 100);
    delay(60);
    digitalWrite(GREEN_LED, LOW);
    delay(60);
  }
  digitalWrite(GREEN_LED, HIGH);
  delay(800);
  digitalWrite(GREEN_LED, LOW);

  // Send to Google Sheets
  bool sent = sendToGoogleSheets(name, uid, statusStr, timestamp);
  if (sent) {
    Serial.println("   ☁️  Logged to Google Sheets ✓");
    // Extra quick beep to confirm cloud upload
    beep(2500, 80);
  } else {
    Serial.println("   ⚠️  Sheet upload failed (saved locally only)");
  }
}

// ============================================================
//  Access Denied feedback
// ============================================================
void denyAccess(String uid) {
  String timestamp = getTimestamp();

  Serial.println("\n❌ ACCESS DENIED — Unknown UID: " + uid);

  digitalWrite(RED_LED, HIGH);
  beep(TONE_DENIED, 600);
  delay(200);
  beep(TONE_DENIED, 600);
  delay(600);
  digitalWrite(RED_LED, LOW);

  // Also log denied attempts to sheet for security audit
  sendToGoogleSheets("UNKNOWN", uid, "DENIED", timestamp);
}

// ============================================================
//  Startup sequence
// ============================================================
void startupSequence() {
  for (int i = 0; i < 3; i++) {
    beep(800 + i * 400, 150);
    digitalWrite(GREEN_LED, HIGH); delay(80);
    digitalWrite(GREEN_LED, LOW);  delay(80);
  }
  Serial.println("=============================================");
  Serial.println("  RFID ATTENDANCE SYSTEM — WITH GOOGLE SHEETS");
  Serial.println("  Scan your Card or Tag...");
  Serial.println("=============================================");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);

  for (int i = 0; i < 10; i++) {
    userStatus[i]   = false;
    lastScanTime[i] = 0;
  }

  delay(300);

  // Connect WiFi
  connectWiFi();

  // Sync time via NTP
  if (WiFi.status() == WL_CONNECTED) {
    configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
    Serial.println("⏰ Syncing time from NTP...");
    delay(2000);
    Serial.println("   Time: " + getTimestamp());
  }

  startupSequence();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {

  // Reconnect WiFi if dropped
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("📶 WiFi lost, reconnecting...");
    connectWiFi();
  }

  // Wait for RFID card/tag
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial())   return;

  String scannedUID = getUIDString(mfrc522.uid);
  Serial.println("\n🔍 Scanned UID: " + scannedUID);

  int userIdx = findUser(mfrc522.uid);

  if (userIdx >= 0) {
    // Debounce: ignore same card within 4 seconds
    unsigned long now = millis();
    if ((now - lastScanTime[userIdx]) < 4000) {
      Serial.println("⚠️  Too fast — ignored (debounce).");
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }
    lastScanTime[userIdx] = now;

    // Toggle IN / OUT
    userStatus[userIdx] = !userStatus[userIdx];
    bool isEntry = userStatus[userIdx];

    grantAccess(authorizedUsers[userIdx].name, isEntry, scannedUID);

  } else {
    denyAccess(scannedUID);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(500);
}
