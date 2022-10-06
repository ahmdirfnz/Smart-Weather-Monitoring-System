#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "DHT.h"


// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "TP-Link_2.4GHz@unifi"
#define WIFI_PASSWORD "WantCoffee?"

#define DHTPIN 4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

#define rainDigital D1  // Rain sensor

// Insert Firebase project API Key
#define API_KEY "AIzaSyC0vH9QPnElKgoIl0g02bz2rP25ZvnPZ1Q"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "irfanz6985@gmail.com"
#define USER_PASSWORD "hmm123"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://azim-weather-default-rtdb.firebaseio.com/"

DHT dht(DHTPIN, DHTTYPE);

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
//String presPath = "/pressure";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;

// BME280 sensor
//Adafruit_BME280 bme; // I2C
float temperature;
float humidity;
//float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000;

// Initialize BME280
//void initBME(){
//  if (!bme.begin(0x76)) {
//    Serial.println("Could not find a valid BME280 sensor, check wiring!");
//    while (1);
//  }
//}

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

void setup(){
  
  Serial.begin(115200);

  // Initialize BME280 sensor
//  initBME();
  initWiFi();
  timeClient.begin();

  pinMode(rainDigital,INPUT);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";
}

int timeSinceLastRead = 0;

void loop(){

  // Report every 2 seconds.
//  if(timeSinceLastRead > 2000) {
    

    // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Rain Sensor
    int rainDigitalVal = digitalRead(rainDigital);

     // LDR Sensor
    int sensorValue = analogRead(A0);

//    float voltage = sensorValue * (5.0 / 1023.0);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    Serial.print("Rain Sensor: ");
    Serial.println(rainDigitalVal);
    
    Serial.print("LDR Sensor: ");
    Serial.println(sensorValue);

    parentPath= databasePath + "/" + String(timestamp);

//    Serial.print("Humidity: ");
//    Serial.print(h);
//    Serial.print(" %\t");
//    Serial.print("Temperature: ");
//    Serial.print(t);
//    Serial.print(" *C ");
//    Serial.print(f);
//    Serial.println(" *F\t");

    json.set(tempPath.c_str(), String(t));
    json.set(humPath.c_str(), String(h));
//    json.set(presPath.c_str(), String(bme.readPressure()/100.0F));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }

//    // Compute heat index in Fahrenheit (the default)
//    float hif = dht.computeHeatIndex(f, h);
//    // Compute heat index in Celsius (isFahreheit = false)
//    float hic = dht.computeHeatIndex(t, h, false);

    
//    Serial.print("Heat index: ");
//    Serial.print(hic);
//    Serial.print(" *C ");
//    Serial.print(hif);
//    Serial.println(" *F");

//    timeSinceLastRead = 0;
//  }
//  delay(100);
//  timeSinceLastRead += 100;

  
}
