//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

// HEADERS
#include <Arduino.h>
#include <FreeRTOS_TEENSY4.h>
#include "stegophone.h"

// Declare a semaphore handle.
SemaphoreHandle_t sem;
usb_serial_class ConsoleSerial = Serial;
HardwareSerial ESP8266Serial = Serial1;
HardwareSerial RN52Serial = Serial7;
char trash = '\0';

//Bool function to search Serial RX buffer for a string value
bool recFind(HardwareSerial serialPort, String target, uint32_t timeout)
{
  char rdChar = '\0';
  String rdBuff = "";
  unsigned long startMillis = millis();
    while (millis() - startMillis < timeout){
      while (serialPort.available() > 0){
        rdChar = serialPort.read();
        rdBuff += rdChar;
        ConsoleSerial.write(rdChar);
        if (rdBuff.indexOf(target) != -1) {
          break;
        }
     }
     if (rdBuff.indexOf(target) != -1) {
          break;
     }
   }
   if (rdBuff.indexOf(target) != -1){
    return true;
   }
   else {
    return false;
   }
}

void threadLoop1(void* arg) {
  // Check communications with the ESP8266 on TX1/RX1
  ConsoleSerial.println();
  ConsoleSerial.print("Checking ESP8266...");
  delay(500);
  ESP8266Serial.println("AT+GMR");
  if(recFind(ESP8266Serial, "OK", 5000)){
    ConsoleSerial.println("....Success");
  }
  else{
    ConsoleSerial.println("Failed");
  }
  // Clear Seria11 RX buffer
  ConsoleSerial.println("Test Complete");
  ConsoleSerial.println();
  while (ESP8266Serial.available() > 0){
    trash = ESP8266Serial.read();
  }
  while(1);
}

void threadLoop2(void* arg) {
  while (1)
    StegoPhone::StegoPhone::getInstance()->loop();
}

void setup() {                
  StegoPhone::StegoPhone::getInstance()->setup();

  // initialize semaphore
  sem = xSemaphoreCreateCounting(1, 0);
  portBASE_TYPE s1, s2;
  // create task at priority two
  s1 = xTaskCreate(threadLoop1, NULL, configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  // create task at priority one
  s2 = xTaskCreate(threadLoop2, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  // check for creation errors
  if (sem== NULL || s1 != pdPASS || s2 != pdPASS ) {
    ConsoleSerial.println("Creation problem");
    while(1);
  }

  ConsoleSerial.println("Starting the scheduler !");

  // start scheduler
  vTaskStartScheduler();
  ConsoleSerial.println("Insufficient RAM");
  while(1);
}

//------------------------------------------------------------------------------
// WARNING idle loop has a very small stack (configMINIMAL_STACK_SIZE)
// loop must never block
void loop() {
  // Not used.
}