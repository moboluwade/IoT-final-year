#include <Wire.h>          // Include the Wire library for I2C communication
#include <Adafruit_GFX.h>  // Include the Adafruit GFX library for graphics functions
#include <Adafruit_BMP085.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "SH1106Wire.h"
#include <SH1106.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert Firebase project API Key
#define API_KEY "AIzaSyCJdRRUtuED8iSiuGx2KfdBmuPYXWqe47Q"
#define USER_EMAIL "silasagbaje@gmail.com"
#define USER_PASSWORD "081******"
// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://iot-weather-station-293de-default-rtdb.europe-west1.firebasedatabase.app/"
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
String presPath = "/pressure";
String ldrPath = "/lightIntensity";
String rainPath = "/rain";
String timePath = "/timestamp";
// Parent Node (to be updated in every loop)
String parentPath;
FirebaseJson json;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
// Variable to save current epoch time
int timestamp;
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000;
// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}
const char* ssid = "Infinix HOT 10 Lite";
const char* password = "081******";
const char* thingSpeakApiKey = "FX1U5RA8T3H5X109";
const unsigned long channelID = 2262545;
WiFiClient client;
#define OLED_RESET 4
SH1106Wire display(0x3c, D2, D1);
unsigned long delayTime;
Adafruit_BMP085 bmp;
const int LDR_PIN = A0;  // Analog pin connected to the LDR
#define DHTTYPE DHT22    // DHT 22
//DHT Sensor;
uint8_t DHTPin = D6;
DHT dht(DHTPin, DHTTYPE);
float Temperature;
float Humidity;
float Temp_Fahrenheit;
String Temp_string;
String humd_string;
String press_string;
String light_string;
float bmp_temperature;
float bmp_pressure;
#define rainDigital D0
int rainState;
void setup() {
  Serial.begin(115200);
  display.init();  // init done
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  // sets the alignment to left( you can use center, right also)
  display.drawString(0, 0, "Weather Station");
  display.drawString(40, 20, "by");
  display.drawString(0, 40, "Silas & Qossim");
  display.display();
  delay(4000);
  display.clear();
  //Wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  display.drawString(0, 40, "Setting up WiFi");
  display.display();
  delay(4000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    display.clear();
    display.drawString(0, 40, "Connecting...");
    display.display();
    delay(1000);
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  display.clear();
  display.drawString(0, 40, "WiFi Connected");
  display.display();
  delay(2000);
  display.clear();
  pinMode(DHTPin, INPUT);
  // pinMode(D8, OUTPUT);
  // digitalWrite(D8, HIGH);
  pinMode(rainDigital, INPUT);
  pinMode(LDR_PIN, INPUT);
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    //while (1) {}
  }
  dht.begin();
  // Initialize ThingSpeak with your API Key
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  //Firebase setup
  timeClient.begin();
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
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

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
void loop() {
  //unsigned long currentMillis = millis(); //thingspeaks delay
  Humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  Temperature = dht.readTemperature();
  humd_string = String(Humidity);
  Temp_string = String(Temperature);
  press_string = String(bmp_pressure);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  Temp_Fahrenheit = dht.readTemperature(true);
  //Check if any reads failed and exit early (to try again).
  if (isnan(Humidity) || isnan(Temperature) || isnan(Temp_Fahrenheit)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.print(F("Humidity: "));
  Serial.print(Humidity);
  Serial.print(F("% Temperature: "));
  Serial.print(Temperature);
  Serial.print(F("°C "));
  Serial.print(Temp_Fahrenheit);
  Serial.println(F("°F "));
  display.clear();
  display.drawString(0, 0, "Temperature: ");
  display.drawString(0, 20, Temp_string + "°C");
  display.display();
  delay(3000);
  display.clear();
  display.drawString(0, 0, "Humidity: ");
  display.drawString(0, 20, humd_string + "%");
  display.display();
  delay(1000);
  display.clear();
  int lightIntensity = analogRead(LDR_PIN);  // Read the analog value from the LDR
  int ldrValue = map(lightIntensity, 0, 1023, 0, 100);
  light_string = String(ldrValue);
  Serial.println(ldrValue);
  display.drawString(0, 0, "Light Intensity");
  display.drawString(0, 20, light_string);
  display.display();
  delay(1000);
  display.clear();
  //Pressure
  bmp_temperature = bmp.readTemperature();
  bmp_pressure = bmp.readPressure();
  if (isnan(bmp_pressure) || isnan(bmp_temperature)) {
    Serial.println("Failed to read from bmp sensor!");
  }
  Serial.print("Temperature (bmp) = ");
  Serial.print(bmp_temperature);
  Serial.println("°C");
  display.clear();
  Serial.print("Pressure = ");
  Serial.print(bmp_pressure);
  Serial.println(" Pa");
  display.drawString(0, 0, "Pressure: ");
  display.drawString(0, 20, press_string + "Pa");
  display.display();
  delay(1000);
  display.clear()
    //Rain
    rainState = digitalRead(rainDigital);
  if (rainState == 0) {
    Serial.println("Rain is detected");
    display.drawString(0, 0, "It's Raining");
    display.display();  // Display the content on the OLED screen
    delay(1000);        // Delay for 1 second
    display.clear();
  } else {
    Serial.println("Rain not detected");
    display.drawString(0, 0, "No rain");
    display.display();  // Display the content on the OLED screen
    delay(1000);        // Delay for 1 second
    display.clear();
  }
  // Send data to ThingSpeak
  ThingSpeak.setField(1, Temperature);
  ThingSpeak.setField(2, Humidity);
  ThingSpeak.setField(3, ldrValue);
  ThingSpeak.setField(4, bmp_pressure);
  ThingSpeak.setField(5, rainState);
  int status = ThingSpeak.writeFields(channelID, thingSpeakApiKey);
  if (status == 200) {
    Serial.println("Data sent to ThingSpeak successfully.");
  } else {
    Serial.println("Failed to send data to ThingSpeak.");
  }
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);
    parentPath = databasePath + "/" + String(timestamp);
    json.set(tempPath.c_str(), String(dht.readTemperature()));
    json.set(humPath.c_str(), String(dht.readHumidity()));
    json.set(presPath.c_str(), String(bmp.readPressure()));
    json.set(ldrPath.c_str(), String(ldrValue));
    json.set(rainPath.c_str(), String(rainState));
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}