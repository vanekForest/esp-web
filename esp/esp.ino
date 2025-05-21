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
DeviceAddress sensorAddress;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WebServer server(80);

float tempSum = 0;
int tempCount = 0;
float currentTemp = 0;

void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void setup() {
  Serial.begin(9600);

  sensors.begin();
  if (sensors.getDeviceCount() > 0) {
    sensors.getAddress(sensorAddress, 0);
  } else {
    Serial.println("DS18B20 not found!");
  }

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
    StaticJsonDocument<200> doc;
    doc["current"] = currentTemp;
    doc["average"] = (tempCount == 0) ? 0 : (tempSum / tempCount);
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  server.on("/api/reset", []() {
    addCORS();
    tempSum = 0;
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
  currentTemp = sensors.getTempC(sensorAddress);
  tempSum += currentTemp;
  tempCount++;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  if (currentTemp == -127) {
    display.print("Sensor error");
  } else {
    display.setCursor(40, 30);
    display.print(currentTemp, 1);
    display.write(247);
    display.print("C");
  }
  display.display();

  server.handleClient();
  delay(1000);
}