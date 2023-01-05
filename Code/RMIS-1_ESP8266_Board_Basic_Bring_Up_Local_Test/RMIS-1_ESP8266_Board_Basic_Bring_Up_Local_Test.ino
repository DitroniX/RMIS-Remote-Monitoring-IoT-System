/*
  Dave Williams, DitroniX 2019-2022 (ditronix.net)
  RMIS 12S DC Remote Monitor IoT System ESP12S SDK 2.0
  PCA v1.00 - Test Code Firmware v1.1 - 12th April 2022
  This code will work with either RMIS-1-30 or RMIS-1-60.  See commented section.
  
  Simplified Board Bring Up and Test Ports (The RMIS 'Hello World')
  
  The purpose of this test code is to cycle through the various main functions of the board as part of bring up testing.
 
  This test code is OPEN SOURCE and although is is not intended for real world use, it may be freely used, or modified as needed.
*/

 /*
  * References to Supporting Libraries (Add to Managed Libraries)
  * MCP9808 Precision I2C Temperature Sensor - Adafruit_MCP9808
  * https://learn.adafruit.com/adafruit-mcp9808-precision-i2c-temperature-sensor-guide
  * 
  * Examaple Output
  * 
  * 
  * Input Current and Sensitivity (measurement scale)
  * -/+ 5A – 185 mV/A
  * -/+ 20A – 100 mV/A
  * -/+ 30 A – 66 mv/A
  */

#include "Arduino.h"
#include <Adafruit_MCP3008.h>
#include "Adafruit_MCP9808.h"
#include "Wire.h" 

// ######### OBJECTS #########
  Adafruit_MCP9808 TempSensor = Adafruit_MCP9808();
  Adafruit_MCP3008 adc;

// ######### VARIABLES / DEFINES / STATIC #########

  // App
  String AppVersion = "v1.220324";
  String AppBuild = "DitroniX RMIS 12S SDK PCA v2204-105";
  String AppName = "RMIS-1 Basic Bring Up - Local Test - No Wifi";

  // Variables  
  int count = 0;
  float PCB_Temp;
  float ADC_Voltage;
  int ADC_RAW;  
  char SensorResult[10];
  float SensorRAW;
  float SensorValue;  

  // MCP9808 Temp Sensor
  int MCP9808 = 0x18;  // PCB Temperature Sensor Address

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
  #define LED_Status 2 // LED

  // **************** INPUTS **************** 
  #define ADC A0  // Define ADC (0 DC - 15V DC)

// ######### SETUP #########
void setup() {

  //Settle
  delay(250);   

  // Initialize UART:
  Serial.begin(115200, SERIAL_8N1);  //115200  
  while (!Serial);
    Serial.println(""); 
    Serial.println(AppVersion + " Firmware - Initialized");
    Serial.println(AppBuild);
    Serial.println(AppName);
    Serial.println(""); 

  // Initialise I2C 
  Wire.begin(I2C_SDA, I2C_SCL); 

  // LED
  pinMode(LED_Status, OUTPUT);     

  // Scan I2C Devices
  Serial.print("Scanning I2C\t");
  byte count = 0;
  Serial.print("Found Devices: ");
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

  // Initialise ADC
  adc.begin();  

  // PCB Temperature Sensor
  if (!TempSensor.begin()) 
  {
    // Sensor or I2C Issue
    Serial.println("Could not find MCP9808! Sensor or I2C Issue. Please check the PCB !!");
  }
  else
  {
    Serial.print("Successfully connected to Digital Temperature Sensor MCP9808\t");

    // Set Resolution 0.0625Â°C 250 ms and Wake Up
    TempSensor.setResolution(1);
    
    // Read PCB_Temp Sensor and Output
    // TempSensor.wake(); // TempSensor Wake Up 
    SensorRAW = TempSensor.readTempC();
    Serial.print("Test PCB Temp: ");
    Serial.print(SensorRAW); Serial.println("°C"); 
    //TempSensor.shutdown_wake(1); // TempSensor Sleep
   }   

   Serial.println(""); 

}

// ######### LOOP #########
void loop() {

    // Stabalise
    delay(250);    
   
    count++;

  // Basic loop through the channels and output the value, with no calibration.

  // **************** HEADER ****************
    
    for (int chan=1; chan<5; chan++) {
    Serial.print("Ch "); Serial.print(chan); Serial.print("\t");
   }
   Serial.print(" Test ["); Serial.print(count); Serial.println("]");

  // **************** CURRENT ****************

  // Loop through all four channels and display the RAW ADC for Current Value
  for (int chan=0; chan<8; chan = chan+2) {
    Serial.print(adc.readADC(chan)); Serial.print("\t");
   }
   Serial.println(" Current RAW ADC Value");  

  // Loop through all four channels and display the Current
  for (int chan=0; chan<8; chan = chan+2) {
    // Divider value may need individual tolerance tweaking / calibrating / temperature
    Serial.print( ( (adc.readADC(chan) - 512) / 13.25) ); Serial.print("\t");
   }
   Serial.println(" Calculated Current A -/+");    
   Serial.println(""); 

  // **************** VOLTAGE ****************   

  // Loop through all four channels and display the RAW ADC Voltage Value
  for (int chan=1; chan<8; chan = chan+2) {
    Serial.print(adc.readADC(chan)); Serial.print("\t");
  }  

  Serial.println(" Voltage RAW ADC Value");   

    // Loop through all four channels and display the voltages
    for (int chan=1; chan<8; chan = chan+2) {
    // Uncomment/comment, depending on the RMIS version used 30V/60V
    // Multiplier value may need individual tolerance tweaking / calibrating / temperature    
    // Serial.print( ( adc.readADC(chan) * 71.5 ) / 1024 ) ; Serial.print("\t");  // RMIS-1-30
    Serial.print( ( adc.readADC(chan) * 117.87 ) / 1024 ) ; Serial.print("\t");  // RMIS-1-60
  }  

  Serial.println(" Calculated Voltage V");   
  Serial.println(""); 

  // **************** PCB **************** 
  
  // Read PCB_Temp Sensor
  PCB_Temp = TempSensor.readTempC();  
  // TempSensor.wake(); // TempSensor Wake Up    
  Serial.print("PCB Temp: ");
  Serial.print(PCB_Temp, 2); Serial.print("°C\t"); 
  //TempSensor.shutdown_wake(1); // TempSensor Sleep
  Serial.println(""); 

  // Read ADC DC Voltage Sensor (0-1V 0-1023).  Takes into account the protection diode.
  ADC_RAW = analogRead(ADC);
  ADC_Voltage = (analogRead(ADC) * 15.64) / 1024; // Multiplier value may need individual tolerance tweaking / calibrating / temperature
  Serial.print("ADC Raw Value: ");
  Serial.print(ADC_RAW);
  Serial.print("\t DC Input Voltage: ");
  Serial.print(ADC_Voltage);
  Serial.print(" V");  
  if (ADC_Voltage < 6) Serial.print(" [WARNING - RMIS DC Input Voltage LOW]");  
  Serial.println("");  

  Serial.println("------------------------------------------------------------------------------------"); 
    
  // Old Fashioned Heartbeat
  digitalWrite(LED_Status, LOW);   // turn the LED ON by making the output LOW
  delay(250); 
  digitalWrite(LED_Status, HIGH);    // turn the LED OFF by making the output HIGH
  delay(2000); 
}
