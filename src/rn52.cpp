//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include "stegophone.h"
#include "rn52.h"
#include "linebuffer.h"

namespace StegoPhone {
    RN52 *RN52::_instance = 0;

    RN52 *RN52::getInstance() {
        if (0 == _instance)
            _instance = new RN52();
        return _instance;
    }

    RN52::RN52() {
        this->_lineBuffer = new LineBuffer;
        this->interruptOccurred = false; // updated by ISR if RN52 has an event
    }

    void RN52::intRN52Update() // (static isr)
    {
        RN52::getInstance()->interruptOccurred = true;
    }

    bool RN52::setup() {
        pinMode(StegoPhone::rn52ENPin, OUTPUT);
        pinMode(StegoPhone::rn52CMDPin, OUTPUT);
        pinMode(StegoPhone::rn52SPISel, OUTPUT);
        digitalWrite(StegoPhone::rn52SPISel, false);

        // force modes/init
        this->_cmd = true;
        this->_enabled = false;
        setEnable(false);
        setCmdModeEnable(true);

        return !this->exceptionOccurred;
    }

    void RN52::loop() {
        //this->_lineBuffer->loop();

        StegoPhone *stego = StegoPhone::StegoPhone::getInstance();

        // blink if you can hear me
        if (this->interruptOccurred) {
            stego->toggleUserLED();
            this->updateStatus();
            this->interruptOccurred = false;
        }
    }

    bool RN52::ExceptionOccurred() {
        return this->exceptionOccurred;
    }

    bool RN52::Enabled() {
        return this->_enabled;
    }

    void RN52::setEnable(bool newValue) {
        this->_enabled = newValue;
        digitalWrite(StegoPhone::StegoPhone::rn52ENPin, this->_enabled);
    }

    bool RN52::Enable() {
        if (this->_enabled) {
            this->exceptionOccurred = true;
            return false;
        }
        this->setEnable(true);

        // waitfor CMD
        char buf[1024];
        bool matched = this->readSerialUntil("CMD\r\n", buf, sizeof(buf), 5000);
        if (!matched) {
            this->exceptionOccurred = true;
            this->Disable();
        } else {
            this->_enabled = true;
            attachInterrupt(digitalPinToInterrupt(StegoPhone::StegoPhone::rn52InterruptPin), intRN52Update, FALLING);

            bool matched = this->rn52Exec("D", buf, sizeof(buf), "END\r\n");
            if (!matched) {

            }
        }

        return this->_enabled;
    }

    bool RN52::Disable() {
        this->setEnable(false);
        detachInterrupt(digitalPinToInterrupt(StegoPhone::StegoPhone::rn52InterruptPin));
        return !this->_enabled;
    }

    void RN52::setCmdModeEnable(bool newValue) {
        this->_cmd = newValue;
        digitalWrite(StegoPhone::StegoPhone::rn52ENPin, !this->_cmd); // Active LOW
    }

    bool RN52::CmdMode() {
        return this->_cmd;
    }

    bool RN52::CmdModeEnable() {
        this->setCmdModeEnable(true);
        return this->_cmd;
    }

    bool RN52::CmdModeDisable() {
        this->setCmdModeEnable(false);
        return this->_cmd;
    }

    void RN52::rn52Command(const char *cmd) {
        if (this->exceptionOccurred) return;
        StegoPhone::StegoPhone::RN52Serial.write(cmd);
        StegoPhone::StegoPhone::RN52Serial.write('\n');
    }

    void RN52::flushSerial() {
        while (StegoPhone::StegoPhone::RN52Serial.available() > 0)
            StegoPhone::StegoPhone::RN52Serial.read();
    }

    bool RN52::readSerialUntil(const char *match, char *buf, const uint32_t bufferSize, uint32_t timeout) {
        memset(buf, 0, bufferSize);
        unsigned long startMillis = millis();
        bool matched = false;
        int bufReceived = 0;
        while (!matched && (bufReceived < (bufferSize - 1)) && (millis() - startMillis < timeout)) {
            if (StegoPhone::StegoPhone::RN52Serial.available() > 0) {
                const char c = (char) StegoPhone::StegoPhone::RN52Serial.read();

                StegoPhone::StegoPhone::ConsoleSerial.print("BT: ");
                StegoPhone::StegoPhone::ConsoleSerial.print(c, DEC);
                StegoPhone::StegoPhone::ConsoleSerial.print(" - ");
                StegoPhone::StegoPhone::ConsoleSerial.print(c, HEX);
                StegoPhone::StegoPhone::ConsoleSerial.print(" -> ");
                StegoPhone::StegoPhone::ConsoleSerial.println(c);

                buf[bufReceived++] = c;

                // last bytes of buffer must = match value
                int matchLen = max(min(bufReceived, strlen(match)), bufferSize - 1);
                if (matchLen == 0) continue;
                if (strncmp(match, buf + (bufferSize - (matchLen + 1)), matchLen) == 0) { // +1 to leave null
                    matched = true;
                }
            }
        }
        return matched;
    }

    bool RN52::rn52Exec(const char *cmd, char *buf, const int bufferSize, const char *match, const int interDelay, const int timeout) {
        if (this->exceptionOccurred) return false;
        if (bufferSize < 1) return false;
        flushSerial();
        rn52Command(cmd);
        delay(interDelay);
        bool matched = readSerialUntil(match, buf, bufferSize, 500); // 1/2 second to complete command
        return matched;
    }

    unsigned short RN52::rn52Status(char *hexArray) {
        if (this->exceptionOccurred) return 0;
        char result[10]; // need 8, allow serbuf to pick up 10 to be sure for leftovers
        bool matched = rn52Exec("Q", result, sizeof(result), "\r\n");
        StegoPhone::StegoPhone::ConsoleSerial.println(matched ? "Matched:" : "No match:");
        for (int i = 0; i < sizeof(result); i++) {
            StegoPhone::StegoPhone::ConsoleSerial.print(result[i], DEC);
            StegoPhone::StegoPhone::ConsoleSerial.print(" - ");
            StegoPhone::StegoPhone::ConsoleSerial.print(result[i], HEX);
            StegoPhone::StegoPhone::ConsoleSerial.print(" -> ");
            StegoPhone::StegoPhone::ConsoleSerial.println((char) result[i]);
        }

        // for (uint8_t i = 4; i < sizeof(result); i++)
        //     result[i] = '\0'; // although null term'd at 10, data ends after 0-3. knock out the rest
        StegoPhone::StegoPhone::ConsoleSerial.println(result);
        for (int i = 0; i < 5; i++) // copy through term to output array
            hexArray[i] = result[i];

        StegoPhone::getInstance()->drawDisplay(0, 50, "RN52:", true, false);
        StegoPhone::getInstance()->drawDisplay(80, 50, "          ", true, false);
        StegoPhone::getInstance()->drawDisplay(80, 50, hexArray, true, false);

        return 0;
        // unsigned short retval = strtol(result, NULL, 16); // return decimal value
        // return retval;
    }

    void RN52::receiveLine(char *line) {
        Serial.print("RN52 RX: ");
        Serial.print(line);
    }

    // after an interrupt, poll the RN52 for its new status
    void RN52::updateStatus() {
        char hexStatus[5];
        const unsigned short s = this->rn52Status(hexStatus);
        Serial.print("RN52 Status DEC / HEX: ");
        Serial.print(s, DEC);
        Serial.print(" / ");
        Serial.print(s, HEX);
        Serial.print(" : orig=");
        Serial.println(hexStatus);
    }

    RN52Status RN52::status() {
        return this->_status;
    }
}