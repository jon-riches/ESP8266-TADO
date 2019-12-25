/*
Written by Jonathan Riches, inspired by many. Apologies for anyone's code portions I used and haven't credited!
This code isn't great, but it does work...
References: -
https://circuits4you.com 
https://arduinojson.org/v6/api/json/deserializejson/
https://shkspr.mobi/blog/2019/02/tado-api-guide-updated-for-2019/
http://blog.scphillips.com/posts/2017/01/the-tado-api-v2/
The Espressif / Arduino documentation / examples
https://github.com/esp8266/Arduino/tree/master/libraries 
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>        // https://arduinojson.org/

// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 4     // Digital pin connected to the DHT sensor - change as required
#define DHTTYPE    DHT22     // Sensor Type DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

//##################################################################################################################
/* Set these to your desired credentials. */

// WiFi
const char *ssid = "your-ssid";
const char *password = "your_password";

// Tado
const char *tlogin = "your_tado_login";
const char *tpassword = "your_tado_password";
String thome = "00000"; // your_home_number"
String tzone = "1"; // zone_id
String tdevice = "VA1234567890"; //device ID to offset

// End of credential / user setup block
//##################################################################################################################

float actTemp = 20; // Actual temperature of the room from ESP8266 Sensor
float txoffset =0; //  Existing offset value returned by Tado
float toffset = 0;  // Offset value to send to Tado
char toffsetS[] = "T0000"; // Converted value to actually submit to Tado

//Web/Server address to read/write from

const char *host = "auth.tado.com";
const char *host2 = "my.tado.com";
const int httpsPort = 443; //HTTPS= 443 and HTTP = 80

//SHA1 finger print of certificate use web browser to view and copy
// Will change if Tado update their certificates
const char fingerprint[]  PROGMEM = "A3 D1 64 E8 F4 FE 55 A9 05 32 D4 15 A3 15 32 22 3F 9A 8A 70";
const char fingerprint2[] PROGMEM = "E3 F4 F5 81 49 0A 2D 49 BD 2C EE F6 E9 CA 9C B3 CE DE 39 8D";

//=======================================================================
//                    Power on setup
//=======================================================================

void setup()
{
  dht.begin();
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
     

  delay(1000);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF); //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA); //Only Station No AP, This line hides the viewing of ESP as wifi hotspot

  WiFi.begin(ssid, password); //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); //IP address assigned to your ESP
}

//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop()
{
  String Ttoken;                //Declare string to hold Tado authentication token
  WiFiClientSecure httpsClient; //Declare object of class WiFiClient

//#######################################################################################################################################
// Read local device temperature, set actTemp
  
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }
actTemp = (event.temperature - 0.4);
     Serial.print(F("actTemp: "));
    Serial.print(actTemp);
    Serial.println(F("°C"));


  // //##################################################################################################################################
  // //Obtain Authentication Token from Tado 
  
  
  Serial.println(host);

  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting");
  int r = 0; //retry counter
  while ((!httpsClient.connect(host, httpsPort)) && (r < 30))
  {
    delay(100);
    Serial.print(".");
    r++;
  }
  if (r == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  String getData, Link;

  //POST Data
  // Link = "/post";
  Link = "/oauth/token";
  Serial.print("requesting URL: ");
  Serial.println(host);

  String httpText = (String("POST ") + Link + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Content-Type: application/x-www-form-urlencoded" + "\r\n" +
                     "Accept: */*" + "\r\n" +
                     "Cache-Control: no-cache" + "\r\n" +
                     "Host: auth.tado.com" + "\r\n" +
                     "Content-Length: 191" + "\r\n" +
                     "Connection: keep-alive" + "\r\n" +
                     +"\r\n" +
                     "client_id=tado-web-app&grant_type=password&scope=home.user&username=" + tlogin + "&password=" + tpassword + "&client_secret=wZaRN7rpjn3FoNyF5IFuxg9uMzYJcvOoQ8QWiIqS3hfk6gLhVlG57j5YNoZL2Rtc"
                     +"Connection: close\r\n\r\n"
                     );

  httpsClient.print(httpText);

  Serial.println("HTTP request is: ");
  Serial.println(httpText);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  String linez;
  String lineT;

    linez = httpsClient.readStringUntil('\n'); //Read Line by Line
    lineT = httpsClient.readStringUntil('\n'); //Read Line by Line
   
   //httpsClient.stop

    //Print response
   // Serial.println("LineT:");
   // Serial.println(lineT); 
  
    Ttoken = lineT.substring(17,916);
  
    Serial.println("==========");
  
 // Serial.println("Token Value:");
 // Serial.println(Ttoken);

//########################################################################################################################################
// GET thermostat temperature from Tado

 Serial.println(host2);

  Serial.printf("Using fingerprint '%s'\n", fingerprint2);
  httpsClient.setFingerprint(fingerprint2);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting");
  int rr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rr < 30))
  {
    delay(100);
    Serial.print(".");
    rr++;
  }
  if (rr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  
  Serial.print("requesting URL: ");
  Serial.println(host2);

  String httpText2 = (String("GET /api/v2/homes/" + thome + "/zones/2/state HTTP/1.1") + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Authorization: Bearer " + Ttoken + "\r\n" +
                     "Accept: */*" + "\r\n" +
                     "Cache-Control: no-cache" + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Connection: close\r\n\r\n"
                     );

  httpsClient.print(httpText2);

  Serial.println("HTTP request is: ");
  Serial.println(httpText2);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
 
  }
 
  Serial.println("reply was:");
  Serial.println("==========");

  String linez2;
  String lineT2;

    linez2 = httpsClient.readStringUntil('\n'); //Read Line by Line
    lineT2 = httpsClient.readStringUntil('\n'); //Read Line by Line

Serial.println("\r\n lineT2");
Serial.println(lineT2); 
Serial.println("\r\n");
//###################################################################################################################
// JSON split to get Tado Temp
// See https://arduinojson.org/v6/assistant/     and    https://arduinojson.org/v6/api/json/deserializejson/ 


const size_t capacity = 3*JSON_OBJECT_SIZE(1) + 5*JSON_OBJECT_SIZE(2) + 4*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(13) + 710;
DynamicJsonDocument doc(capacity);

//const char* json = "{\"tadoMode\":\"HOME\",\"geolocationOverride\":false,\"geolocationOverrideDisableTime\":null,\"preparation\":null,\"setting\":{\"type\":\"HEATING\",\"power\":\"ON\",\"temperature\":{\"celsius\":21,\"fahrenheit\":69.8}},\"overlayType\":null,\"overlay\":null,\"openWindow\":null,\"nextScheduleChange\":{\"start\":\"2019-12-22T00:00:00Z\",\"setting\":{\"type\":\"HEATING\",\"power\":\"ON\",\"temperature\":{\"celsius\":18,\"fahrenheit\":64.4}}},\"nextTimeBlock\":{\"start\":\"2019-12-22T00:00:00.000Z\"},\"link\":{\"state\":\"ONLINE\"},\"activityDataPoints\":{\"heatingPower\":{\"type\":\"PERCENTAGE\",\"percentage\":0,\"timestamp\":\"2019-12-21T14:12:53.522Z\"}},\"sensorDataPoints\":{\"insideTemperature\":{\"celsius\":21.48,\"fahrenheit\":70.66,\"timestamp\":\"2019-12-21T14:22:48.761Z\",\"type\":\"TEMPERATURE\",\"precision\":{\"celsius\":1,\"fahrenheit\":1}},\"humidity\":{\"type\":\"PERCENTAGE\",\"percentage\":52.4,\"timestamp\":\"2019-12-21T14:22:48.761Z\"}}}";

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, lineT2);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

// Fetch values
// Most are not required, so commented out but left in for reference   
//const char* tadoMode = doc["tadoMode"]; // "HOME"
//bool geolocationOverride = doc["geolocationOverride"]; // false

//JsonObject setting = doc["setting"];
//const char* setting_type = setting["type"]; // "HEATING"
//const char* setting_power = setting["power"]; // "ON"

//float setting_temperature_celsius = setting["temperature"]["celsius"]; // 21
//float setting_temperature_fahrenheit = setting["temperature"]["fahrenheit"]; // 69.8

// const char* nextScheduleChange_start = doc["nextScheduleChange"]["start"]; // "2019-12-22T00:00:00Z"

// JsonObject nextScheduleChange_setting = doc["nextScheduleChange"]["setting"];
// const char* nextScheduleChange_setting_type = nextScheduleChange_setting["type"]; // "HEATING"
// const char* nextScheduleChange_setting_power = nextScheduleChange_setting["power"]; // "ON"

// int nextScheduleChange_setting_temperature_celsius = nextScheduleChange_setting["temperature"]["celsius"]; // 18
// float nextScheduleChange_setting_temperature_fahrenheit = nextScheduleChange_setting["temperature"]["fahrenheit"]; // 64.4

// const char* nextTimeBlock_start = doc["nextTimeBlock"]["start"]; // "2019-12-22T00:00:00.000Z"

// const char* link_state = doc["link"]["state"]; // "ONLINE"

// JsonObject activityDataPoints_heatingPower = doc["activityDataPoints"]["heatingPower"];
// const char* activityDataPoints_heatingPower_type = activityDataPoints_heatingPower["type"]; // "PERCENTAGE"
// int activityDataPoints_heatingPower_percentage = activityDataPoints_heatingPower["percentage"]; // 0
// const char* activityDataPoints_heatingPower_timestamp = activityDataPoints_heatingPower["timestamp"]; // "2019-12-21T14:12:53.522Z"

JsonObject sensorDataPoints_insideTemperature = doc["sensorDataPoints"]["insideTemperature"];
float sensorDataPoints_insideTemperature_celsius = sensorDataPoints_insideTemperature["celsius"]; // 21.48
// float sensorDataPoints_insideTemperature_fahrenheit = sensorDataPoints_insideTemperature["fahrenheit"]; // 70.66
// const char* sensorDataPoints_insideTemperature_timestamp = sensorDataPoints_insideTemperature["timestamp"]; // "2019-12-21T14:22:48.761Z"
// const char* sensorDataPoints_insideTemperature_type = sensorDataPoints_insideTemperature["type"]; // "TEMPERATURE"

// int sensorDataPoints_insideTemperature_precision_celsius = sensorDataPoints_insideTemperature["precision"]["celsius"]; // 1
// int sensorDataPoints_insideTemperature_precision_fahrenheit = sensorDataPoints_insideTemperature["precision"]["fahrenheit"]; // 1

// JsonObject sensorDataPoints_humidity = doc["sensorDataPoints"]["humidity"];
// const char* sensorDataPoints_humidity_type = sensorDataPoints_humidity["type"]; // "PERCENTAGE"
// float sensorDataPoints_humidity_percentage = sensorDataPoints_humidity["percentage"]; // 52.4
// const char* sensorDataPoints_humidity_timestamp = sensorDataPoints_humidity["timestamp"]; // "2019-12-21T14:22:48.761Z"

  // Print values.
  // Serial.print("setting_temperature_celsius: - ");
  // Serial.println(setting_temperature_celsius);
  Serial.print("sensorDataPoints_insideTemperature_celsius: ");
  Serial.println(sensorDataPoints_insideTemperature_celsius);

//###################################################################################################################
// GET existing offset Value

Serial.println(host2);

  Serial.printf("Using fingerprint '%s'\n", fingerprint2);
  httpsClient.setFingerprint(fingerprint2);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting");
  int rrrr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rrrr < 30))
  {
    delay(100);
    Serial.print(".");
    rrrr++;
  }
  if (rrrr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  
  Serial.print("requesting URL: ");
  Serial.println(host2);


  String httpText4 = (String("GET /api/v2/devices/" + tdevice + "/temperatureOffset HTTP/1.1") + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Authorization: Bearer " + Ttoken + "\r\n" +
                     "Accept: */*" + "\r\n" +
                     "Cache-Control: no-cache" + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Connection: close\r\n\r\n"
                     );

  httpsClient.print(httpText4);

  Serial.println("HTTP request is: ");
  Serial.println(httpText4);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
 
  }
 
  Serial.println("reply was:");
  Serial.println("==========");

  String linez4;
  String lineT4;

    linez4 = httpsClient.readStringUntil('\n'); //Read Line by Line
    lineT4 = httpsClient.readStringUntil('\n'); //Read Line by Line

Serial.println("\r\n lineT4");
Serial.println(lineT4); 
Serial.println("\r\n");

//################################################################################################################
// JSON split to get Tado Offset

const size_t capacity2 = JSON_OBJECT_SIZE(2) + 30;
DynamicJsonDocument doc2(capacity2);

//const char* json = "{\"celsius\":-21.8,\"fahrenheit\":-35.24}";
DeserializationError error2 = deserializeJson(doc2, lineT4);

  // Test if parsing succeeds.
  if (error2) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error2.c_str());
    return;
  }

txoffset = doc2["celsius"]; // -21.8


//###################################################################################################################
// Calculate offset value required
  Serial.print("Actual Temp: ");
  Serial.println(actTemp);
  Serial.print("Current Offset: ");
  Serial.println(txoffset); 
  Serial.println("\r\n");
  Serial.print("Tado reading inc. current offset: ");
  Serial.println(sensorDataPoints_insideTemperature_celsius); 
  Serial.println("\r\n");

toffset = actTemp - (sensorDataPoints_insideTemperature_celsius - txoffset);
  Serial.print("Offset calculated: ");
  Serial.println(toffset);
  Serial.print("Offset to submit: ");
 // Serial.printf("%.1f",toffset);
  sprintf(toffsetS,"%.1f",toffset);
  //Serial.println("toffsetS: ");
  Serial.println(toffsetS);
//###################################################################################################################
// PUT Offset to Tado

  Serial.println(host2);
  Serial.printf("Using fingerprint '%s'\n", fingerprint2);
  httpsClient.setFingerprint(fingerprint2);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting");
  int rrr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rrr < 30))
  {
    delay(100);
    Serial.print(".");
    rrr++;
  }
  if (rrr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  Serial.print("requesting URL: ");
  Serial.println(host2);

  String httpBody3 =  (String("{\r\n") +
                        "            celsius: "  + toffsetS
                        + "\r\n" +
                        "}"
  );
  int httpBodyLength3 = httpBody3.length();

  // Serial.print("httpBody3 Length: ");
  // Serial.println(httpBodyLength3);

String httpText3 = (String("PUT /api/v2/devices/" + tdevice + "/temperatureOffset HTTP/1.1") + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Authorization: Bearer " + Ttoken + "\r\n" +
                     "Accept: */*" + "\r\n" +
                     "Cache-Control: no-cache" + "\r\n" +
                     "Host: my.tado.com" + "\r\n" +
                     "Content-Length: " + httpBodyLength3 + "\r\n" +
                     "Connection: keep-alive\r\n\r\n" +
                    httpBody3
                     );

  httpsClient.print(httpText3);

  Serial.println("HTTP request is: ");
  Serial.println(httpText3);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
 
  }
 
  Serial.println("reply was:");
  Serial.println("==========");

  String linez3;
  String lineT3;

    linez3 = httpsClient.readStringUntil('\n'); //Read Line by Line
    lineT3 = httpsClient.readStringUntil('\n'); //Read Line by Line


// Serial.println("\r\n linez3");
// Serial.println(linez3); 
// Serial.println("\r\n");

Serial.println("\r\n lineT3");
Serial.println(lineT3); 
Serial.println("\r\n");


//###################################################################################################################
  // Serial.println();
  Serial.println("==========");
  Serial.println("closing connection");

  delay(300000); //POST Data at every 5 minute
}
//=======================================================================
