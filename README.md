# IoT RFID Attendance System — ESP32 + RC522 + Google Sheets

A complete RFID-based attendance logger using an ESP32 microcontroller,
MFRC522 reader, and Google Sheets as a real-time cloud database.
Scan a card → LED + buzzer feedback → instant cloud log. No server required.

## Features

- Real-time attendance logging to Google Sheets via Wi-Fi (HTTPS POST)
- Automatic IN / OUT toggle per registered user
- Duration tracking — calculates time spent between IN and OUT scans
- NTP time sync (UTC+5, Pakistan Standard Time)
- Offline resilience — works locally if Wi-Fi is unavailable
- Security audit log — unknown card scans are recorded as DENIED
- Debounce protection — ignores duplicate scans within 4 seconds
- LED + buzzer feedback for every event (startup, granted, denied, cloud sync)

## Hardware

| Component              | Qty |
|------------------------|-----|
| ESP32 Dev Board        | ×1  |
| MFRC522 RC522          | ×1  |
| RFID Card (13.56 MHz)  | ×1  |
| RFID Key Tag           | ×1  |
| Green LED + 220Ω       | ×1  |
| Red LED + 220Ω         | ×1  |
| Buzzer                 | ×1  |
| Breadboard + Wires     | —   |

## Quick Start

1. Clone this repo
2. Open `rfid_attendance/rfid_attendance.ino` in Arduino IDE
3. Fill in your Wi-Fi credentials and Google Sheets Web App URL
4. Upload to ESP32, open Serial Monitor at 115200 baud
5. Scan your card to find its UID, then add it to `authorizedUsers[]`
6. Upload again — done!

See the full setup guide in the README for Google Apps Script deployment.
