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

    void StegoPhone::displayGrayscaleBytes(uint8_t count, unsigned char *data) {
        uint8_t u8x8_d_ssd1322_256x64_gray_preamble[] = {

                U8X8_DLY(1),
                U8X8_START_TRANSFER(),      /* enable chip, delay is part of the transfer start */
                U8X8_DLY(1),
                U8X8_CAA(0xA0, 0x14, 0x11), // Set_Remap_25664(0x14,0x11);
                U8X8_CAA(0x15, 0x1C, 0x5B), // Set_Column_Address_25664(0x1C,0x5B);
                U8X8_CAA(0x75, 0x00, 0x3F), // Set_Row_Address_25664(0x00,0x3F);
                U8X8_C(0x5C),               // Set_Write_RAM_25664() - Enable MCU to Write DATA RAM
        };
        uint8_t u8x8_d_ssd1322_256x64_gray_postamble[] = {
                U8X8_DLY(1),                          /* delay 2ms */
                U8X8_END_TRANSFER(),        /* disable chip */
                U8X8_END()                        /* end of sequence */
        };
        u8x8_t *tmp = display.getU8x8();
        u8x8_byte_SendBytes(tmp, sizeof(u8x8_d_ssd1322_256x64_gray_preamble), u8x8_d_ssd1322_256x64_gray_preamble);
        u8x8_byte_SendBytes(tmp, count, data);
        u8x8_byte_SendBytes(tmp, sizeof(u8x8_d_ssd1322_256x64_gray_postamble), u8x8_d_ssd1322_256x64_gray_postamble);
    }

    bool StegoPhone::displayLogo() {
        bool haveLogo = false;
        std::vector<unsigned char> logoEncoded;
        display.clearBuffer();
        display.drawStr(0, 10, "StegoPhone / StegOS");
        display.drawStr(0, 20, "SD Initialized");
        display.drawStr(0, 30, "Opening Logo File");
        display.sendBuffer();


        BMPpp::BMPpp stegoLogo;
        if (stegoLogo.read(StegoPhone::sd, "stegophone.bmp")) {
            display.drawStr(0, 40, "Reading Logo File");
            display.sendBuffer();

            BMPpp::BMP stegoBMP = stegoLogo.getBMP();
            // get its size:
            uint32_t fileSize = stegoBMP.file_header.file_size;
            std::vector<unsigned char> grayBMP = stegoLogo.get4bppPackedLCDGray();

            ConsoleSerial.println(fileSize);
            ConsoleSerial.println(grayBMP.size());
            ConsoleSerial.println(stegoBMP.bmp_info_header.height);
            ConsoleSerial.println(stegoBMP.bmp_info_header.width);
            displayGrayscaleBytes(grayBMP.size(), &grayBMP[0]);

            haveLogo = true;
        }

        return haveLogo;
    }

    void StegoPhone::setup() {
        this->_status = StegoStatus::InitializationStart;
        display.clear();
        display.setFont(u8g2_font_amstrad_cpc_extended_8f);
        display.drawStr(0, 10, "StegoPhone / StegOS");
        display.sendBuffer();

        this->_status = StegoStatus::DisplayInitialized;

        // boot RN-52
        display.drawStr(0, 20, "RN52 Initializing");
        display.sendBuffer();
        RN52 *rn52 = RN52::getInstance();
        // try to initialize
        if (!rn52->setup()) {
            display.clearBuffer();
            display.drawStr(0, 10, "StegoPhone / StegOS");
            display.drawStr(0, 20, "RN52 Error");
            display.sendBuffer();
            this->_status = StegoStatus::InitializationFailure;
            this->blinkForever();
        } else {
            this->_status = StegoStatus::Ready;
        }

        display.clearBuffer();
        display.drawStr(0, 10, "StegoPhone / StegOS");
        display.drawStr(0, 20, "Initializing SD Card");
        display.sendBuffer();

        if (!sd.begin(SD_CONFIG)) {
            display.clearBuffer();
            display.drawStr(0, 10, "StegoPhone / StegOS");
            display.drawStr(0, 20, "SD Failed init");
            display.sendBuffer();
            ConsoleSerial.println("SD initialization failed");
            this->blinkForever();
        }

        if (!this->displayLogo()) {
            display.clearBuffer();
            display.drawStr(0, 10, "StegoPhone / StegOS");
            display.drawStr(0, 20, "Ready");
            display.sendBuffer();
        }
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
