//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include <cstdio>
#include "stegophone.h"
#include "rn52.h"
#include "linebuffer.h"

namespace StegoPhone
{
  RN52 *RN52::getInstance() {
    if (0 == _instance)
      _instance = new RN52();
    return _instance;
  }
  
  RN52::RN52() {
    this->_lineBuffer = new LineBuffer;
  }

  bool RN52::setup()
  {
      //this->_lineBuffer->setup();
    // force modes/init
    this->_cmd = true;
    this->_enabled = false;
    pinMode(StegoPhone::rn52ENPin, OUTPUT);
    pinMode(StegoPhone::rn52CMDPin, OUTPUT);
    pinMode(StegoPhone::rn52SPISel, OUTPUT);
    digitalWrite(StegoPhone::rn52ENPin, !this->_enabled);
    digitalWrite(StegoPhone::rn52CMDPin, !this->_cmd); // cmd mode by default
    digitalWrite(StegoPhone::rn52SPISel, false);    
      
    return !this->exceptionOccurred;
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

  void RN52::loop(bool rn52InterruptOccurred) {
    //this->_lineBuffer->loop();

    //StegoPhone *stego = StegoPhone::StegoPhone::getInstance();

    if (rn52InterruptOccurred)
      this->updateStatus();
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
    if(!StegoPhone::recFind(StegoPhone::StegoPhone::RN52Serial, "CMD", 5000)){
      this->exceptionOccurred = true;
      this->Disable();
    } else {
      this->_enabled = true;
    }

    return this->_enabled;
  }

  bool RN52::Disable() {
    this->setEnable(false);
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
    Serial1.write(cmd);
    Serial1.write('\n');
  }
  
  void RN52::rn52Exec(const char *cmd, char *buf, const int bufferSize, const int interDelay) {
    if (this->exceptionOccurred) return;
    if (bufferSize < 1) return;
    rn52Command(cmd);
    delay(interDelay);
    Serial1.setTimeout(500);
    Serial1.readBytesUntil(0x0D, buf, bufferSize);
    for (int i=0; i<bufferSize; i++) Serial.println(buf[i], DEC);
    buf[bufferSize - 1] = '\0'; // enforce null term at end of array
  }
  
  unsigned short RN52::rn52Status(char *hexArray) {  
    if (this->exceptionOccurred) return 0;
    char result[10]; // need 8, allow serbuf to pick up 10 to be sure for leftovers
    rn52Exec("Q", result, sizeof(result)); 
    for (uint8_t i=4; i<sizeof(result); i++)
      result[i] = '\0'; // although null term'd at 10, data ends after 0-3. knock out the rest
    Serial.println(result);
    for (int i=0; i<5; i++) // copy through term to output array
      hexArray[i] = result[i];      
    char * pEnd;
    return strtol(result, &pEnd, 16); // return decimal value
  }
  
  void RN52::rn52Debug(const char *cmd, const int interDelay, const int bufferSize) {
    if (this->exceptionOccurred) return;
    char cmdTest[bufferSize];
    uint8_t cmdIdx = 0;
    //StegoPhone *stego = StegoPhone::getInstance();
    rn52Command(cmd);
    delay(interDelay);
    while (Serial1.available() > 0) {
      int incomingByte = Serial1.read();
      if (cmdIdx < (sizeof(cmdTest)-1)) {
        cmdTest[cmdIdx++] = (char) incomingByte;
      } else {
        break;
      }
    }
    cmdTest[cmdIdx++] = '\0';
    Serial.print("RN52:");
    Serial.print(cmdTest);
  }
  
  RN52 *RN52::_instance = 0;
}