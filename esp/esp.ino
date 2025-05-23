#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

const char* ssid = "vanek";
const char* password = "12345678";

#define ONE_WIRE_BUS 18
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorIndoor, sensorOutdoor;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WebServer server(80);

float indoorSum = 0;
float outdoorSum = 0;
int tempCount = 0;
float currentIndoor = 0;
float currentOutdoor = 0;

void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void setup() {
  Serial.begin(9600);

  sensors.begin();
  int count = sensors.getDeviceCount();
  Serial.print("Found sensors: ");
  Serial.println(count);

  if (count < 2) {
    Serial.println("Ошибка: ожидалось 2 датчика!");
    while (true);
  }

  sensors.getAddress(sensorIndoor, 0);
  sensors.getAddress(sensorOutdoor, 1);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 not found");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  server.on("/api/temp", []() {
    addCORS();
    StaticJsonDocument<256> doc;
    doc["indoor"] = currentIndoor;
    doc["outdoor"] = currentOutdoor;
    doc["indoor_avg"] = (tempCount == 0) ? 0 : (indoorSum / tempCount);
    doc["outdoor_avg"] = (tempCount == 0) ? 0 : (outdoorSum / tempCount);
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  server.on("/api/reset", []() {
    addCORS();
    indoorSum = 0;
    outdoorSum = 0;
    tempCount = 0;
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      addCORS();
      server.send(204);
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  sensors.requestTemperatures();

  currentIndoor = sensors.getTempC(sensorIndoor);
  currentOutdoor = sensors.getTempC(sensorOutdoor);

  indoorSum += currentIndoor;
  outdoorSum += currentOutdoor;
  tempCount++;

  display.clearDisplay();
  display.setTextSize(1);

  display.setCursor(0, 20);
  display.print("Indoor: ");
  display.print(currentIndoor, 1);
  display.write(247); // знак °
  display.print("C");

  display.setCursor(0, 40);
  display.print("Outdoor: ");
  display.print(currentOutdoor, 1);
  display.write(247);
  display.print("C");

  display.display();

  server.handleClient();
  delay(1000);
}