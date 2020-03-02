#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <ArduinoJson.h>

// Temp. sensor
#define DHTPIN 4
#define DHTTYPE DHT22

// Rotary Encoder
#define ROT1 12
#define ROT2 14

/**
 * ToDo
 * - Replace String with char[] 
 *  
 * */

// 2 x Buttons: PIN, State, debounce, delay
int buttonState;
int buttonState2;
int BUTTON = 15;
int BUTTON2 = 13;
int lastButtonState = LOW;
int lastButtonState2 = LOW;
unsigned long lastDebounceTime = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 30;


// WiFi credentials
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

// TODO Change to to int temp, the precision isn't necessary
void set_new_temp(float temp) {
  float new_temp = (temp * 2) - 10;
  pulse("-", 60);
  pulse("+", new_temp);
}

void pulse(char dir[], float qty_pulses) {
  bool pinStatus = digitalRead(ROT1);
  for (int i = 0; i < qty_pulses; i++) {
    if (pinStatus == LOW) {
      pinStatus = HIGH;
    } else {
      pinStatus = LOW;
    }
    if (dir == "-") {
      digitalWrite(ROT1, pinStatus);
      delay(10);
      digitalWrite(ROT2, pinStatus);
      delay(10);
    } else if (dir == "+") {
      digitalWrite(ROT2, pinStatus);
      delay(10);
      digitalWrite(ROT1, pinStatus);
      delay(10);
    }
  }
}

void buttonUp() {
  Serial.println(F("pulse +1"));
  //digitalWrite(LED, HIGH);
  //delay(100);
  //digitalWrite(LED, LOW);
  //delay(100);
  pulse("+", 2);
}

void buttonDown() {
  Serial.println(F("pulse -1"));
  //digitalWrite(LED, HIGH);
  //delay(100);
  //digitalWrite(LED, LOW);
  //delay(100);
  pulse("-", 2);
}

void processButton(int buttonId, int* currentState, int* lastState, unsigned long* lastDebounce, void (*action)()) {
   int reading = digitalRead(buttonId);

  if (reading != *lastState) {
    // reset the debouncing timer
    *lastDebounce = millis();
  }

  if ((millis() - *lastDebounce) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != *currentState) {
      *currentState = reading;

      // only toggle the LED if the new button state is HIGH
      if (*currentState == HIGH) {
        action();
      } 
    }
  }
  *lastState = reading;
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

  // Setup button pins
  pinMode(BUTTON, INPUT);
  pinMode(BUTTON2, INPUT);

  // Setup rotary encoder pins
  pinMode(ROT1, OUTPUT);
  pinMode(ROT2, OUTPUT);
  digitalWrite(ROT1, LOW);
  digitalWrite(ROT2, LOW);

/*
  connect();

  float heat = getHeatIndex();
  Serial.print(F("Heat index: "));
  Serial.println(heat);

  reportTemperature(heat);


  Serial.println("Going into deep sleep for 10 seconds");
  ESP.deepSleep(10e6); // 10e6 is 20 microseconds
  */
}

void loop() {

  processButton(BUTTON, &buttonState, &lastButtonState, &lastDebounceTime, buttonUp);
  processButton(BUTTON2, &buttonState2, &lastButtonState2, &lastDebounceTime2, buttonDown);
  //digitalWrite(LED, HIGH);
  delay(50);
  Serial.println(F("looping :B)"));
  
}
