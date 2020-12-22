//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include <cmath>
#include <iostream>
#include <cstdio>
#include <fstream>
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
    std::vector<unsigned char> logoEncoded;
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

      ExFile stegoLogo = sd.open("stegophone.png", FILE_READ);
      if (stegoLogo) {
        display.drawStr(0,40,"Reading Logo File");
        display.sendBuffer();

        // get its size:
        uint64_t fileSize = stegoLogo.size();

        // read the data:
        unsigned char pngBytes[fileSize];
        int count = stegoLogo.read(pngBytes, fileSize);
        ConsoleSerial.println(count);
        stegoLogo.close();

        display.drawStr(0,50,"Preparing palatte");
        display.sendBuffer();

        //create encoder and set settings and info (optional)
        lodepng::State state;
        //generate palette
        for(int i = 0; i < 16; i++) {
          unsigned char r = 127 * (1 + std::sin(5 * i * 6.28318531 / 16));
          unsigned char g = 127 * (1 + std::sin(2 * i * 6.28318531 / 16));
          unsigned char b = 127 * (1 + std::sin(3 * i * 6.28318531 / 16));
          unsigned char a = 63 * (1 + std::sin(8 * i * 6.28318531 / 16)) + 128; /*alpha channel of the palette (tRNS chunk)*/

          //palette must be added both to input and output color mode, because in this
          //sample both the raw image and the expected PNG image use that palette.
          lodepng_palette_add(&state.info_png.color, r, g, b, a);
          lodepng_palette_add(&state.info_raw, r, g, b, a);
        }
        //both the raw image and the encoded image must get colorType 3 (palette)
        state.info_png.color.colortype = LCT_PALETTE; //if you comment this line, and create the above palette in info_raw instead, then you get the same image in a RGBA PNG.
        state.info_png.color.bitdepth = 4;
        state.info_raw.colortype = LCT_PALETTE;
        state.info_raw.bitdepth = 4;
        state.encoder.auto_convert = 0; //we specify ourselves exactly what output PNG color mode we want

        char bytesStr[50];
        snprintf(bytesStr, sizeof(bytesStr), "Encoding %" PRIu64 " bytes", fileSize);
        display.drawStr(0,60,bytesStr);
        display.sendBuffer();

        //encode
        unsigned error = lodepng::encode(logoEncoded, pngBytes, 256, 64, state);
        display.clearBuffer();
        display.drawStr(0,10,"StegoPhone / StegOS");

        if(error) {
          display.drawStr(0,60,lodepng_error_text(error));
          display.sendBuffer();
          ConsoleSerial.println("encoder error");
          ConsoleSerial.println(error);
          ConsoleSerial.println(lodepng_error_text(error));
        } else {
          haveLogo = true;
        }
      } else {
        ConsoleSerial.println("Failed to read from SD");
        display.drawStr(0,60,"Failed to read from SD");
        display.sendBuffer();
      }
    }

    if (haveLogo) {
      display.clearBuffer();
      display.drawXBM( 0, 0, 256, 64, (const uint8_t *)&logoEncoded[0]);
      display.sendBuffer();

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
