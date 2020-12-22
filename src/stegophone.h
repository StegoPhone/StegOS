//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#ifndef _STEGOPHONE_H_
#define _STEGOPHONE_H_

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include "keypad.h"
#include "rn52.h"

namespace StegoPhone
{
  enum class StegoStatus {
    Offline,
    InitializationStart,
    InitializationFailure,
    DisplayInitialized,
    KeypadInitialized,
    ExternalRN52Initialized,
    InternalBTInitialized,    
    Ready,
    Ringing,
    CallConnected,
    PiedPiperPiping,
    PiedPiperDuet,
    CallPINSending,
    CallPINSynchronized,
    ModemInitializing,
    ModemHandshaking,
    EncryptionHandshaking,
    EncryptedDataEstablished,
    EncryptedVoice
  };

  class StegoPhone
  {
    public:
      // PIN DEFINITIONS
      //================================================================================================
      static const byte rn52ENPin = 2;          // active high
      static const byte rn52CMDPin = 3;         // active low
      static const byte rn52SPISel = 4;         //
      static const byte rn52InterruptPin = 30;  // input no pull, active low 100ms
      static const byte keypadInterruptPin = 6; // input no pull, active low
      static const byte twistInterruptPin = 7;  // input no pull, active low
      static const byte userLEDPin = 13;        // D13

      static const int ConsoleSerialRate = 115200;
      static const int DisplaySerialRate = 9600;
      static const int ESP8266SerialRate = 115200;
      static const int RN52SerialRate = 115200;

      usb_serial_class ConsoleSerial = Serial;
      HardwareSerial ESP8266Serial = Serial1;
      HardwareSerial DisplaySerial = Serial2;
      HardwareSerial RN52Serial = Serial7;
      //================================================================================================

      StegoStatus status();

      static StegoPhone *getInstance();
      void setup();
      void loop();

      // Built-In LED
      //================================================================================================
      void setUserLED(bool newValue);
      
      void toggleUserLED();
      
      void blinkForever(int interval  = 1000);

    protected:
      // ISR/MODIFIED
      //================================================================================================
      static void intReadKeypad();  
      static void intRN52Update();
      volatile bool userLEDStatus;
      volatile boolean rn52InterruptOccurred; // updated by ISR if RN52 has an event
      volatile boolean keypadInterruptOccurred; // used to keep track if there is a button on the stack

      // HARDWARE HANDLES
      //================================================================================================
      static KEYPAD keypad;

      // Internal
      //================================================================================================
      StegoPhone();
      StegoStatus _status;
      static StegoPhone *_instance;
    };
}

#endif
