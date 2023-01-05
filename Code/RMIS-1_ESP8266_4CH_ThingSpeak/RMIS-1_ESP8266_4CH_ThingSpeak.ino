/*
  Dave Williams, DitroniX 2019-2023 (ditronix.net)
  RMIS 12S DC Remote Monitor IoT System ESP12S SDK
  PCA v2202-100 - Test Code Firmware 1.220318 - 18th March 2022
  
  Example code for sending to ThingSpeak (Sensors Calibrated Individually)
  
  The purpose of this test code is to cycle through the various main functions of the board as part of bring up testing.
 
  This test code is OPEN SOURCE and formatted for easier viewing.  Although is is not intended for real world use, it may be freely used, or modified as needed.
  It is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

  Further information, details and examples can be found on our website wiki pages ditronix.net/wiki and github.com/DitroniX
*/

/* USB1 
* Examaple Output
* -1.19   Current A -/+ (491)
* 13.63  Voltage V (297)
* 20.21  Voltage V A (511)
* 13.69  DC Input Voltage V (915)
* 34.38  PCB Temperature °C
* Channel 1673722 update successful
* System Uptime 0 days, 0 hours, 0 minutes, 6 seconds
* 
* Input Current and Sensitivity (measurement scale)
* -/+ 5A – 185 mV/A
* -/+ 20A – 100 mV/A
* -/+ 30 A – 66 mv/A
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "Adafruit_MCP9808.h"   // Temp Sensor
#include <Adafruit_MCP3008.h>   // ADC
#include "Wire.h" 
#include "ThingSpeak.h"         // ThingSPeak IoT
#include "uptime_formatter.h" //https://github.com/YiannisBourkelis/Uptime-Library

// ######### OBJECTS #########
  Adafruit_MCP9808 TempSensor = Adafruit_MCP9808();
  Adafruit_MCP3008 adc;

// ######### VARIABLES / DEFINES / STATIC #########
   
  // App
  String AppVersion = "v1.220318";
  String AppBuild = "DitroniX RMIS ESP8266 PCA v2202-100";
  String AppName = "RMIS ThingSpeak Test";

  // Variables
  char SensorResult[10];
  float SensorRAW;
  float SensorValue;

  // MCP9808 Temp Sensor
  int MCP9808 = 0x18;  // PCB Temperature Sensor Address

  // WLAN
  const char* ssid = "xxxx";       // network SSID (name) 
  const char* password = "xxxx";    // network password
  const uint32_t connectTimeoutMs = 5000; // WiFi connect timeout per AP. Increase when connecting takes longer.
  WiFiClient  client;  
  byte macaddress[6]; 

  // ThingSpeak
  unsigned long myChannelNumber = xxxx;
  const char * myWriteAPIKey = "xxxx";  
  unsigned long timerDelay = 60; // Every 1 mins

  // **************** IO ****************
  
  // Define I2C (Expansion Port)
  #define I2C_SDA 4
  #define I2C_SCL 5   

  // Define SPI (Expansion Port)
  #define SPI_MISO 12
  #define SPI_MOSI 13
  #define SPI_SCK 14
  #define SPI_SS 15

  // **************** OUTPUTS **************** 
  #define LED_Status 2  // Define ESP Output Port LED
  
  // **************** INPUTS **************** 
  #define ADC A0  // Define ADC (0 DC - 15V DC)

// ######### FUNCTIONS #########

  void scan_i2c_devices() {
  // Scan I2C Devices
    Serial.print("Scanning I2C\t");
    byte count = 0;
    Serial.print("Found Devices: ");
    Serial.print(" Devices: ");
    for (byte i = 8; i < 120; i++)
    {
    Wire.beginTransmission(i);
      if (Wire.endTransmission() == 0)
        {
        Serial.print(" (0x");
        Serial.print(i, HEX);
        Serial.print(")\t");
        count++;
        delay(1);
        }
    }      
    Serial.print("Found ");
    Serial.print(count, HEX);
    Serial.println(" Device(s)."); 
  } 

    void InitialiseWLAN() {

    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("Attempting to connect to " + String(ssid));

      WiFi.begin(ssid, password); 
      
      // Let Stabalise
      delay(5000);     

      WiFi.mode(WIFI_STA); 
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
      
      Serial.println("WLAN SSID \t " + String(ssid)) + "(Wifi Station Mode)";
      Serial.printf("WLAN IP \t %s\n", WiFi.localIP().toString().c_str()); 
      Serial.printf("WLAN GW \t %s\n", WiFi.gatewayIP().toString().c_str());
      Serial.printf("WLAN MASK \t %s\n", WiFi.subnetMask().toString().c_str());
      Serial.println("WLAN MAC \t " + WiFi.macAddress());
      Serial.println("");

      // Stabalise
      delay(5000);  
      
      // Initialise ThingSpeak Connection
      ThingSpeak.begin(client);  // Initialize ThingSpeak
      Serial.println("Initialised ThingSpeak");
          
      }
   }  

  void InitialiseTemperatureSensor() {
    // PCB Temperature Sensor
    if (!TempSensor.begin()) 
    {
      // Sensor or I2C Issue
      Serial.println("Couldn't find MCP9808! Sensor or I2C Issue. Check the PCB !!");
    }
    else
    {
      Serial.print("Connected to Digital Temperature Sensor MCP9808\t");
  
      // Set Resolution 0.0625Â°C 250 ms and Wake Up
      TempSensor.setResolution(3);
      
       // Read PCB_Temp Sensor and Output
      TempSensor.wake(); // Wake Up
      SensorRAW = TempSensor.readTempC();
      Serial.print("Test PCB Temp: ");
      Serial.print(SensorRAW); Serial.println("°C"); 
      TempSensor.shutdown_wake(1); // Sleep
     }   
  }   

     // ######### SETUP #########
  void setup() {

    // Stabalise
    delay(500);   
  
    // Initialize UART:
    Serial.begin(115200, SERIAL_8N1);  //115200  
   while (!Serial);
    Serial.println(""); 
    Serial.println(AppVersion + " Firmware - Initialized");
    Serial.println(AppBuild);
    Serial.println(AppName);
    Serial.println(""); 
    
    // LED
    pinMode(LED_Status, OUTPUT); 
  
    // WLAN
    InitialiseWLAN();
  
    // Initialise I2C 
    Wire.begin(I2C_SDA, I2C_SCL);
    scan_i2c_devices();
    InitialiseTemperatureSensor();
  
    // Initialise ADC
    adc.begin();
  
    Serial.println(""); 
  }

  // ######### LOOP #########
  void loop() {

    // Read Channel 1 Current 510 = 0A Using 30A Sensor (Calibrated)
    SensorRAW = adc.readADC(0);
    SensorValue = ( SensorRAW - 510 ) / 16.00; 
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);        
    Serial.println(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")" );    
    ThingSpeak.setField(1, SensorResult ); // Update ThingSpeak Field
  
    // Read Channel 1 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = adc.readADC(1);
    SensorValue = ( SensorRAW * 46.98 ) / 1024;  
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);    
    Serial.println(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")" );      
    ThingSpeak.setField(2, SensorResult ); // Update ThingSpeak Field  
   
    // Read Channel 2 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = adc.readADC(2);
    SensorValue = ( SensorRAW * 40.50 ) / 1024;  //24.07 
    dtostrf (SensorValue, 6, 2, SensorResult); 
    sprintf (SensorResult, "%s", SensorResult);         
    Serial.println(String (SensorResult) + "\t Voltage V A (" + String(int(SensorRAW)) + ")" );     
    ThingSpeak.setField(3, SensorResult ); // Update ThingSpeak Field  
  
    // Read ADC RMIS DC Voltage Sensor (0-1V 0-1023).  Takes into account the protection diode.
    SensorRAW = analogRead(ADC);
    SensorValue = (SensorRAW * 15.32 ) / 1024; // Divider value may need tweaking  
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);       
    Serial.println(String (SensorResult) + "\t DC Input Voltage V (" + String(int(SensorRAW)) + ")" ); 
    ThingSpeak.setField(7, SensorResult ); // Update ThingSpeak Field
    
    // Read PCB_Temp Sensor Temperature ºC and round to 1 decimal place
    SensorRAW = TempSensor.readTempC();   
    dtostrf (SensorRAW, 6, 1, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t PCB Temperature °C");     
    ThingSpeak.setField(8, SensorResult ); // Update ThingSpeak Field
  
    // Update ThingSpeak
    ThingSpeak.setStatus(AppVersion + " Last: " + uptime_formatter::getUptime());
    Serial.println("System Uptime " + uptime_formatter::getUptime()); 
   
    // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
    // pieces of information in a channel.  Here, we write to field 1.
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    if(x == 200)
    {
      Serial.print ("Channel ");
      Serial.println(String(myChannelNumber) + " update successful.");
      Serial.println("");
    } else {      
      Serial.println("Problem updating channel. HTTP error code " + String(x));      
      Serial.println("Restarting ESP .... ");
      ESP.restart();
    }  
          
    // Hearbeat LED
    digitalWrite(LED_Status, LOW);   // turn the LED on (HIGH is the voltage level)
    delay(500);                       // wait for a second
    digitalWrite(LED_Status, HIGH);    // turn the LED off by making the voltage LOW
  
    // Loop Delay
    delay(timerDelay * 1000);
  }
