#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <vector>

// WiFi Access Point credentials
const char* ssid = "ESP32-AP";
const char* password = "12345678";

// Initialize Async Web Server on port 80
AsyncWebServer server(80);

// Buffer for received sensor data
float receivedData[12];  // Stores 12 float values: 6 sensor readings + 6 timestamp components

// Timing variables for data recording
unsigned long lastRecordTime = 0;
const unsigned long recordInterval = 1 * 60 * 1000;  // Record data every 1 minute (in milliseconds)

// Structure to store sensor data records
struct DataRecord {
  unsigned long timestamp;  // Record timestamp in milliseconds
  float data[6];           // Array for 6 sensor readings: temperature, pressure, humidity, windMPH, oxygenData, CO2Data
  int year, month, day, hour, minute, second;  // Timestamp components
};

// Vector to store data history
std::vector<DataRecord> dataHistory;

// Initialize hardware and web server
void setup() {
  Serial.begin(115200);  // Start primary serial for debugging
  Serial1.begin(115200, SERIAL_8N1, 16, 17);  // Start secondary serial (RX1=16, TX1=17) for sensor data

  // Initialize LittleFS file system
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;  // Exit if file system initialization fails
  }

  // Set up WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);

  // Define web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");  // Serve index.html
  });

  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/highcharts.js", "application/javascript");  // Serve Highcharts library
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Generate JSON response with current sensor data
    String json = "[";
    for (int i = 0; i < 6; i++) {
      json += String(receivedData[i], 2);
      json += ",";
    }
    json += "\"" + String(receivedData[6], 0) + "/" + String(receivedData[7], 0) + "/" + 
            String(receivedData[8], 0) + " " + String(receivedData[9], 0) + ":" + 
            String(receivedData[10], 0) + ":" + String(receivedData[11], 0) + "\"";
    json += "]";
    request->send(200, "application/json", json);
  });

  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", getHistoryContent());  // Serve historical data as HTML
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Generate CSV file for download
    String csv = "Timestamp,Temperature (C),Pressure (hPa),Humidity (%),Wind Speed (MPH),Oxygen Level (%vol),CO2 Level (ppm)\n";
    for (const auto& record : dataHistory) {
      char timeString[20];
      snprintf(timeString, sizeof(timeString), "%04d/%02d/%02d %02d:%02d:%02d",
               record.year, record.month, record.day,
               record.hour, record.minute, record.second);
      csv += String(timeString) + ",";
      for (int i = 0; i < 6; i++) {
        csv += String(record.data[i], 2);
        if (i < 5) csv += ",";
      }
      csv += "\n";
    }

    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csv);
    response->addHeader("Content-Disposition", "attachment; filename=\"WeatherStationData.csv\"");
    request->send(response);  // Send CSV as downloadable file
  });

  server.begin();  // Start web server
  Serial.println("HTTP server started");
}

// Main loop for receiving and recording data
void loop() {
  // Read sensor data from Serial1 if available
  if (Serial1.available() >= sizeof(receivedData)) {
    Serial1.readBytes((byte*)receivedData, sizeof(receivedData));
  }

  // Record data at regular intervals
  unsigned long currentTime = millis();
  if (currentTime - lastRecordTime >= recordInterval) {
    recordData();
    lastRecordTime = currentTime;
  }
}

// Record sensor data to history
void recordData() {
  DataRecord record;
  record.timestamp = millis();  // Store current timestamp
  for (int i = 0; i < 6; i++) {
    record.data[i] = receivedData[i];  // Copy sensor data
  }
  // Store timestamp components
  record.year = static_cast<int>(receivedData[6]);
  record.month = static_cast<int>(receivedData[7]);
  record.day = static_cast<int>(receivedData[8]);
  record.hour = static_cast<int>(receivedData[9]);
  record.minute = static_cast<int>(receivedData[10]);
  record.second = static_cast<int>(receivedData[11]);
  
  dataHistory.push_back(record);  // Add record to history

  // Limit history size to 96 entries
  if (dataHistory.size() > 96) {
    dataHistory.erase(dataHistory.begin());
  }
}

// Generate HTML content for data history page
String getHistoryContent() {
  String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Data History</title><style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }";
  html += "table { border-collapse: collapse; width: 100%; background-color: white; }";
  html += "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }";
  html += "th { background-color: #007bff; color: white; }";
  html += "tr:nth-child(even) { background-color: #f2f2f2; }";
  html += "td.time-column { width: 200px; }";
  html += ".button { margin: 20px 0; padding: 10px 20px; background-color: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".button:hover { background-color: #0056b3; }";
  html += "</style></head><body>";
  html += "<h1>Data History</h1>";
  html += "<button class='button' onclick=\"window.location.href='/download'\">Download CSV</button>";
  html += "<table><tr><th class='time-column'>Time</th><th>Temperature (C)</th><th>Pressure (hPa)</th><th>Humidity (%)</th><th>Wind Speed (MPH)</th><th>Oxygen Level (%vol)</th><th>CO2 Level (ppm)</th></tr>";
  
  // Populate table with historical data
  for (const auto& record : dataHistory) {
    char timeString[20];
    snprintf(timeString, sizeof(timeString), "%04d/%02d/%02d %02d:%02d:%02d",
             record.year, record.month, record.day,
             record.hour, record.minute, record.second);
    html += "<tr><td class='time-column'>" + String(timeString) + "</td>";
    for (int i = 0; i < 6; i++) {
      html += "<td>" + String(record.data[i], 2) + "</td>";
    }
    html += "</tr>";
  }
  
  html += "</table></body></html>";
  return html;
}