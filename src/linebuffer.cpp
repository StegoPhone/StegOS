//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#include "linebuffer.h"

using namespace StegoPhone;

LineBuffer::LineBuffer() {
  this->_serialBufferDataLength = 0; // no bytes currently stored in the buffer
  this->_serialBufferSize = 1024; // default to 1k to start. actual size
  this->_serialBuffer = (char*) malloc(this->_serialBufferSize);
}

LineBuffer::~LineBuffer() {
  // will never happen
  free(this->_serialBuffer);
}

void LineBuffer::setup() {
  
}

void LineBuffer::loop() {
  const int bytesAvailable = Serial1.available();
  const int bufferAvailable = this->_serialBufferSize - _serialBufferDataLength;
  if (bytesAvailable > 0) {
    if (bufferAvailable < bytesAvailable) {
      const int newSize = 2 * _serialBufferSize;
      char *reallocPtr = (char *)realloc(this ->_serialBuffer, newSize);
      if (reallocPtr != 0) {
        this->_serialBuffer = reallocPtr;
        this->_serialBufferSize = newSize;
        Serial1.readBytes(this->_serialBuffer + this->_serialBufferDataLength, bytesAvailable);
        _serialBufferDataLength += bytesAvailable;

        const char delims[] = {'\n'};
        char *line = NULL;
        line = strtok(_serialBuffer, delims);
        while(line != NULL)
        {
            
            line = strtok(NULL, delims);
        }
      } else {
        // exception?
      }
    }
  }
}