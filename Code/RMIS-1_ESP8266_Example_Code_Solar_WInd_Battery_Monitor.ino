/*
  Dave Williams, G8PUO, DitroniX 2019-2022 (ditronix.net)
  RMIS ESP6288 DC Remote Monitor IoT System ESP-12S SDK (Wifi Enabled, ThingSpeak Enabled)
  PCA v2204-106 - Test Code Firmware 1.220418 - 18th April 2022

  This example code has been developed for a test Solar/Wind/Battery/UPS project - which is work in progress.
  Monitoring the voltage and charge current from Solar and Wind, and also monitoring the charge/discharge of the batteries.

  Each of the ADC outputs are multi-sampled, then averaged [AverageSamples], in order to remove/reduce jitter
  A small level of Hysteresis [HysteresisValue], is then applied to help reduce duplicate postings.
  These provide more stable updates of each channel, as they are looped, so reducing duplicate updates to ThingSpeak.
  When a value changes on each channel a + is displayed at the end of the line and a batch update to ThingSpeak is made.
  Should an error be encountered when posting an update, the ESP will automatically reboot to re-initialise the connection.

  The purpose of this test code is to cycle through the various main functions of the board as part of bring up testing.
  Read data is compared and then only updated if changed.  Example output: https://thingspeak.com/channels/1697689

  This test code is OPEN SOURCE and although is not intended for real world use, it may be freely used, or modified as needed.

  WIKI.  Please see https://ditronix.net/wiki/ for further information
*/

/* 
  Examaple Output

  0.00   Current A -/+ (510)   LastWindCurrent
  0.52   Voltage V (4)     LastWindVoltage
  0.06   Current A -/+ (510)   LastSolarCurrent
  13.44   Voltage V (116)   LastSolarVoltage
  0.17   Current A -/+ (512)   LastBatteryCurrent
  55.32   Voltage V (480)   LastBatteryVoltage
  12.39   Voltage V (828)   LastPCBVoltage +
  30.6   Temperature °C (30)   LastTemperature
  Voltage  12.39  System Uptime 0 days, 13 hours, 41 minutes, 1 seconds
  Channel xxxx update successful.

  Input Current and Sensitivity (measurement scale)
  -/+ 5A – 185 mV/A
  -/+ 20A – 100 mV/A
  -/+ 30 A – 66 mv/A
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "Wire.h"
#include "Adafruit_MCP9808.h"   // Temperature Sensor
#include <Adafruit_MCP3008.h>   // 8 Channel ADC
#include "ThingSpeak.h"         // ThingSPeak IoT
#include "uptime_formatter.h" //https://github.com/YiannisBourkelis/Uptime-Library

// ######### OBJECTS #########
Adafruit_MCP9808 TempSensor = Adafruit_MCP9808();
Adafruit_MCP3008 adc;

// ######### VARIABLES / DEFINES / STATIC #########

// App
String AppVersion = "v1.220418";
String AppBuild = "DitroniX RMIS ESP6288 SDK PCA v2204-106";
String AppName = "G8PUO Solar, Wind Generator and Battery Monitor";

// Variables
char SensorResult[10];
float SensorRAW;
float SensorValue;
float LastWindCurrent;
float LastWindVoltage;
float LastSolarCurrent;
float LastSolarVoltage;
float LastBatteryCurrent;
float LastBatteryVoltage;
float LastDumpCurrent;
float LastDumpVoltage;
float LastTemperature;
float LastPCBVoltage;
boolean UpdateFlag;

// Constants
const int HysteresisValue = 1;  // RAW Value +/- Threshold (Software Window Comparator)
const int AverageSamples = 100; // Average Multi-Samples on each Channel.

// MCP9808 Temp Sensor
int MCP9808 = 0x18;  // PCB Temperature Sensor Address

// WiFi
const char* ssid = "xxxx";       // network SSID (name)
const char* password = "xxxx";    // network password
String Hostname = "ESP-RMIS-";  // Hostname Prefix
WiFiClient client; // Initialize the client library

// ThingSpeak
unsigned long myChannelNumber = 0; // Put your ThingSpeak Channel ID here
const char * myWriteAPIKey = "xxxx";  // Put your WriteAPIKey here

// **************** IO ****************

// Define I2C (Expansion Port)
#define I2C_SDA 4
#define I2C_SCL 5

// Define SPI (Expansion Port)
#define SPI_MISO 12
#define SPI_MOSI 13
#define SPI_SCK 14
#define SPI_SS 15

// Define GPIO 16 for General IO or 1-Wire Use.
#define GP16 16

// **************** OUTPUTS ****************
#define LED_Status 2  // Define ESP Output Port LED

// **************** INPUTS ****************
#define ADC A0  // Define ADC (0 DC - 15V DC)

// ######### FUNCTIONS #########

// Scan I2C for Devices
void scan_i2c_devices() {
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

// Initialise WiFi and ThingSpeak
void InitialiseWiFi() {

  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect to " + String(ssid));

    WiFi.begin(ssid, password);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);

    // Stabalise for slow Access Points
    delay(5000);

    // Force Hostname
    Hostname = Hostname + WiFi.macAddress().substring(WiFi.macAddress().length() - 5, WiFi.macAddress().length());
    Hostname.replace(":", "");
    WiFi.setHostname(Hostname.c_str());

    // Wifi Information
    Serial.println("WiFi SSID \t " + String(ssid)) + "(Wifi Station Mode)";
    Serial.printf("WiFi IP \t %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("WiFi GW \t %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("WiFi MASK \t %s\n", WiFi.subnetMask().toString().c_str());
    Serial.println("WiFi MAC \t " + WiFi.macAddress());
    Serial.printf("WiFi Hostname \t %s\n", WiFi.hostname().c_str());
    Serial.println("WiFi RSSI \t " + String(WiFi.RSSI()));
    Serial.println("");

    // Initialise ThingSpeak Connection
    ThingSpeak.begin(client);  // Initialize ThingSpeak
    Serial.println("Initialised ThingSpeak");
  }
}

// Initialise Temperature Sensor
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
    //TempSensor.wake(); // Wake Up
    SensorRAW = TempSensor.readTempC();
    Serial.print("Test PCB Temp: ");
    Serial.print(SensorRAW); Serial.println("°C");
    //TempSensor.shutdown_wake(1); // Sleep Optional
  }
}

// Calculate Average Value and Reduce Jitter
float CalculateAverage (int SensorChannel)
{
  float AverageRAW = 0;
  for (int i = 0; i < AverageSamples; i++) {
    AverageRAW = AverageRAW + adc.readADC(SensorChannel);
    delay(1);
  }
  AverageRAW = AverageRAW / AverageSamples;
  return AverageRAW;
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

  // WiFi
  InitialiseWiFi();

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

  // Example of RMIS monitoring port pin allocation. Channels arranged for test purposes.
  // Channel 1 Wind Current (Pins 1+ and 2)
  // Channel 1 Wind Voltage (Pin 1+)
  // Channel 2 Solar Current (Pins 3+ and 4)
  // Channel 2 Solar Voltage (Pin 3+)
  // Channel 3 Battery  Current (Pins 5+ and 6)
  // Channel 3 Battery Voltage (Pin 5+)
  // Channel 4 not used

  // Wind Current Channel 1
  // Read Channel 1 Current 510 = 0A Using 30A Sensor (Calibrated)
  SensorRAW = CalculateAverage(0);
  SensorValue = ( SensorRAW - 510 ) / 16.00;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t LastWindCurrent" );
  if (SensorRAW < LastWindCurrent - HysteresisValue || SensorRAW > LastWindCurrent + HysteresisValue) {
    LastWindCurrent = SensorRAW;
    ThingSpeak.setField(1, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Wind Voltage Channel 1
  // Read Channel 1 Voltage 0-5V 0-1023 (Calibrated)
  SensorRAW = CalculateAverage(1);
  SensorValue = ( SensorRAW * 117.87 ) / 1024;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t\t LastWindVoltage" );
  if (SensorRAW < LastWindVoltage - HysteresisValue || SensorRAW > LastWindVoltage + HysteresisValue) {
    LastWindVoltage = SensorRAW;
    ThingSpeak.setField(2, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Solar Current Channel 2
  // Read Channel 2 Current 510 = 0A Using 30A Sensor (Calibrated)
  SensorRAW = CalculateAverage(2);
  SensorValue = ( SensorRAW - 510 ) / 16.00;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t LastSolarCurrent" );
  if (SensorRAW < LastSolarCurrent - HysteresisValue || SensorRAW > LastSolarCurrent + HysteresisValue) {
    LastSolarCurrent = SensorRAW;
    ThingSpeak.setField(3, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Solar Voltage Channel 2
  // Read Channel 2 Voltage 0-5V 0-1023 (Calibrated)
  SensorRAW = CalculateAverage(3);
  SensorValue = ( SensorRAW * 117.87 ) / 1024;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t LastSolarVoltage" );
  if (SensorRAW < LastSolarVoltage - HysteresisValue || SensorRAW > LastSolarVoltage + HysteresisValue) {
    LastSolarVoltage = SensorRAW;
    ThingSpeak.setField(4, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Battery Current Channel 3
  // Read Channel 3 Current 510 = 0A Using 30A Sensor (Calibrated)
  SensorRAW = CalculateAverage(4);
  SensorValue = ( SensorRAW - 510 ) / 16.00;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t LastBatteryCurrent" );
  if (SensorRAW < LastBatteryCurrent - HysteresisValue || SensorRAW > LastBatteryCurrent + HysteresisValue) {
    LastBatteryCurrent = SensorRAW;
    ThingSpeak.setField(5, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Battery Voltage Channel 3
  // Read Channel 3 Voltage 0-5V 0-1023 (Calibrated)
  SensorRAW = CalculateAverage(5);
  SensorValue = ( SensorRAW * 117.87 ) / 1024;
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t LastBatteryVoltage" );
  if (SensorRAW < LastBatteryVoltage - HysteresisValue || SensorRAW > LastBatteryVoltage + HysteresisValue) {
    LastBatteryVoltage = SensorRAW;
    ThingSpeak.setField(6, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Dump Current Channel 4
  // Reserved.  Channels 7 & 8 are used for the below during development.


  // PCB Voltage
  SensorRAW = analogRead(ADC);
  SensorValue = (SensorRAW * 15.32 ) / 1024; // Divider value may need tweaking
  dtostrf (SensorValue, 6, 2, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t LastPCBVoltage" );
  if (SensorRAW < LastPCBVoltage - HysteresisValue || SensorRAW > LastPCBVoltage + HysteresisValue) {
    LastPCBVoltage = SensorRAW;
    ThingSpeak.setField(7, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // PCB Temperature
  // Read PCB_Temp Sensor Temperature ºC and round to 1 decimal place
  SensorRAW = TempSensor.readTempC();
  dtostrf (SensorRAW, 6, 1, SensorResult);
  sprintf (SensorResult, "%s", SensorResult);
  Serial.print(String (SensorResult) + "\t Temperature °C (" + String(int(SensorRAW)) + ")\t LastTemperature" );
  if (SensorRAW < LastTemperature - HysteresisValue || SensorRAW > LastTemperature + HysteresisValue) {
    LastTemperature = SensorRAW;
    ThingSpeak.setField(8, SensorResult ); // Update ThingSpeak Field
    UpdateFlag = true;
    Serial.println(" +");
  } else {
    Serial.println("");
  }

  // Update ThingSpeak (Only on change)
  if (UpdateFlag == true) {

    UpdateFlag = false;

    SensorRAW = analogRead(ADC);
    SensorValue = (SensorRAW * 15.32 ) / 1024; // Divider value may need tweaking
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);

    ThingSpeak.setStatus("DEV TEST ONLY " + AppVersion + " - " + SensorResult + "V. Last: " + uptime_formatter::getUptime());
    Serial.print("Voltage " + String(SensorResult) );
    Serial.println("  System Uptime " + uptime_formatter::getUptime());

    // Write up to 8 channels to ThingSpeak. Check Update Error #
    int UpdateErrorCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

    // Check for any Update Errors
    if (UpdateErrorCode == 200)
    {
      Serial.print ("Channel ");
      Serial.println(String(myChannelNumber) + " update successful.\n");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(UpdateErrorCode));
      Serial.println("Automatically Restarting ESP .... ");
      ESP.restart();  
    }

  } else {
    Serial.println(" No Update\n");
  }

  // Hearbeat LED and Loop Delay
  digitalWrite(LED_Status, LOW);   // turn the LED ON by making the voltage HIGH
  delay(150);                       // wait for a second
  digitalWrite(LED_Status, HIGH);    // turn the LED OFF by making the voltage LOW
}
