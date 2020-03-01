#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <ArduinoJson.h>

#define DHTPIN 4
#define DHTTYPE DHT22

// WiFi credentials.
const char* WIFI_SSID = "--";
const char* WIFI_PASS = "--";

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure wifiClient;

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // WiFi fix: https://github.com/esp8266/Arduino/issues/2186
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      delay(10000);
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("Connected!");
  Serial.println("This device is now ready for use!");
}

void reportTemperature(float tmp) {

  HTTPClient http;

/*
  http.begin("http://51.15.123.89:8086/write?db=mydb");
  http.GET();
*/
  // Send request
  String payload="tmp,id=1,name=living_room value=";
  payload += String(tmp, 2);
  http.begin("http://51.15.123.89:8086/write?db=mydb");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(payload);

  // Print the response
  Serial.println(F("Http response:"));
  Serial.println(http.getString());

  // Disconnect
  http.end();
  
}

float getHeatIndex() {
  dht.begin();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return 0;
  }

  return dht.computeHeatIndex(t, h, false);
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while (!Serial) { }

  Serial.println(F("Device Started"));
  Serial.println(F("-------------------------------------"));
  Serial.println(F("Running Deep Sleep Firmware!"));
  Serial.println(F("-------------------------------------"));

  connect();

  float heat = getHeatIndex();
  Serial.print(F("Heat index: "));
  Serial.println(heat);

  reportTemperature(heat);

/*
  // Prepare JSON document
  DynamicJsonDocument doc(2048);
  doc["hello"] = "world";

  // Serialize JSON document
  String json;
  serializeJson(doc, json);
  Serial.print(F("Json:"));
  Serial.println(json);  
*/

  Serial.println("Going into deep sleep for 10 seconds");
  ESP.deepSleep(10e6); // 10e6 is 20 microseconds
}

void loop() {
}
