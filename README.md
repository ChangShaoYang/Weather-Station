# 🌤️ ESP32 Offline Weather Station

An offline-capable environmental monitoring station using Arduino Mega + ESP32. Sensor data is displayed on an OLED and served via a mobile-accessible Wi-Fi dashboard (hosted by ESP32) with real-time charts and CSV export functionality.

---

## 🔧 Features

- Real-time monitoring of temperature, humidity, pressure, wind, O₂ and CO₂
- ESP32 acts as a Wi-Fi hotspot, serving an interactive web dashboard
- Highcharts.js for real-time plotting (embedded via LittleFS)
- CSV exportable historical data
- OLED display for on-device feedback
- Battery-powered for remote field deployment

---

## 🧰 Hardware

- Arduino Mega 2560  
- ESP32 Dev Module  
- BME280 (Temp/Humidity/Pressure)  
- O₂ & CO₂ gas sensors  
- Wind speed sensor (≥9V required)  
- DS3231 RTC module  
- OLED display  
- 12V battery + buck converter (regulated to 10V)  
- Power switch, voltage display, breadboard, jumper wires  

---

## 🔌 Arduino ↔ ESP32 Communication

| Arduino | ESP32 |
|---------|-------|
| TX      | RX (GPIO16) |
| RX      | TX (GPIO17) |
| GND     | GND |
| 5V      | 5V  |

**Note:** Use Vin (7V–12V) for power input; avoid barrel jack due to regulator heating.

---

## 🌐 Web Dashboard

- Hosted on ESP32 via LittleFS  
- Files placed in `/data` folder  
- Built with `index.html`, `highcharts.js`, etc.  
- Accessible at `192.168.4.1` when connected to ESP32-AP

---

## 📤 Uploading Files (LittleFS)

1. Download [highcharts.js](https://code.highcharts.com/highcharts.js)  
2. Place all web files in a folder named `data/`  
3. Install LittleFS uploader ([GitHub link](https://github.com/earlephilhower/arduino-littlefs-upload/releases))  
4. In Arduino IDE, press `Ctrl+Shift+P` → **Upload LittleFS to ESP32**

---

## 📱 Usage

1. Power the device  
2. Connect phone to **ESP32-AP** (`password: 12345678`)  
3. Open browser and go to `192.168.4.1`  
4. View real-time data, adjust threshold, or export CSV logs  

---

## 📎 Notes

- Wind sensor requires ≥9V → power separately from buck converter  
- Entire system works offline, suitable for field deployment  
