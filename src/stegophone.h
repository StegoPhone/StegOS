//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#ifndef _STEGOPHONE_H_
#define _STEGOPHONE_H_

#include <Arduino.h>
#define U8G2_16BIT
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <TimeLib.h>

#include "SdFat.h"
#include "sdios.h"
#include "FreeStack.h"
#include "ExFatlib\ExFatLib.h"

#define SD_CONFIG SdioConfig(DMA_SDIO)

//#include "BMP.h"
#include "rn52.h"

namespace StegoPhone
{
  enum class StegoStatus {
    Offline,
    InitializationStart,
    InitializationFailure,
    DisplayInitialized,
    InputInitialized,
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
      static const byte userLEDPin = 13;        //
      static const byte OLED_CLK_Pin = 16;      //
      static const byte OLED_SDA_Pin = 17;      //
      static const byte OLED_CS_Pin = 10;       //
      static const byte OLED_DC_Pin = 9;        //
      static const byte OLED_RESET_Pin = 33;    //

      static const int ConsoleSerialRate = 9600;
      static const int ESP8266SerialRate = 115200;
      static const int RN52SerialRate = 115200;

      static usb_serial_class ConsoleSerial;
      static HardwareSerial ESP8266Serial;
      static HardwareSerial RN52Serial;
      //================================================================================================
      static bool recFind(HardwareSerial serialPort, String target, uint32_t timeout);
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
      static void intRN52Update();
      volatile bool userLEDStatus;
      volatile boolean rn52InterruptOccurred; // updated by ISR if RN52 has an event

      // HARDWARE HANDLES
      //================================================================================================
      static U8G2_SSD1322_NHD_256X64_F_4W_SW_SPI display;

      static SdExFat sd;

      // Internal
      //================================================================================================
      StegoPhone();
      StegoStatus _status;
      static StegoPhone *_instance;
    };
}

#endif
