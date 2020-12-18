//################################################################################################
//## StegoPhone : Steganography over Telephone
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#ifndef _STEGOPHONE_H_
#define _STEGOPHONE_H_

#include <Arduino.h>
#include "keypad.h"
//#include <Wire.h>
//include <ArduinoBLE.h>

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
      StegoStatus status();

      // PIN DEFINITIONS
      //================================================================================================
      static const byte rn52ENPin = 0;          // active high
      static const byte rn52CMDPin = 1;         // active low
      static const byte rn52SPISel = 2;         //
      static const byte rn52InterruptPin = 7; // input no pull, active low 100ms
      static const byte keypadInterruptPin = 8; // input no pull, active low
      static const byte userLED = 19;
      //================================================================================================

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
      KEYPAD keypad;

      // Internal
      //================================================================================================
      StegoPhone();
      StegoStatus _status;
      static StegoPhone *_instance;
    };
}

#endif