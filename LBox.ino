#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <Arduino.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#include <Wire.h>  
#include "SSD1306.h" 

#include "icons.h"

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecureBearSSL.h>


// Initialize the OLED display using Wire library
SSD1306  display(0x3c, D2, D1);

int counter = 1;
String city = "City";
String urlcity = "http://";
int refreshdelay = 5000;
String urlrefreshdelay = "http://";
String message = "Message";
String urlmessage = "http://";
String API_KEY = "key";
String urlAPI_KEY = "http://";
String urlup = "";
String icontest = "null";
String urlicontest = "http://";

String units = "metric";


long weatherDataTimer = 0;

std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
    // Initialising the UI will init the display too.
  display.init();

  //display.flipScreenVertically();
  
  // clear the display
  display.clear();

  drawTextCenterAlignment24("Starting ...");
  delay(2000);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    drawTextCenterAlignment10("Waiting for WiFi connection");
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if(!wifiManager.autoConnect("LBox", "")) {
      drawTextCenterAlignment10("Failed to connect and hit timeout");
      Serial.println("failed to connect and hit timeout");
      //reset and try again, or maybe put it to deep sleep
      wifiManager.resetSettings();
      ESP.reset();
      delay(1000);
    } 
    counter++;
  }  
  
  
  //if you get here you have connected to the WiFi
  Serial.println("Connected...yeey :)");
  drawTextCenterAlignment24("Connected");
  String ip = WiFi.localIP().toString().c_str();
  Serial.print("IP Address: \n");
  Serial.println(ip);


  isup();
  getparams();
  drawTextCenterAlignment16(message);
  
  initWeather();

}


void loop() {
    if (millis() - weatherDataTimer > refreshdelay) {
      initWeather();
      weatherDataTimer = millis();
  }

}

void getparams() {
  HTTPClient http;  //Declare an object of class HTTPClient
  client->setInsecure();
  http.begin(*client,urlcity);  //Specify request destination
  int httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    city = http.getString();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment16(city);
  }
  http.end();//Close connection


  //client->setInsecure();
  http.begin(*client,urlmessage);  //Specify request destination
  httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    message = http.getString();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment16(message);
  }
  http.end();//Close connection


  //client->setInsecure();
  http.begin(*client,urlrefreshdelay);  //Specify request destination
  httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    refreshdelay = http.getString().toInt();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment16(message);
    //delay(refreshdelay);
  }
  http.end();//Close connection


  //client->setInsecure();
  http.begin(*client,urlAPI_KEY);  //Specify request destination
  httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    API_KEY = http.getString();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment10(API_KEY);
    //delay(pagedelay);
  }
  http.end();//Close connection
  
}

void isup() {
  HTTPClient http;  //Declare an object of class HTTPClient
  client->setInsecure();
  http.begin(*client,urlup);  //Specify request destination
  int httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    String result = http.getString();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment10(result);
  }
  http.end();//Close connection

  http.begin(*client,urlicontest);  //Specify request destination
  httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    icontest = http.getString();   //Get the request response payload
    //parse data
    //drawTextCenterAlignment10(icontest);
  }
  http.end();//Close connection
  
}


void initWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    drawTextCenterAlignment24("Disconnected");
    ESP.reset();
  } else {
    Serial.println("getWeatherData\n");
    isup();
    getparams();
    getWeatherData();
  }
}

void getWeatherData() {
  String url = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&units=" + units + "&APPID=" + API_KEY;

  HTTPClient http;  //Declare an object of class HTTPClient
  http.begin(url);  //Specify request destination
  int httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check t he returning code
    String payload = http.getString();   //Get the request response payload
    //parse data
    parseWeatherData(payload);
  }
  http.end();//Close connection
}




void drawTextCenterAlignment10(String text) {
  // The coordinates define the center of the text
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, text);
  display.display();
  delay(3000);
}

void parseWeatherData(String payload) {
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(11) + 500;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(payload);

  float coord_lon = root["coord"]["lon"]; // 25.61
  float coord_lat = root["coord"]["lat"]; // 45.65

  JsonObject& weather_0 = root["weather"][0];
  int weather_0_id = weather_0["id"]; // 803
  const char* weather_0_main = weather_0["main"]; // "Clouds"
  const char* weather_0_description = weather_0["description"]; // "broken clouds"
  const char* weather_0_icon = weather_0["icon"]; // "04d"

  const char* base = root["base"]; // "stations"

  JsonObject& main = root["main"];
  float main_temp = main["temp"]; // -6.04
  float main_pressure = main["pressure"]; // 1036.21
  int main_humidity = main["humidity"]; // 65
  float main_temp_min = main["temp_min"]; // -6.04
  float main_temp_max = main["temp_max"]; // -6.04
  float main_sea_level = main["sea_level"]; // 1036.21
  float main_grnd_level = main["grnd_level"]; // 922.42

  float wind_speed = root["wind"]["speed"]; // 1.21
  float wind_deg = root["wind"]["deg"]; // 344.501

  int clouds_all = root["clouds"]["all"]; // 68

  long dt = root["dt"]; // 1551023165

  JsonObject& sys = root["sys"];
  float sys_message = sys["message"]; // 0.0077
  const char* sys_country = sys["country"]; // COUNTRY
  long sys_sunrise = sys["sunrise"]; // 1550984672
  long sys_sunset = sys["sunset"]; // 1551023855

  long id = root["id"]; // 683844
  const char* cityName = root["name"]; // CITY
  int cod = root["cod"]; // 200

  display.clear();
  

  getCurrentTimeRequest(coord_lat, coord_lon);

  displayTemperature(main_temp);
  displayIcon(weather_0_icon);

  delay(5000);
}

// Icons
void displayIcon(String weatherIcon) {
  Serial.println(weatherIcon);
  if(icontest == "null"){
    
  } else {
    weatherIcon = icontest;
  }
  if (weatherIcon == "01d") {
    display.drawXbm(0, 0, cleard_width, cleard_height, cleard_bits);
  } else if (weatherIcon == "01n") {
    display.drawXbm(0, 0, clearn_width, clearn_height, clearn_bits);
  } else if (weatherIcon == "02d") {
    display.drawXbm(0, 0, cloudd_width, cloudd_height, cloudd_bits);
  } else if (weatherIcon == "02n") {
    display.drawXbm(0, 0, cloudn_width, cloudn_height, cloudn_bits);
  } else if (weatherIcon == "03d" || weatherIcon == "03n" || weatherIcon == "04d" || weatherIcon == "04n") {
    display.drawXbm(0, 0, cloud_width, cloud_height, cloud_bits);
  } else if (weatherIcon == "09d" || weatherIcon == "09n" || weatherIcon == "10d" || weatherIcon == "10n") {
    display.drawXbm(0, 0, rain_width, rain_height, rain_bits);
  } else if (weatherIcon == "11d" || weatherIcon == "11n") {
    display.drawXbm(0, 0, storm_width, storm_height, storm_bits);
  } else if (weatherIcon == "13d" || weatherIcon == "13n") {
    display.drawXbm(0, 0, snow_width, snow_height, snow_bits);
  } else if (weatherIcon == "50d" || weatherIcon == "50n") {
    display.drawXbm(0, 0, mist_width, mist_height, mist_bits);
  } else if (weatherIcon == "na") {
    display.drawXbm(0, 0, na_width, na_height, na_bits);
  } else {
    display.drawXbm(0, 0, na_width, na_height, na_bits);
  }
  display.display();
}


void getCurrentTimeRequest(float latitude, float longitude) {
  String url = "http://api.geonames.org/timezoneJSON?lat=" + String(latitude) + "&lng=" + String(longitude) + "&username=kode";

  HTTPClient http;  //Declare an object of class HTTPClient
  http.begin(url);  //Specify request destination
  int httpCode = http.GET();//Send the request
  if (httpCode > 0) { //Check the returning code
    String payload = http.getString();   //Get the request response payload
    //parse data
    parseTimeData(payload);
  }
  http.end();   //Close connection
}

void parseTimeData(String payload) {
  const size_t capacity = JSON_OBJECT_SIZE(11) + 220;
  DynamicJsonBuffer jsonBuffer(capacity);

  JsonObject& root = jsonBuffer.parseObject(payload);

  const char* sunrise = root["sunrise"]; // "2019-03-08 06:44"
  float lng = root["lng"]; // 25.61
  const char* countryCode = root["countryCode"]; // "RO"
  int gmtOffset = root["gmtOffset"]; // 2
  int rawOffset = root["rawOffset"]; // 2
  const char* sunset = root["sunset"]; // "2019-03-08 18:13"
  const char* timezoneId = root["timezoneId"]; // "Europe/Bucharest"
  int dstOffset = root["dstOffset"]; // 3
  const char* countryName = root["countryName"]; // "Romania"
  const char* currentTime = root["time"]; // "2019-03-08 01:11"
  float lat = root["lat"]; // 45.65

  Serial.print(currentTime);

  displayCurrentTime(currentTime);
}

// TIME
void displayCurrentTime(String currentTime) {
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  String timeOnly = currentTime.substring(10);
  display.drawString(128, 40, timeOnly);
  display.display();
}

// TEMPERATURE
void displayTemperature(float main_temp) {
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  String temperatureValue = String((int)main_temp) + "°C";
  display.drawString(128, 0, temperatureValue);
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 20, city);
  display.display();
}



void drawTextCenterAlignment16(String text) {
  // The coordinates define the center of the text
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, text);
  display.display();
  delay(3000);
}

void drawTextCenterAlignment24(String text) {
  // The coordinates define the center of the text
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, text);
  display.display();
  delay(2000);
}
