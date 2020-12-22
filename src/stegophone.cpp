//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include <stddef.h>
#include "stegophone.h"

namespace StegoPhone
{
  StegoPhone *StegoPhone::_instance = 0;


  KEYPAD StegoPhone::keypad = KEYPAD();

  StegoPhone::StegoPhone() {
    this->_status = StegoStatus::Offline;
    this->userLEDStatus = true;
    this->rn52InterruptOccurred = false; // updated by ISR if RN52 has an event
    this->keypadInterruptOccurred = false; // used to keep track if there is a button on the stack

    // Serial ports
    ConsoleSerial.begin(ConsoleSerialRate); // console/debug
    RN52Serial.begin(RN52SerialRate); // Connected to RN52
    ESP8266Serial.begin(ESP8266SerialRate); // ESP-12E
    DisplaySerial.begin(DisplaySerialRate); // ESP-12E

    pinMode(rn52ENPin, OUTPUT);
    pinMode(rn52CMDPin, OUTPUT);
    pinMode(rn52SPISel, OUTPUT);
    pinMode(rn52InterruptPin, INPUT); // Qwiic Keypad holds INT pin HIGH @ 3.3V, then LOW when fired.
    pinMode(keypadInterruptPin, INPUT); // Qwiic Keypad holds INT pin HIGH @ 3.3V, then LOW when fired.
    // Note, this means we do not want INPUT_PULLUP.
    pinMode(userLEDPin, OUTPUT);

    digitalWrite(rn52ENPin, false);
    digitalWrite(rn52CMDPin, false); // cmd mode by default
    digitalWrite(rn52SPISel, false);
    digitalWrite(userLEDPin, StegoPhone::userLEDStatus);

    attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), intReadKeypad, FALLING);
    // Note, INT on the Keypad will "fall" from HIGH to LOW when a new button has been pressed.
    // Also note, it will stay low while there are still button events on the stack.
    // This is useful if you want to "poll" the INT pin, rather than use a hardware interrupt.
    attachInterrupt(digitalPinToInterrupt(rn52InterruptPin), intRN52Update, FALLING);

    Wire.begin(); //Join I2C bus
  }

  StegoPhone *StegoPhone::getInstance() {
    if (0 == _instance)
      _instance = new StegoPhone();
    return _instance;
  }

  void StegoPhone::setup() {
    this->_status = StegoStatus::InitializationStart;

    this->_status = StegoStatus::DisplayInitialized;
    DisplaySerial.write(0xFE);
    DisplaySerial.write(0x01);
    DisplaySerial.println("StegOS");
    delay(500);

    if (keypad.begin() == false)   // Note, using begin() like this will use default I2C address, 0x4B.
    {                              // You can pass begin() a different address like so: keypad1.begin(Wire, 0x4A).
      //this->displayMessageDual("KYPD","MSNG");
      this->_status = StegoStatus::InitializationFailure;
      this->blinkForever();
    }
    // keypad input is ok

    // boot RN-52
    DisplaySerial.println("RN52 Init");
    RN52 *rn52 = RN52::getInstance();
    // try to initialize
    if (rn52->setup()) {
      DisplaySerial.println("RN52 Error");
      this->_status = StegoStatus::InitializationFailure;
      this->blinkForever();
    } else {
      DisplaySerial.println("StegoPhone Ready");
      this->_status = StegoStatus::Ready; 
    }
  }

  void StegoPhone::loop()
  {
      // blink if you can hear me
      if (this->keypadInterruptOccurred || this->rn52InterruptOccurred)
        this->toggleUserLED();

      // process any keypad inputs first
      if (this->keypadInterruptOccurred) {
        keypad.updateFIFO();  // necessary for keypad to pull button from stack to readable register
        const byte pressedDigit = keypad.getButton();
        if (pressedDigit == -1) {
          // fifo cleared
        } else {
          DisplaySerial.print((char) pressedDigit);
          ConsoleSerial.print((char) pressedDigit);
        }
        this->keypadInterruptOccurred = false;
      }

      // give the RN52 a chance to handle its inputs
      RN52 *rn52 = RN52::getInstance();
      rn52->loop(this->rn52InterruptOccurred);
      this->rn52InterruptOccurred = false;

      switch (this->_status) {
        case StegoStatus::Ready:
        // intentional fallthrough
        default:
          break;
      }
  }

  StegoStatus StegoPhone::status() {
    return this->_status;
  }

  void StegoPhone::setUserLED(bool newValue) {
    this->userLEDStatus = newValue;
    digitalWrite(userLEDPin, this->userLEDStatus);
  }

  void StegoPhone::toggleUserLED() {
        setUserLED(!this->userLEDStatus);
  }

  void StegoPhone::blinkForever(int interval) {
      while (1) {
        this->toggleUserLED();
        delay(interval);
      }
  }

  // intReadKeypad() is triggered via FALLING interrupt pin (digital pin 2)
  // it them updates the global variable keypadInterruptOccurred
  void StegoPhone::intReadKeypad() // (static isr) keep this quick; no delays, prints, I2C allowed.
  {
    StegoPhone::getInstance()->keypadInterruptOccurred = true;
  }

  void StegoPhone::intRN52Update() // (static isr)
  {
    StegoPhone::getInstance()->rn52InterruptOccurred = true;
  }
}
