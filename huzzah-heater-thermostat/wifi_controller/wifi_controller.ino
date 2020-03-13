#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h" // Temperature sensor library
#include "LocalConstants.h"

extern "C" {
  // Import functions for rtc memory read/write
  #include "user_interface.h"
}

// Default/Backup temperature value
const int DEFAULT_TEMPERATURE = 18;

// Temp. sensor
#define DHTPIN 4
#define DHTTYPE DHT22

// Rotary Encoder
#define ROT1 12
#define ROT2 14

#define ERR_GET_TEMP 1
#define ERR_READING_DHT 2
#define ERR_WIFI_FAILED 3
#define ERR_WIFI_BAD_CREDS 4

/**
 * ToDo
 * - Write multiple metrics at once: https://docs.influxdata.com/influxdb/v1.7/guides/writing_data/#writing-multiple-points or rather seperate http calls?
 * x Bug: When going into sleep mode it steps half a degree down (Go half a degree up, so it's correct at the end
 * - Modularise -> SOC
 * - Report temperature
 * - Persist and report number of restarts (This can help as an indicator how long/stable the device runs -> Report)
 * - What to do in case of error? ERROR could be logged to ES, exceptions happening because of no Wifi connection (FATAL)
 * - Report humidity
 * - Offline mode: Device should still be able to work if the gateway is not reachable
 * 
 * x Replace Go button with a switch
 * x Only set set temp if value has changed via RTC memory
 * x Fix: Reset pin shouldn't be connected manually -> How to enable reset properly?
 * 
 * Other features:
 * - Replace switch button with toggle
 * 
 * */

// Init button: After startup the up/down buttons can be used to setup the HT
// When finished the init button sets the controller into "read temperature from remote and set it" mode
#define INIT_LED 0

#define RTC_INIT_CODE 12345678
typedef struct {
  uint32_t magic;
  uint8_t  lastKnownTemp;
  uint8_t  restartCounter;
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

void connectWifi() {

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
      Serial.println(F("Wifi: Bad credentials"));
      handleError(ERR_WIFI_BAD_CREDS, "Bad Wifi credentials");
      delay(10000);
    }

    delay(500);
    Serial.print("...");
    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      handleError(ERR_WIFI_FAILED, "Wifi failed to connect");
      return;
    }
  }
  Serial.println("");
  Serial.print(F("WiFi connected, IP="));
  Serial.println(WiFi.localIP());
}

/**
 * Requires wifi connection
 **/
void reportTemperature(float tmp) {

  HTTPClient http;

  // Send request
  String payload="tmp,id=1,name=living_room value=";
  payload += String(tmp, 2);
  // TODO Put ip+port and DB into localconstants.h
  http.begin("http://51.15.123.89:8086/write?db=mydb");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int returnCode = http.POST(payload);

  // Print the response
  Serial.print(F("Return code: "));
  Serial.print(returnCode);
  Serial.println(F(" Http response:"));
  Serial.println(http.getString());

  // Disconnect
  http.end();
}

void ping() {

  // Can a globally shared HttpClient be reused?
  HTTPClient http;

  // Send request
  String payload="restart_counter,id=1,name=living_room value=";
  payload += String(rtcStore.restartCounter);
  // TODO Put ip+port and DB into localconstants.h
  http.begin("http://51.15.123.89:8086/write?db=mydb");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int returnCode = http.POST(payload);

  // Print the response
  Serial.print(F("Return code: "));
  Serial.print(returnCode);
  Serial.println(F(" Http response:"));
  Serial.println(http.getString());

  // Disconnect
  http.end();
}

int retrieveTemperature() {
  
  HTTPClient http;

  char url[50];
  sprintf(url, "http://%s:5000/tmp", RASPBERRYPI_ADDRESS);
  Serial.print(F("Retrieving temperature from "));
  Serial.println(url);
  
  http.begin(url);
  
  int returnCode = http.GET();
  if (returnCode != 200) {
    handleError(ERR_GET_TEMP, "Couldnt read temperature. Return code: " + String(returnCode));
    return -1;
  }

  return http.getString().toInt();
}

/**
 * Reads sensore and calculates the heat index
 */
float getHeatIndex() {
  dht.begin();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    handleError(ERR_READING_DHT, "Couldnt read from temperature sensor");
    return 0;
  }

  return dht.computeHeatIndex(t, h, false);
}

// TODO Change to int temp, the precision isn't necessary
void set_new_temp(float temp) {
  // 2xImpulses == half a degree, so 1 degree are 4 impulses. Minus 5 (20 impulses) degree for the minimum temp of the HT
  float new_temp = (temp * 4) - 20;

  // TODO + and - are mixed up
  pulse("+", 100);
  delay(10);
  pulse("-", 2); // Changing direction
  pulse("-", new_temp);
}

// TODO Use better approach than a char for up vs down
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
      delay(20);
      digitalWrite(ROT2, pinStatus);
      delay(20);
    } else if (dir == "+") {
      digitalWrite(ROT2, pinStatus);
      delay(20);
      digitalWrite(ROT1, pinStatus);
      delay(20);
    }
  }
}

void buttonUp() {
  Serial.println(F("pulse +1"));
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

/**
 * Retrieves the target temperature from the gateway.
 * If the target temperature has changed since last time send the temperature to the HT
 */
void readAndSetTemp() {
  
  int newTemperature = retrieveTemperature();

  if (newTemperature == -1) {// -1 would indicate an error
    Serial.println(F("Skipping setting of temperature!"));
    return;
  }

  if (rtcStore.lastKnownTemp == newTemperature) {
    Serial.print(F("Target temperature ("));
    Serial.print(newTemperature);
    Serial.println(F(") hasn't changed. Skipping."));
    
    // When going to deep sleep power on the rotary encoder pin changes, resulting in an unwanted pulse
    // This steers in the other direction to prevent the temperature from being changed
    pulse("-", 1);
    
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
}

void setup() {
  // Setup Serial
  Serial.begin(9600);
  Serial.setTimeout(2000);
  while (!Serial) { }

  Serial.println(F("Device Started"));
  Serial.println(F("-------------------------------------"));
  Serial.println(F("Running heater thermostat firmware"));
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

  // Init rtc store
  system_rtc_mem_read(100, &rtcStore, sizeof(rtcStore));
  if (rtcStore.magic != RTC_INIT_CODE) {
    Serial.println("Initialising RTC memory");
    rtcStore.magic = RTC_INIT_CODE;
    rtcStore.lastKnownTemp = 0;
    rtcStore.restartCounter = 0;

    if (system_rtc_mem_write(100, &rtcStore, sizeof(rtcStore))) {
      Serial.println("rtc mem write is ok during init");
    } else {
      Serial.println("rtc mem write is fail during init");
    }
  } else {
    ++rtcStore.restartCounter;
  }
}

void loop() {

  int DEFAULT_MODE = digitalRead(BUTTON_GO);
  if (DEFAULT_MODE == HIGH) { // Standard (ON)
    
    Serial.println(F("Entering default mode"));
    digitalWrite(INIT_LED, HIGH);

    connectWifi();

    // Report heat index to InfluxDB
    float heat = getHeatIndex();
    Serial.print(F("Heat index: "));
    Serial.println(heat);

    reportTemperature(heat);
    
    readAndSetTemp();

    ping();

    Serial.println(F("Going into deep sleep for 10 seconds"));
    Serial.println(F("ZZZZzzzzzzz...."));
    Serial.println("");
    Serial.println("");

    // TODO Move magic number
    ESP.deepSleep(10e6); // 10e6 is 20 microseconds
    
  } else { // OFF -> Manual mode to setup HT
    
    Serial.println(F("Init mode (HT can no be controlled manually"));
    digitalWrite(INIT_LED, LOW); // Indicate through init LED

    processButton(BUTTON, &buttonState, &lastButtonState, &lastDebounceTime, buttonUp);
    processButton(BUTTON2, &buttonState2, &lastButtonState2, &lastDebounceTime2, buttonDown);
  }

  delay(50);
}
