# RFID Attendance System — ESP32 + RC522

A complete RFID-based attendance logger using ESP32, MFRC522, Green/Red LEDs and a Buzzer.

## Hardware Required
| Component | Qty |
|-----------|-----|
| ESP32 Dev Board | 1 |
| MFRC522 RC522 RFID Module | 1 |
| RFID Card (13.56 MHz) | 1 |
| RFID Key Tag (13.56 MHz) | 1 |
| Green LED | 1 |
| Red LED | 1 |
| Buzzer (passive or active) | 1 |
| 220Ω Resistor | 2 |
| Breadboard + Jumper Wires | — |

## Wiring

### RC522 → ESP32
| RC522 Pin | ESP32 Pin |
|-----------|-----------|
| SDA (SS) | GPIO 21 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| 3.3V | 3.3V |
| GND | GND |

### LEDs & Buzzer
| Component | ESP32 Pin |
|-----------|-----------|
| Green LED (+ resistor) | GPIO 26 |
| Red LED (+ resistor) | GPIO 27 |
| Buzzer | GPIO 25 |

## Arduino IDE Setup

1. Install **Arduino IDE**
2. Add ESP32 board: `File → Preferences → Additional Boards Manager URLs`
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Install board: `Tools → Board → Boards Manager → search "esp32" → Install`
4. Install library: `Sketch → Include Library → Manage Libraries → search "MFRC522" → Install`
5. Select Board: `Tools → Board → ESP32 Dev Module`
6. Select Port: `Tools → Port → COMx`

## Step 1 — Find your Card & Tag UIDs

1. Open `rfid_attendance.ino`
2. Upload as-is (UIDs don't matter yet)
3. Open Serial Monitor at **115200 baud**
4. Scan your **RFID card** → note the UID (e.g. `A1:B2:C3:D4`)
5. Scan your **RFID tag** → note the UID
6. Update the `authorizedUsers[]` array:

```cpp
RFIDUser authorizedUsers[] = {
  { {0xA1, 0xB2, 0xC3, 0xD4}, "Card - Alice" },   // YOUR CARD UID
  { {0x11, 0x22, 0x33, 0x44}, "Tag  - Bob"   },   // YOUR TAG  UID
};
```

7. Upload again → Done!

## How It Works

| Event | Green LED | Red LED | Buzzer |
|-------|-----------|---------|--------|
| Startup | Blinks 3x | — | 3 ascending beeps |
| Access Granted (IN) | 2 blinks + stays on | — | 2 short happy beeps |
| Access Granted (OUT) | Same | — | Same |
| Access Denied | — | ON | 2 long low beeps |

## Serial Monitor Commands

| Key | Action |
|-----|--------|
| `L` | Print attendance log table |
| `H` | Show help |

## Attendance Log Example
```
╔══════════════════════════════════════════════╗
║        ATTENDANCE LOG (4 records)            ║
╠══════╦══════════════╦══════════╦═════════════╣
║  #   ║  Name        ║  Status  ║  Time (s)   ║
╠══════╬══════════════╬══════════╬═════════════╣
║   1  ║ Card - Alice  ║   IN     ║          12  ║
║   2  ║ Tag  - Bob    ║   IN     ║          45  ║
║   3  ║ Card - Alice  ║   OUT    ║         120  ║
║   4  ║ Tag  - Bob    ║   OUT    ║         200  ║
╚══════╩══════════════╩══════════╩═════════════╝
```

## Project Structure
```
iot rfid/
├── rfid_attendance/
│   └── rfid_attendance.ino   ← Main sketch
└── README.md                  ← This file
```
