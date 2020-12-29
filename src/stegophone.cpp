//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include <cmath>
#include <stddef.h>
#include "stegophone.h"

namespace StegoPhone {
    StegoPhone *StegoPhone::_instance = 0;

    U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI StegoPhone::display(U8G2_R0, OLED_CLK_Pin, OLED_SDA_Pin, OLED_CS_Pin,
                                                            OLED_DC_Pin, OLED_RESET_Pin);

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

    void StegoPhone::drawDisplay(u8g2_int_t x, u8g2_int_t y, uint64_t data, bool send, bool clear) {
        char buf[50];
        snprintf(buf, sizeof(buf),  "%" PRIu64, data);
        drawDisplay(x , y, buf, send, clear);
    }

    void StegoPhone::drawDisplay(u8g2_int_t x, u8g2_int_t y, const char* data, bool send, bool clear) {
        if (clear) display.clearBuffer();
        display.drawStr(x, y, data);
        if (send) display.sendBuffer();
    }

    void StegoPhone::drawDisplay(u8g2_int_t x, u8g2_int_t y, std::vector<char*> data, bool send, bool clear) {
        for (size_t i=0; i<data.size(); i++) {
            drawDisplay(x,y,data[i], (i == 0) && send, (i == (data.size() - 1)) && clear);
        }
    }

    bool StegoPhone::displayLogo() {
        bool haveLogo = false;
        std::vector<unsigned char> logoEncoded;
        drawDisplay(0, 10, "StegoPhone / StegOS", true, true);
        drawDisplay(0, 20, "SD Initialized", true, false);
        drawDisplay(0, 30, "Opening Logo File", true, false);

        ExFile stegoLogo = StegoPhone::sd.open("stegophone.xbm", FILE_READ | O_READ);
        if (stegoLogo) {
            uint64_t fileSize = stegoLogo.size();
            drawDisplay(0, 10, fileSize, true, true);

            delay(1000);
            uint8_t fileBuf[fileSize];
            uint64_t read = stegoLogo.readBytes(fileBuf, fileSize);
            drawDisplay(0, 20, read, true, false);
            delay(1000);
            if (read == fileSize) {
                display.drawXBM(0, 0, 256, 64, fileBuf);
                display.sendBuffer();
                haveLogo = true;
            } else {
                drawDisplay(0,10, "error", true, true);
            }
        } else {
            drawDisplay(0, 10, "no load", true, true);
        }

        return haveLogo;
    }

    void StegoPhone::setup() {
        this->_status = StegoStatus::InitializationStart;
        display.setFont(u8g2_font_amstrad_cpc_extended_8f);
        drawDisplay(0, 10, "StegoPhone / StegOS", true, true);

        this->_status = StegoStatus::DisplayInitialized;

        // boot RN-52
        drawDisplay(0, 20, "RN52 Initializing", true, false);
        RN52 *rn52 = RN52::getInstance();
        // try to initialize
        if (!rn52->setup()) {
            drawDisplay(0, 10, "StegoPhone / StegOS", true, true);
            drawDisplay(0, 20, "RN52 Error", true, false);
            this->_status = StegoStatus::InitializationFailure;
            this->blinkForever();
        } else {
            this->_status = StegoStatus::Ready;
        }

        drawDisplay(0, 10, "StegoPhone / StegOS", true, true);
        drawDisplay(0, 20, "Initializing SD Card", true, false);

        if (!sd.begin(SD_CONFIG)) {
            drawDisplay(0, 10, "StegoPhone / StegOS", true, true);
            drawDisplay(0, 20, "SD Failed init", true, false);
            ConsoleSerial.println("SD initialization failed");
            this->blinkForever();
        }

        drawDisplay(0, 30, "SD init complete", true, false);

        if (!this->displayLogo()) {
        }
        // if (!this->displayLogo()) {
        //     display.clearBuffer();
        //     display.drawStr(0, 10, "StegoPhone / StegOS");
        //     display.drawStr(0, 20, "Ready");
        //     display.sendBuffer();
        // }
    }

    void StegoPhone::loop() {
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
    bool StegoPhone::recFind(HardwareSerial serialPort, String target, uint32_t timeout) {
        char rdChar = '\0';
        String rdBuff = "";
        unsigned long startMillis = millis();
        while (millis() - startMillis < timeout) {
            while (serialPort.available() > 0) {
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
        if (rdBuff.indexOf(target) != -1) {
            return true;
        } else {
            return false;
        }
    }
}
