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
char trash = '\0';

void threadLoop2(void* arg) {
  while (true) {
    // Check communications with the ESP8266 on TX1/RX1
    StegoPhone::StegoPhone::ConsoleSerial.println();
    StegoPhone::StegoPhone::ConsoleSerial.print("Checking ESP8266...");
    delay(500);
    StegoPhone::StegoPhone::ESP8266Serial.println("AT+GMR");
    if(StegoPhone::StegoPhone::recFind(StegoPhone::StegoPhone::ESP8266Serial, "OK", 5000)){
      StegoPhone::StegoPhone::ConsoleSerial.println("....Success");
    }
    else{
      StegoPhone::StegoPhone::ConsoleSerial.println("Failed");
    }
    // Clear Seria11 RX buffer
    StegoPhone::StegoPhone::ConsoleSerial.println("Test Complete");
    StegoPhone::StegoPhone::ConsoleSerial.println();
    while (StegoPhone::StegoPhone::ESP8266Serial.available() > 0){
      trash = StegoPhone::StegoPhone::ESP8266Serial.read();
    }
    delay(2000);
  }
}

void threadLoop1(void* arg) {
  while (1) {
    StegoPhone::StegoPhone::getInstance()->loop();
    delay(500);
  }
}

time_t getTeensy3Time()
{
  return Teensy3Clock.get();
}

void setup() {                
  StegoPhone::StegoPhone::getInstance()->setup();
  
  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);

  // initialize semaphore
  sem = xSemaphoreCreateCounting(1, 0);
  portBASE_TYPE s1, s2;
  // create task at priority two
  s1 = xTaskCreate(threadLoop1, NULL, configMINIMAL_STACK_SIZE, NULL, 2, NULL);
  // create task at priority one
  s2 = xTaskCreate(threadLoop2, NULL, configMINIMAL_STACK_SIZE, NULL, 1, NULL);

  // check for creation errors
  if (sem== NULL || s1 != pdPASS || s2 != pdPASS ) {
    StegoPhone::StegoPhone::ConsoleSerial.println("Creation problem");
    while(1);
  }

  StegoPhone::StegoPhone::ConsoleSerial.println("Starting the scheduler !");

  // start scheduler
  vTaskStartScheduler();
  StegoPhone::StegoPhone::ConsoleSerial.println("Insufficient RAM");
  while(1);
}

//------------------------------------------------------------------------------
// WARNING idle loop has a very small stack (configMINIMAL_STACK_SIZE)
// loop must never block
void loop() {
  // Not used.
}