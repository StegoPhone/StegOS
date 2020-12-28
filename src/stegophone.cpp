//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include <cmath>
#include <stddef.h>
#include "stegophone.h"

namespace StegoPhone
{
  StegoPhone *StegoPhone::_instance = 0;

  U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI StegoPhone::display(U8G2_R0, OLED_CLK_Pin, OLED_SDA_Pin, OLED_CS_Pin, OLED_DC_Pin, OLED_RESET_Pin);

  usb_serial_class StegoPhone::ConsoleSerial = Serial;
  HardwareSerial StegoPhone::ESP8266Serial = Serial1;
  HardwareSerial StegoPhone::RN52Serial = Serial7;
  SdExFat StegoPhone::sd = SdExFat();

  StegoPhone::StegoPhone() {
    this->_status = StegoStatus::Offline;
    this->userLEDStatus = true;
    this->rn52InterruptOccurred = false; // updated by ISR if RN52 has an event

    // Serial ports
    ConsoleSerial.begin(ConsoleSerialRate); // console/debug
    RN52Serial.begin(RN52SerialRate); // Connected to RN52
    ESP8266Serial.begin(ESP8266SerialRate); // ESP-12E

    // Display
    display.begin();

    pinMode(rn52InterruptPin, INPUT);
    // Note, this means we do not want INPUT_PULLUP.
    pinMode(userLEDPin, OUTPUT);

    digitalWrite(userLEDPin, StegoPhone::userLEDStatus);

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
    display.clear();
    display.setFont(u8g2_font_amstrad_cpc_extended_8f);
    display.drawStr(0,10,"StegoPhone / StegOS");
    display.sendBuffer();

    this->_status = StegoStatus::DisplayInitialized;

    // boot RN-52
    display.drawStr(0,20,"RN52 Initializing");
    display.sendBuffer();
    RN52 *rn52 = RN52::getInstance();
    // try to initialize
    if (!rn52->setup()) {
      display.clearBuffer();
      display.drawStr(0,10,"StegoPhone / StegOS");
      display.drawStr(0,20,"RN52 Error");
      display.sendBuffer();
      this->_status = StegoStatus::InitializationFailure;
      this->blinkForever();
    } else {
      this->_status = StegoStatus::Ready;
    }

    display.clearBuffer();
    display.drawStr(0,10,"StegoPhone / StegOS");
    display.drawStr(0,20,"Initializing SD Card");
    display.sendBuffer();

    bool haveLogo = false;
    if (!sd.begin(SD_CONFIG)) {
      display.clearBuffer();
      display.drawStr(0,10,"StegoPhone / StegOS");
      display.drawStr(0,20,"SD Failed init");
      display.sendBuffer();
      ConsoleSerial.println("SD initialization failed");
      this->blinkForever();
    } else {
      display.clearBuffer();
      display.drawStr(0,10,"StegoPhone / StegOS");
      display.drawStr(0,20,"SD Initialized");
      display.drawStr(0,30,"Opening Logo File");
      display.sendBuffer();
    }

    if (haveLogo) {

    } else {
      display.clearBuffer();
      display.drawStr(0,10,"StegoPhone / StegOS");
      display.drawStr(0,20,"Ready");
      display.sendBuffer();
    }
  }

  void StegoPhone::loop()
  {
      // blink if you can hear me
      if (this->rn52InterruptOccurred)
        this->toggleUserLED();

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

  void StegoPhone::intRN52Update() // (static isr)
  {
    StegoPhone::getInstance()->rn52InterruptOccurred = true;
  }

  //Bool function to search Serial RX buffer for a string value
  bool StegoPhone::recFind(HardwareSerial serialPort, String target, uint32_t timeout)
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
}
