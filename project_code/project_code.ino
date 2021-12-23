/*
  Web Server Based Electrical Monitoring System
  See the complete project details at: https://github.com/raihannur45/Web-Server-Based-Electrical-Monitoring-System
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
//libraries used
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#define sensorIn 34 //analog pin that connect to sensor
#define mVperAmp 66  //30 A ACS712 sensitivity. You can use 100 for 20 A or 185 for 5 A
#define Vstandar 200 //AC voltage of power outlet. You can measure it first with voltmeter
#define LED 2 //LED built in ESP32
#define interval 10000000 //watchdog timer interval

int ADC0 = 0; //variable that used to store ADC value for no load condition
int readValue = 0;
double Ampere = 0;
double P = 0;
double totalP = 0;
double PConsumption = 0;
int counter = 0;
int detik;
int PExceed = 0;  //variable that indicate whether the power consumption has reached the limit or not

//ssid and password of your network
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

String input_field = "999999";  //variabel that used to store the limit of power consumption

//variable for watchdog timer
hw_timer_t * timer0 = NULL;

AsyncWebServer server(80);

//function to reset ESP32
void IRAM_ATTR watchDogInterrupt() {
  Serial.println("reboot");
  ESP.restart();
}

//reset watchdog timer
void watchDogRefresh() {
  timerWrite(timer0, 0); //reset timer (feed watchdog)
}

//fungsi multitasking for LED indicator
void LED_indicator(void * parameter) {
  for (;;) {
    //ketika ESP32 belum terkoneksi WiFi
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED, HIGH);
      vTaskDelay(250 / portTICK_PERIOD_MS); //delay 500ms
      digitalWrite(LED, LOW);
      vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    //ketika ESP32 sudah terkoneksi WiFi
    else if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED, HIGH);
    }
  }
}

String processor(const String& var){
  if(var == "AMPERE"){
    return getAmpere();
  } else if(var == "PCONSUMPTION"){
    return powerconsumption();
  } else if(var == "PMAX"){
    return input_field;
  } else if(var == "ALERT"){
    return getalert();
  }
  return String();
}

const char* input_parameter = "threshold_input";

void setup() {
  Serial.begin(115200);

  //inisialisasi watchdog timer
  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &watchDogInterrupt, true);
  timerAlarmWrite(timer0, interval, true);
  timerAlarmEnable(timer0);
  pinMode(LED, OUTPUT);

  //find the average of ADC value for no load condition
  for(int i=0; i<2000; i++){
    ADC0 += analogRead(sensorIn);
  }
  ADC0 = ADC0 / 2000;

  xTaskCreate(
    LED_indicator, // Function that should be called
    "LED Indicator", // Name of the task (for debugging)
    1000, // Stack size (bytes)
    NULL, // Parameter to pass
    1, // Task priority
    NULL // Task handle
  );

  //initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //connect to your local network
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {}
  watchDogRefresh();
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("IP: ");
  //IP Address that used to open the web server
  Serial.println(WiFi.localIP());

  //request for the web page /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/webserver.html");
  });

  //request for the web page /ampere
  server.on("/ampere", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getAmpere().c_str());
  });
  
  //request for the web page /power
  server.on("/power", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getPower().c_str());
  });

  //request for the web page /pconsumption
  server.on("/pconsumption", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", powerconsumption().c_str());
  });

  //request for the web page when the user inputs power limitation
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    if (request->hasParam(input_parameter)) {
      input_field = request->getParam(input_parameter)->value();
    }
    Serial.println(input_field);
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });

  //request for the web page /alert
  server.on("/alert", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", getalert().c_str());
  });

  //memulai server
  server.begin();
}

void loop() {
  watchDogRefresh();  //reset timer watchdog if connected to local network
  while (WiFi.status() != WL_CONNECTED) {}  //infinite loop until connected back, will trigger watchdog timer if not conneeccted for 10 seconds

  //finding the average of the sensor readings 2000 times
  for(int i=0; i<2000; i++){
    readValue += analogRead(sensorIn);
  }
  readValue = readValue / 2000;
    
  //calculating the read current value
  double Vref = ((readValue - ADC0) * 5) / 4095;
  Ampere = (Vref * 1000) / mVperAmp;
  //kisaran nilai 0 pada sensor ACS712
  if (Ampere > -0.015 && Ampere < 0.008) {
    Ampere = 0;
  }
 
  //calculating the power and check the status
  P = Vstandar * Ampere;  
  totalP += P;
  counter++;
  detik = millis()/1000;
  PConsumption = (totalP * detik) / (counter * 3600);
  //if power consumption exceed the limitation
  if(PConsumption > input_field.toFloat()){
    PExceed = 1;
  } //if power consumption not exceed the limitation
  else if(PConsumption < input_field.toFloat()){
    PExceed = 0;
  }
}

//taking current value from measurement result to display on webserver
String getAmpere() {
  if (isnan(Ampere)) {
    Serial.println("Failed to read from ACS712 sensor!");
    return "";
  } else{
    Serial.print("Ampere: ");
    Serial.println(Ampere);
    return String(Ampere); 
  }
}

//taking power value from measurement result to display on webserver
String getPower() {
  if (isnan(P)) {
    Serial.println("Failed to read from ACS712 sensor!");
    return "";
  } else {
    Serial.print("Power: ");
    Serial.println(P);
    return String(P);
  }
}

//taking power consumption value to display on webserver
String powerconsumption() {
  if (isnan(PConsumption)) {
    Serial.println("Failed to read from ACS712 sensor!");
    return "";
  } else {
    Serial.print("Total Power detected: ");
    Serial.println(totalP);
    Serial.print("Detik: ");
    Serial.println(detik);
    Serial.print("Power consumption: ");
    Serial.println(PConsumption);
    return String(PConsumption);
  }
}

//printing a warning sentence
String getalert(){
  const char* a = " ";
  const char* b = "Your power consumption is exceed the threshold";
  if (isnan(PExceed)) {
    Serial.println("Failed to read from ACS712 sensor!");
    return "";
  } else {
    Serial.println(PExceed);
    if(PExceed == 0){
      Serial.println(a);
      return String(a);
    }
    else if(PExceed == 1){
      Serial.println(b);
      return String(b);
    }
  }
}
