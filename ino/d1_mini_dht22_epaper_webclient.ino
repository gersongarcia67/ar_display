//Libraries
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>

// Communication support
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <TimeLib.h>

// mapping suggestion from Waveshare SPI e-Paper to Wemos D1 mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/TomThumb.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

//======================= BEGIN OF CUSTOM DATA =======================

// Your network ssid
const char* ssid = "YOUR_SSID";

// Its password
const char* password = "YOUR_SSID_PASSWORD";

// IP address of your API server
const String address="http://YOUR_WEB_SERVER:5050/json";

// ToSerial=1 if serial is availabe
boolean ToSerial=0;

// Your device name
String devicename="YOUR_DEVICE_NAME";

//GMT Time Zone with sign
//#define GMT_TIME_ZONE -5 // CDT
#define GMT_TIME_ZONE -6 // CST

//======================= END OF CUSTOM DATA =======================

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D1*/ 5);
GxEPD_Class display(io, /*RST=D4*/ 2, /*BUSY=D2*/ 4); // default selection of D4(=2), D2(=4)

//Constants
#define DHTPIN D6     // what pin we're connected to
#define DHTTYPE DHT22   // DHT22

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
//Variables
float hum;  //Stores humidity value
float temp; //Stores temperature value
float ftemp;
float ktemp;
float rtemp;

//closest NTP Server
#define NTP_SERVER "0.us.pool.ntp.org"

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
//NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_TIME_ZONE * 3600 , 60000);
NTPClient timeClient(ntpUDP, NTP_SERVER, 0 , 60000);

WiFiClient espClient;

IPAddress myIP;                    // the IP address of your shield
String str_ip;

void setup() 
{
  if(ToSerial) Serial.begin(74880);

  if(ToSerial) Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    if(ToSerial) Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  myIP = WiFi.localIP();
  str_ip= String(myIP[0]) + "." + String(myIP[1]) + "." + String(myIP[2]) + "." + String(myIP[3]);

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(devicename)) {
    if(ToSerial) Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  if(ToSerial) Serial.println("mDNS responder started");

  if(ToSerial) Serial.println("Device IP: "+str_ip);

  //Initialize the DHT sensor
  dht.begin();  

  // Init epaper display
  display.init(74880); // enable diagnostic output on Serial
  
}

void loop() 
{ 

  if(ToSerial) Serial.println("Contact ntp server and update clock.");

  if ((WiFi.status() == WL_CONNECTED)) {

    // Don't bother if wifi is not connected.

    if (MDNS.begin(devicename)) {
      MDNS.update();
      if(ToSerial) Serial.println("MDNS responder started");
    }

    // Set time from NTP
    timeClient.begin();
    timeClient.update();

  }
    
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {

    MDNS.update();

    WiFiClient client;
    HTTPClient http;

    // Set Json up
    // https://arduinojson.org/v6/example/string/

    DynamicJsonDocument doc(1024);
    JsonObject obj = doc.to<JsonObject>();

    // Get current timestamp
    String devicetime=getDeviceTime();
    if(ToSerial) Serial.println("Time from NTP: "+devicetime);
    obj["devicetime"]=devicetime;

    // Also send the local time. set GMT_TIME_ZONE
    obj["device_localtime"]=getDeviceTimeDisplay();

    if(ToSerial) Serial.println("Device Name: "+devicename);
    obj["devicename"]=devicename;

    // Add device IP address
    obj["deviceip"]=str_ip;

    // Add sensor type
    obj["sensortype"]="DHT22";

    float converted = 0.00;
    
    //Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp= dht.readTemperature();

    //Fahrenheit
    //T(°F) = T(°C) × 9/5 + 32
    ftemp = ( temp * 1.8 ) + 32;

    //Kelvin
    //T(K) = T(°C) + 273.15          
    ktemp = temp + 273.15;
      
    //Rankine
    //T(°R) = (T(°C) + 273.15) × 9/5          
    rtemp = temp + 273.15;
    rtemp = (rtemp * 1.8);
      
    if(ToSerial) Serial.print("Celsius = ");
    if(ToSerial) Serial.print(temp);
    if(ToSerial) Serial.println("C");
    if(ToSerial) Serial.print("Fahrenheit = ");
    if(ToSerial) Serial.print(ftemp);
    if(ToSerial) Serial.println("F");
    if(ToSerial) Serial.print("Kelvin = ");
    if(ToSerial) Serial.print(ktemp);
    if(ToSerial) Serial.println("K");
    if(ToSerial) Serial.print("Rankin = ");
    if(ToSerial) Serial.print(rtemp);
    if(ToSerial) Serial.println("R");
    if(ToSerial) Serial.print("Humidity =");
    if(ToSerial) Serial.println(hum);
    if(ToSerial) Serial.println();

    // Add sensor data to json

    obj["temp_c"]=String(temp);
    obj["humidit"]=String(hum);
    obj["temp_f"]=String(ftemp);
    obj["temp_kevin"]=String(converted);
    obj["temp_rankin"]=String(rtemp);
    // Add sketch name
    obj["sketch_name"]=__FILE__;
    
    String jsonout;
    serializeJson(doc, jsonout);

    // Send to ar-api
    sendToARapi(jsonout);
      
    if(ToSerial) Serial.println("going into deep sleep mode for 10 mins");
  
    ESP.deepSleep(1e6*60*10);

  }

}

/* FUNCTIONS */

void sendToARapi(String jsonout) {

  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, address); //HTTP
  http.addHeader("Content-Type", "application/json");
 
  if(ToSerial) Serial.println("Server: "+address);
  if(ToSerial) Serial.println("Sending: \""+jsonout+"\"");
  int httpCode = http.POST(jsonout);
  
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    if(ToSerial) Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = http.getString();
      if(ToSerial) {
        Serial.println("received payload:\n<<");
        Serial.println(payload);
        Serial.println(">>");
      }
    }
  } else {
    if(ToSerial) Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();

  return;

}

String getDeviceTime() {
  
  String str_time="";
  long t=timeClient.getEpochTime();

  str_time+=formatDigits(int(month(t)));
  str_time+="/";
  str_time+=formatDigits(int(day(t)));
  str_time+="/";
  str_time+=formatDigits(int(year(t)));
  str_time+=" ";
  str_time+=formatDigits(int(hour(t)));
  str_time+=":";
  str_time+=formatDigits(int(minute(t)));
  str_time+=":";
  str_time+=formatDigits(int(second(t))); 

  return str_time;
  
}

String getDeviceTimeDisplay() {
  
  String str_time="";
  long t=timeClient.getEpochTime()-(GMT_TIME_ZONE * 3600);
  
  str_time+=formatDigits(int(month(t)));
  str_time+="/";
  str_time+=formatDigits(int(day(t)));
  str_time+="/";
  str_time+=formatDigits(int(year(t)));
  str_time+=" ";
  str_time+=formatDigits(int(hour(t)));
  str_time+=":";
  str_time+=formatDigits(int(minute(t)));
  str_time+=":";
  str_time+=formatDigits(int(second(t))); 

  return str_time;
  
}

String formatDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  String str="";
  if(digits < 10)
    str+='0';
  str+=String(digits);
  return str;
}
