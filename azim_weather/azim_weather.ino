#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>

#include "DHT.h"


// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Initialize Telegram BOT
#define BOTtoken "5745278821:AAFoJGoosW96T8hM-Ya1v2aLAAcPKKoCyow"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "562232392"

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
//int botRequestDelay = 1000;
//unsigned long lastTimeBotRan;

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
String rainPath = "/rain";
String ldrPath = "/ldr";
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

float temperature;
float humidity;
//float pressure;

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 10000;

// Initialize BME280
//void initBME(){
//  if (!bme.begin(0x76)) {
//    Serial.println("Could not find a valid BME280 sensor, check wiring!");
//    while (1);
//  }
//}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  client.setInsecure(); 
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

  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  #endif

  
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

  // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

//    // Rain Sensor
    int rainDigitalVal = digitalRead(rainDigital);
//
//     // LDR Sensor
    int sensorValue = analogRead(A0);

//    if((millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
//      String message = "Humidity: " + String(h);
//      bot.sendMessage(CHAT_ID, message, "");
//    }

  if (sensorValue <= 300) {
    String message = "Humidity: " + String(h);

    bot.sendMessage(CHAT_ID, message, "");
  }
    
    // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){

    String message = "Humidity: " + String(h) + "\n" + "https://azim-weather.web.app/";

    bot.sendMessage(CHAT_ID, message, "");
    
    sendDataPrevMillis = millis();

//    // Reading temperature or humidity takes about 250 milliseconds!
//    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
//    float h = dht.readHumidity();
//    // Read temperature as Celsius (the default)
//    float t = dht.readTemperature();
//    // Read temperature as Fahrenheit (isFahrenheit = true)
//    float f = dht.readTemperature(true);
//
////    // Rain Sensor
//    int rainDigitalVal = digitalRead(rainDigital);
////
////     // LDR Sensor
//    int sensorValue = analogRead(A0);

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

    json.set(tempPath.c_str(), String(t));
    json.set(humPath.c_str(), String(h));
    json.set(rainPath.c_str(), String(rainDigitalVal));
    json.set(ldrPath.c_str(), String(sensorValue));
//    json.set(presPath.c_str(), String(bme.readPressure()/100.0F));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

//    String weatherCondition = "";
//    String rainCondition = "";
//
//    if (rainDigitalVal == 1) {
//      rainCondition = "Not Rain";
//    } else {
//      rainCondition = "Raining";
//      
//    }
//
//    if(sensorValue <= 300) {
//    weatherCondition = "Cloudy";
//    String message = "Weather Condition: " + weatherCondition + "/n" + "Rain Condition: " + rainCondition + "/n" + "Humidity: " + String(h) + "%%\n" + "Temperature: " +  String(t) + "\370\n" + "https://azim-weather.web.app/";
//    bot.sendMessage(CHAT_ID, message, "");
//    
//    } else {
//      weatherCondition = "Sunny";
//    }
//
//    String message = "Weather Condition: " + weatherCondition + "/n" + "Rain Condition: " + rainCondition + "/n" + "Humidity: " + String(h) + "%%\n" + "Temperature: " +  String(t) + "\370\n" + "https://azim-weather.web.app/";
//
//    if (millis() > lastTimeBotRan + botRequestDelay)  {
//
//    if(sensorValue <= 300) {
//    bot.sendMessage(CHAT_ID, message, "");
//    
//    }
//
//    lastTimeBotRan = millis();
//  } 

    // setup bot message

//    bot.sendMessage(CHAT_ID, "hello", "");
        
  }

//  int sensorValue = analogRead(A0);
//
//  Serial.print("Ldr: ");
//  Serial.println(sensorValue);
//
//  if(sensorValue <= 300) {
//    bot.sendMessage(CHAT_ID, "Cloudy", "");
//    
//    } else {
//      bot.sendMessage(CHAT_ID, "Sunny", "");
//    }

  

//  bot.sendMessage(CHAT_ID, "Hello", "");

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
