/*
  Dave Williams, G8PUO, DitroniX 2019-2023 (ditronix.net)
  RMIS ESP6288 DC Remote Monitor IoT System ESP-12S SDK (Wifi Enabled, Domoticz Enabled)
  PCA v2204-106 - Test Code Firmware 1.220710 - 10th July 2022

  Each of the ADC outputs are multi-sampled, then averaged [AverageSamples], in order to remove/reduce jitter
  A small level of Hysteresis [HysteresisValue], is then applied to help reduce duplicate postings.
  These provide more stable Indexs of each channel, as they are looped, so reducing duplicate Indexs to Domoticz.
  When a value changes on each channel a + is displayed at the end of the line and a batch Index to Domoticz is made.
  Should an error be encountered when posting an Index, the ESP will automatically reboot to re-initialise the connection.

    The purpose of this test code is to cycle through the various main functions of the board as part of bring up testing.
 
  This test code is OPEN SOURCE and formatted for easier viewing.  Although is is not intended for real world use, it may be freely used, or modified as needed.
  It is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

  Further information, details and examples can be found on our website wiki pages ditronix.net/wiki and github.com/DitroniX


  Read data is compared and then only Indexed if changed.

  // Domoticz Index Constants for Updating (If > 0 then will Index sensor)
  int Index_CH1_Voltage = 1;
  int Index_CH1_Current = 0; // Channel Ignored
  int Index_CH2_Voltage = 3;
  int Index_CH2_Current = 0; // Channel Ignored
  int Index_CH3_Voltage = 5;
  int Index_CH3_Current = 0; // Channel Ignored
  int Index_CH4_Voltage = 7;
  int Index_CH4_Current = 0; // Channel Ignored
  int Index_Temperature = 9;
  int Index_PCB_Voltage = 0; // Channel Ignored
*/

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "Wire.h"
#include "Adafruit_MCP9808.h"   // Temperature Sensor
#include <Adafruit_MCP3008.h>   // 8 Channel ADC

// ######### OBJECTS #########
Adafruit_MCP9808 TempSensor = Adafruit_MCP9808();
Adafruit_MCP3008 adc;

// ######### VARIABLES / DEFINES / STATIC #########

// App
String AppVersion = "v1.220831";
String AppBuild = "DitroniX RMIS ESP6288 SDK PCA v2204-106";
String AppName = "Remote Battery Monitor RMIS";

// Variables
char SensorResult[10];
float SensorRAW;
float SensorValue;
float Last_CH1_Voltage;
float Last_CH1_Current;
float Last_CH2_Voltage;
float Last_CH2_Current;
float Last_CH3_Voltage;
float Last_CH3_Current;
float Last_CH4_Voltage;
float Last_CH4_Current;
float Last_Temperature;
float Last_PCB_Voltage;
int BaseInxCounter;

// Constants
const int HysteresisValue = 2;  // RAW Value +/- Threshold (Software Window Comparator)
const int AverageSamples = 100; // Average Multi-Samples on each Channel.
const int AverageDelay = 2; // Average Multi-Sample Delay.
const int LoopDelay = 1; // Loop Delay in Seconds

// MCP9808 Temp Sensor
int MCP9808 = 0x18;  // PCB Temperature Sensor Address

// WiFi
const char* ssid = "xxxx";       // network SSID (name)
const char* password = "xxxx";    // network password
String Hostname = "ESP-RMIS-";  // Hostname Prefix
WiFiClient client; // Initialize the client library

// Domoticz Server info
const char* domoticz_server = "0.0.0.0";  // IP Address
int port = 8080;                                //Domoticz port

// Domoticz Index Constants for Updating (If > 0 then will update sensor)
// Set to desired index according to the Devices
int Index_CH1_Voltage = 0;
int Index_CH1_Current = 0;
int Index_CH2_Voltage = 0;
int Index_CH2_Current = 0;
int Index_CH3_Voltage = 0;
int Index_CH3_Current = 0;
int Index_CH4_Voltage = 0;
int Index_CH4_Current = 0;
int Index_Temperature = 0;
int Index_PCB_Voltage = 0;

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

// Initialise WiFi
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
    //
    //    // Force Hostname
    //    Hostname = Hostname + WiFi.macAddress().substring(WiFi.macAddress().length() - 5, WiFi.macAddress().length());
    //    Hostname.replace(":", "");
    //    WiFi.setHostname(Hostname.c_str());

    // Wifi Information
    Serial.println("WiFi SSID \t " + String(ssid)) + "(Wifi Station Mode)";
    Serial.printf("WiFi IP \t %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("WiFi GW \t %s\n", WiFi.gatewayIP().toString().c_str());
    Serial.printf("WiFi MASK \t %s\n", WiFi.subnetMask().toString().c_str());
    Serial.println("WiFi MAC \t " + WiFi.macAddress());
    Serial.printf("WiFi Hostname \t %s\n", WiFi.hostname().c_str());
    Serial.println("WiFi RSSI \t " + String(WiFi.RSSI()));
    Serial.println("");
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

    // Set Resolution 0.0625°C 250 ms and Wake Up
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
    delay(AverageDelay);
  }
  AverageRAW = AverageRAW / AverageSamples;
  if (AverageRAW < 2) AverageRAW = 0;
  return AverageRAW;
}

// Publish to Domoticz
void PublishDomoticz(int Sensor_Index, float Sensor_Value)
{
    if (client.connect(domoticz_server, port)) {
      Serial.print("Sending Message to Domoticz:");
  
      client.print("GET /json.htm?type=command&param=udevice&idx=");
      client.print(Sensor_Index);
  
      client.print("&svalue=");
      client.print(Sensor_Value);
  
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.print(domoticz_server);
      client.print(":");
  
      client.println(port);
      client.println("User-Agent: Arduino-ethernet");
      client.println("Connection: close");
      client.println();
  
      client.stop();
    }
    else
    {
      Serial.println("Not Connected");
      InitialiseWiFi();
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

  if (Index_CH1_Voltage > 0) {
    // Read Channel 1 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = CalculateAverage(1);
    SensorValue = ( SensorRAW * 152.82 ) / 1024;  //60=117.87.98  60=46.98
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t Last_CH1_Voltage" + " #" + String(Index_CH1_Voltage));
    if (SensorRAW < Last_CH1_Voltage - HysteresisValue || SensorRAW > Last_CH1_Voltage + HysteresisValue) {
      Last_CH1_Voltage = SensorRAW;
      PublishDomoticz(Index_CH1_Voltage , SensorValue);
    }
  }

  if (Index_CH1_Current > 0) {
    // Read Channel 1 Current 510 = 0A Using 30A Sensor (Calibrated)
    SensorRAW = CalculateAverage(0);
    SensorValue = ( SensorRAW - 510 ) / 16.00;
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t Last_CH1_Current" + " #" + String(Index_CH1_Current));
    if (SensorRAW < Last_CH1_Current - HysteresisValue || SensorRAW > Last_CH1_Current + HysteresisValue) {
      Last_CH1_Current = SensorRAW;
      PublishDomoticz(Index_CH1_Current, SensorValue);
    }
  }

  if (Index_CH2_Voltage > 0) {
    // Read Channel 2 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = CalculateAverage(3);
    SensorValue = ( SensorRAW * 152.40) / 1024;
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t Last_CH2_Voltage" + " #" + String(Index_CH2_Voltage));
    if (SensorRAW < Last_CH2_Voltage - HysteresisValue || SensorRAW > Last_CH2_Voltage + HysteresisValue) {
      Last_CH2_Voltage = SensorRAW;
      PublishDomoticz(Index_CH2_Voltage, SensorValue);
    }
  }

  if (Index_CH2_Current > 0) {
    // Read Channel 2 Current 510 = 0A Using 30A Sensor (Calibrated)
    SensorRAW = CalculateAverage(2);
    SensorValue = ( SensorRAW - 509 ) / 16.00;
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t Last_CH2_Current" + " #" + String(Index_CH2_Current));
    if (SensorRAW < Last_CH2_Current - HysteresisValue || SensorRAW > Last_CH2_Current + HysteresisValue) {
      Last_CH2_Current = SensorRAW;
      PublishDomoticz(Index_CH2_Current, SensorValue);
    }
  }

  if (Index_CH3_Voltage > 0) {
    // Read Channel 3 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = CalculateAverage(5);
    SensorValue = ( SensorRAW * 117.87 ) / 1024; 
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t Last_CH3_Voltage" + " #" + String(Index_CH3_Voltage));
    if (SensorRAW < Last_CH3_Voltage - HysteresisValue || SensorRAW > Last_CH3_Voltage + HysteresisValue) {
      Last_CH3_Voltage = SensorRAW;
      PublishDomoticz(Index_CH3_Voltage, SensorValue);
    }
  }

  if (Index_CH3_Current > 0) {
    // Read Channel 3 Current 510 = 0A Using 30A Sensor (Calibrated)
    SensorRAW = CalculateAverage(4);
    SensorValue = ( SensorRAW - 510 ) / 16.00;
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t Last_CH3_Current" + " #" + String(Index_CH3_Current));
    if (SensorRAW < Last_CH3_Current - HysteresisValue || SensorRAW > Last_CH3_Current + HysteresisValue) {
      Last_CH3_Current = SensorRAW;
      PublishDomoticz(Index_CH3_Current, SensorValue);
    }
  }

  if (Index_CH4_Voltage > 0) {
    // Read Channel 4 Voltage 0-5V 0-1023 (Calibrated)
    SensorRAW = CalculateAverage(7);
    SensorValue = ( SensorRAW * 117.87 ) / 1024; 
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Voltage V (" + String(int(SensorRAW)) + ")\t Last_CH4_Voltage" + " #" + String(Index_CH4_Voltage));
    if (SensorRAW < Last_CH4_Voltage - HysteresisValue || SensorRAW > Last_CH4_Voltage + HysteresisValue) {
      Last_CH4_Voltage = SensorRAW;
      PublishDomoticz(Index_CH4_Voltage, SensorValue);
    }
  }

  if (Index_CH4_Current > 0) {
    // Read Channel 4 Current 510 = 0A Using 30A Sensor (Calibrated)
    SensorRAW = CalculateAverage(6);
    SensorValue = ( SensorRAW - 510 ) / 16.00;
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Current A -/+ (" + String(int(SensorRAW)) + ")\t Last_CH4_Current" + " #" + String(Index_CH4_Current));
    if (SensorRAW < Last_CH4_Current - HysteresisValue || SensorRAW > Last_CH4_Current + HysteresisValue) {
      Last_CH4_Current = SensorRAW;
      PublishDomoticz(Index_CH4_Current, SensorValue);
    }
  }

  if (Index_Temperature > 0) {
    // Read PCB_Temp Sensor Temperature ºC and round to 1 decimal place
    SensorRAW = TempSensor.readTempC();
    dtostrf (SensorRAW, 6, 1, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t Temperature °C (" + String(int(SensorRAW)) + ")\t Last_Temperature" + " #" + String(Index_Temperature));
    if (SensorRAW < Last_Temperature - HysteresisValue || SensorRAW > Last_Temperature + HysteresisValue) {
      Last_Temperature = SensorRAW;
      PublishDomoticz(Index_Temperature, SensorRAW);
    }
  }

  if (Index_PCB_Voltage > 0) {
    // Read PCB_Voltage Sensor
    SensorRAW = analogRead(ADC);
    SensorValue = (SensorRAW * 15.32 ) / 1024; // Divider value may need tweaking
    dtostrf (SensorValue, 6, 2, SensorResult);
    sprintf (SensorResult, "%s", SensorResult);
    Serial.println(String (SensorResult) + "\t DC Voltage V (" + String(int(SensorRAW)) + ")\t Last_PCB_Voltage" + " #" + String(Index_PCB_Voltage));
    if (SensorRAW < Last_PCB_Voltage - HysteresisValue || SensorRAW > Last_PCB_Voltage + HysteresisValue) {
      Last_PCB_Voltage = SensorRAW;
      PublishDomoticz(Index_PCB_Voltage, SensorValue);
    }
  }

  Serial.println("------------------");

  // Hearbeat LED and Loop Delay
  digitalWrite(LED_Status, LOW);   // turn the LED ON by making the voltage HIGH
  delay(150);                       // wait for a second
  digitalWrite(LED_Status, HIGH);    // turn the LED OFF by making the voltage LOW

  // Loop Delay
  delay(LoopDelay * 1000);  
}
