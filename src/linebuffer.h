//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#ifndef _LINEBUFFER_H_
#define _LINEBUFFER_H_

#include <Arduino.h>
#include <Callback.h>

namespace StegoPhone
{
  class LineBuffer
  {
    public:
      LineBuffer();
      ~LineBuffer();
      void setup();
      void loop();
      Signal<char*> lineReceived;
      
    private:
      char *_serialBuffer;
      int _serialBufferSize;
      int _serialBufferDataLength;
  };
}

#endif //_LINEBUFFER_H_