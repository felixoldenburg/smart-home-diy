#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <ArduinoJson.h>
#include "LocalConstants.h"

extern "C" {
  // Import functions for rtc memory read/write
  #include "user_interface.h"
}

// Temp. sensor
#define DHTPIN 4
#define DHTTYPE DHT22

// Rotary Encoder
#define ROT1 12
#define ROT2 14

#define ERR_GET_TEMP 1

/**
 * ToDo
 * x Fix: Reset pin shouldn't be connected manually -> How to enable reset properly?
 * - Persist number of restarts (This can help as an indicator how long/stable the device runs)
 * - Replace String with char[] 
 * - What to do in case of error?
 * x Replace Go button with a switch
 * - Only set set temp if value has changed
 * -- Needs to persist last known value in RTC memory
 * - Report temperature
 * - Report humidity
 * - Modularise/SOC
 * 
 * Other features:
 * - Replace switch again with small button and self build electronic switch (toggle)
 * 
 * */

// Init button: After startup the up/down buttons can be used to setup the HT
// When finished the init button sets the controller into "read temperature from remote and set it" mode
#define INIT_LED 0

#define RTC_MAGIC 12345678
typedef struct {
  uint32_t magic;
  uint8_t  lastKnownTemp;
  //uint8_t  wlanConnectFailCnt;
  //float    tempAccu;
  //float    humiAccu;
} RTCSTORE __attribute__((aligned(4)));

RTCSTORE rtcStore;

int buttonStateGo;
int BUTTON_GO = 5;
int lastButtonStateGo = LOW;
unsigned long lastDebounceTimeGo = 0;

// 2 x Buttons: PIN, State, debounce, delay
int buttonState;
int buttonState2;
int BUTTON = 13;
int BUTTON2 = 15;
int lastButtonState = LOW;
int lastButtonState2 = LOW;
unsigned long lastDebounceTime = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 30;

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure wifiClient;

// F() puts string into flash not RAM
void handleError(int errorCode, String message) {
  Serial.print(F("Error("));
  Serial.print(errorCode);
  Serial.print(F("): "));
  Serial.println(message);
}

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

int getTemperature() {
  
  HTTPClient http;

  char url[50];
  sprintf(url, "http://%s:5000/tmp", RASPBERRYPI_ADDRESS);
  Serial.print(F("Getting temperature from: "));
  Serial.println(url);
  
  http.begin(url);
  int returnCode = http.GET();

  if (returnCode != 200) {
    handleError(ERR_GET_TEMP, "Couldnt read temperature");
    return -1;
  }

  return http.getString().toInt();
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
  //float new_temp = (temp * 2) - 10;
  float new_temp = (temp * 4) - 18;
  pulse("+", 100);
  //delay(100);
  //pulse("-", 2); // Change direction
  delay(10);
  pulse("-", new_temp);
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

void buttonGo() {
  Serial.println(F("Go button pressed"));
  digitalWrite(INIT_LED, HIGH);

  readAndSetTemp();
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

void readAndSetTemp() {
  connect();

  int newTemperature = getTemperature();

  if (rtcStore.lastKnownTemp == newTemperature) {
    Serial.println(F("Target temperature hasn't changed. Skipping."));
  } else {
    Serial.print(F("Setting temperature to: "));
    Serial.println(newTemperature);

    set_new_temp((float)newTemperature);
    
    rtcStore.lastKnownTemp = newTemperature;
    
    if (system_rtc_mem_write(100, &rtcStore, sizeof(rtcStore))) {
      Serial.println("rtc mem write is ok during temp persist");
    } else {
      Serial.println("rtc mem write is fail during temp persist");
    }
  }
  
/*
  float heat = getHeatIndex();
  Serial.print(F("Heat index: "));
  Serial.println(heat);

  reportTemperature(heat);
*/


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

  pinMode(INIT_LED, OUTPUT);
  digitalWrite(INIT_LED, LOW);

  // Setup button pins
  pinMode(BUTTON, INPUT);
  pinMode(BUTTON2, INPUT);

  // Setup rotary encoder pins
  pinMode(ROT1, OUTPUT);
  pinMode(ROT2, OUTPUT);
  digitalWrite(ROT1, LOW);
  digitalWrite(ROT2, LOW);

  system_rtc_mem_read(100, &rtcStore, sizeof(rtcStore));
  if (rtcStore.magic != RTC_MAGIC) {
    Serial.println("Initialising RTC memory");
    rtcStore.magic = RTC_MAGIC;
    rtcStore.lastKnownTemp = 0;

    if (system_rtc_mem_write(100, &rtcStore, sizeof(rtcStore))) {
      Serial.println("rtc mem write is ok during init");
    } else {
      Serial.println("rtc mem write is fail during init");
    }
  }
}

void loop() {
  //processButton(BUTTON_GO, &buttonStateGo, &lastButtonStateGo, &lastDebounceTimeGo, buttonGo);
  int initPhase = digitalRead(BUTTON_GO);
  if (initPhase == HIGH) {
    
    Serial.println(F("Auto mode"));
    digitalWrite(INIT_LED, HIGH);
    readAndSetTemp();

    Serial.println("Going into deep sleep for 10 seconds");
    ESP.deepSleep(10e6); // 10e6 is 20 microseconds
    
  } else { // OFF -> Manual mode to setup HT
    
    Serial.println(F("Manual mode"));
    digitalWrite(INIT_LED, LOW);
    processButton(BUTTON, &buttonState, &lastButtonState, &lastDebounceTime, buttonUp);
    processButton(BUTTON2, &buttonState2, &lastButtonState2, &lastDebounceTime2, buttonDown);
  }

  
  //digitalWrite(LED, HIGH);
  delay(50);
  //Serial.println(F("looping :B)"));
  
}
