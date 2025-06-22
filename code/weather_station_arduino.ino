#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "DFRobot_OxygenSensor.h"
#include <Adafruit_SCD30.h>
#include "SD.h"

#define DS3231_ADDRESS 0x68
#define OLED_ADDRESS 0x3C
#define BME280_ADDRESS 0x76
#define Oxygen_ADDRESS 0x73
#define COLLECT_NUMBER  10 
#define RST_PIN -1
SSD1306AsciiWire oled;
RTC_DS3231 rtc;
Adafruit_SCD30  scd30;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const int OutPin  = A0;   // wind sensor analog pin hooked up to Wind P sensor "OUT" pin
const int TempPin = A1;   // temp sensor analog pin hooked up to Wind P sensor "TMP" pin
const int buttonPin = 19;
unsigned long lastButtonPressTime = 0;
unsigned long lastLogTime = 0;
bool displayActive = true;
Adafruit_BME280 bme;
DFRobot_OxygenSensor oxygen;
unsigned long lastTransmitTime = 0;
const unsigned long transmitInterval = 5000;

File dataFile;

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  Wire.begin();
  Wire.setClock(400000L);
  pinMode(buttonPin, INPUT_PULLUP);

  // Try to initialize!
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");

  // Initialize DS3231
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!bme.begin(BME280_ADDRESS)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (!oxygen.begin(Oxygen_ADDRESS)) {
    Serial.println("Could not find O2 sensor, check wiring!");
    while (1);
  }

  Serial.print("Initializing SD card...");
  if (!SD.begin(53)) {
    Serial.println("Initialization failed!");
    while (1);
  }
  Serial.println("Initialization done.");
  
  dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Time,Temperature(°C),Pressure(hPa),Humidity(%),Wind Speed(MPH),Oxygen Concentration(%vol),CO2(ppm)");
    dataFile.close();
  } else {
    Serial.println("Error opening data.txt");
  }            

  #if RST_PIN >= 0
    oled.begin(&SH1106_128x64, OLED_ADDRESS, RST_PIN);
  #else
    oled.begin(&SH1106_128x64, OLED_ADDRESS);
  #endif
  oled.setFont(Adafruit5x7);
  oled.set1X();
  oled.clear();
  oled.println(F("Setup done."));
  delay(2000);
  oled.clear();
}

void logData(DateTime now, float temperature, float pressure, float humidity, float windMPH, float oxygenData, float CO2Data) {
  dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print(" ");
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print(", ");
    dataFile.print(temperature);
    dataFile.print(", ");
    dataFile.print(pressure);
    dataFile.print(", ");
    dataFile.print(humidity);
    dataFile.print(", ");
    dataFile.print(windMPH);
    dataFile.print(", ");
    dataFile.print(oxygenData);
    dataFile.print(", ");
    dataFile.println(CO2Data);      

    dataFile.flush();
    dataFile.close();
    Serial.println("Logged Data");
  } else {
    Serial.println("Error opening data file");
  }
}

void SendData(DateTime now, float temperature, float pressure, float humidity, float windMPH, float oxygenData, float CO2Data) {
  float dataToSend[12] = {temperature, pressure, humidity, windMPH, oxygenData, CO2Data, now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  byte* dataAsBytes = (byte*)dataToSend;
  Serial1.write(dataAsBytes, sizeof(dataToSend));

}

void loop() {
  DateTime now = rtc.now();
  scd30.dataReady();
  scd30.read();
  int buttonState = digitalRead(buttonPin);
  unsigned long currentTime = millis();
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;
  float humidity = bme.readHumidity();  
  int windADunits = analogRead(OutPin);
  float windMPH = pow((((float)windADunits - 264.0) / 85.6814), 3.36814);
  float oxygenData = oxygen.getOxygenData(COLLECT_NUMBER);
  float CO2Data = scd30.CO2;
  // if (millis() - lastTransmitTime >= transmitInterval) {
  lastTransmitTime = millis();
  SendData(now, temperature, pressure, humidity, windMPH, oxygenData, CO2Data);
  // if (scd30.dataReady()){
  //   if (!scd30.read()){ return; }
  // }

  // if (buttonState == LOW) {
    displayActive = true;
    lastButtonPressTime = currentTime;
    oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
    oled.clear();
    oled.setCursor(0, 0);
    oled.print(now.year(), DEC);
    oled.print('/');
    oled.print(now.month(), DEC);
    oled.print('/');
    oled.print(now.day(), DEC);
    oled.print(' ');
    oled.print(now.hour(), DEC);
    oled.print(':');
    oled.print(now.minute(), DEC);
    oled.print(':');
    oled.print(now.second(), DEC);

    oled.setCursor(0, 1);
    oled.print("Temp: ");
    oled.print(temperature);
    oled.print(" C");

    oled.setCursor(0, 2);
    oled.print("Pres: ");
    oled.print(pressure);
    oled.print(" hPa");

    oled.setCursor(0, 3);
    oled.print("Humi: ");
    oled.print(humidity);
    oled.print(" %");

    oled.setCursor(0, 4);
    oled.print("Wind: ");
    oled.print(windMPH);
    oled.print(" MPH");

    oled.setCursor(0, 5);
    oled.print("O2: ");
    oled.print(oxygenData);
    oled.print(" %vol"); 

    oled.setCursor(0, 6);
    oled.print("CO2: ");
    oled.print(CO2Data, 3);
    oled.print(" ppm");
    delay(1000);
    // }
    // else{
    // oled.clear();
    // }   
  // Log data every five minutes
  if (currentTime - lastLogTime >= 300000) { // 300000 milliseconds = 5 minutes
    lastLogTime = currentTime; // Update the last log time
    logData(now, temperature, pressure, humidity, windMPH, oxygenData, CO2Data);
    Serial.println("Data logged");
  }

  delay(1000);
}