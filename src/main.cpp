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

/*
This sketch tests the LEDs and ESP8266 on the Arduino-Teensy board.
The LEDs connected to the serial ports are active-low.  This means 
that you need to set the pinmode to OUTPUT and then use digitalwrite
to set the pin LOW in order to turn them on.  To turn them off, use
digitalwrite to set the pin HIGH, or use pinmode to set the pin to INPUT mode.
Pins D13 (built-in LED pin) and D30 (bluetooth module STATE pin) are
active-high and are turned on and off like standard LEDs.

TX1/RX1 are checked once before enabling Serial1 port due to the fact that
the Teensy does not handle disabling and re-enabling serial ports correctly.

*/
char trash = '\0';

//Bool function to search Serial RX buffer for a string value
bool recFind(String target, uint32_t timeout)
{
  char rdChar = '\0';
  String rdBuff = "";
  unsigned long startMillis = millis();
    while (millis() - startMillis < timeout){
      while (Serial1.available() > 0){
        rdChar = Serial1.read();
        rdBuff += rdChar;
        Serial.write(rdChar);
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
  // Check D13 LED (RED)
  Serial.println("D13 - built-in LED");
  digitalWrite(13, HIGH); //D13 built-in LED - Set the pin HIGH turn on the LED
  delay(2000);
  digitalWrite(13, LOW); //Set the pin LOW to turn off the LED

  // Check the Bluetooth State LED
  Serial.println("Bluetooth STATE pin");
  digitalWrite(30, HIGH); //D24 bluetooth STATE pin
  delay(2000);
  digitalWrite(30, LOW);
  
  // Check communications with the ESP8266 on TX1/RX1
  Serial.println();Serial.print("Checking ESP8266...");
  delay(500);
  Serial1.println("AT+GMR");
  if(recFind("OK", 5000)){
    Serial.println("....Success");
  }
  else{
    Serial.println("Failed");
  }
  // Clear Seria11 RX buffer
  Serial.println("Test Complete");
  Serial.println();
  while (Serial1.available() > 0){
    trash = Serial1.read();
  }
}

void threadLoop2(void* arg) {
  StegoPhone::StegoPhone::getInstance()->loop();
}

void setup() {                
  //Start the USB Serial port
  Serial.begin(115200);
  Serial.println("Started");
  // initialize the digital pins as an output.
  pinMode(13, OUTPUT);  
  pinMode(30, OUTPUT);    
  Serial.println("Checking LEDs");
  
  // Check TX1 LED
  pinMode(1, OUTPUT); // set the pinmode to OUTPUT
  Serial.println("TX1");
  digitalWrite(1, LOW); //TX1 - set the pin LOW to turn on the LED
  delay(2000);  //wait 2 seconds
  digitalWrite(1, HIGH); //set the pin HIGH to turn off the LED
  pinMode(1, INPUT);  //set pinmode back to INPUT so the LED will go out without the pin being set HIGH
  
  // Check RX1 LED
  pinMode(0, OUTPUT);
  Serial.println("RX1");
  digitalWrite(0, LOW); //RX1
  delay(2000);
  digitalWrite(0, HIGH);
  pinMode(0, INPUT);
  
  // Start Serial1 port
  Serial1.begin(115200);

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
    Serial.println("Creation problem");
    while(1);
  }

  Serial.println("Starting the scheduler !");

  // start scheduler
  vTaskStartScheduler();
  Serial.println("Insufficient RAM");
  while(1);
}

//------------------------------------------------------------------------------
// WARNING idle loop has a very small stack (configMINIMAL_STACK_SIZE)
// loop must never block
void loop() {
  // Not used.
}