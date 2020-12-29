//################################################################################################
//## StegoPhone : Steganography over Telephone / StegOS
//## (c) 2020 Jessica Mulein (jessica@mulein.com)
//## All rights reserved.
//## Made available under the GPLv3
//################################################################################################

#ifndef _RN52_H_
#define _RN52_H_

#include <Arduino.h>
#include "linebuffer.h"

namespace StegoPhone {
    enum class RN52Status {
        Offline,
        Present,
        Error
    };

    class RN52 {
    public:
        static RN52 *getInstance();


        bool setup();

        void loop();

        void receiveLine(char *line);

        RN52Status status();

        void updateStatus();

        void rn52Command(const char *cmd);

        bool rn52Exec(const char *cmd, char *buf, const int bufferSize, const char *match, const int interDelay = 100, const int timeout = 500);

        unsigned short rn52Status(char *hexArray);

        bool ExceptionOccurred();

        bool Enable();

        bool Disable();

        bool Enabled();

        bool CmdMode();

        bool CmdModeEnable();

        bool CmdModeDisable();

        void flushSerial();

        bool readSerialUntil(const char *match, char *buf, const uint32_t bufferSize, uint32_t timeout);

    protected:
        void setEnable(bool newValue);

        void setCmdModeEnable(bool newValue);

        static RN52 *_instance;

        RN52();

        bool exceptionOccurred = false;
        LineBuffer *_lineBuffer;
        RN52Status _status;
        bool _enabled;
        bool _cmd;

        // ISR/MODIFIED
        //================================================================================================
        static void intRN52Update();

        volatile bool interruptOccurred; // updated by ISR if RN52 has an event
    };
}

#endif // _RN52_H_