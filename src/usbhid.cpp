#include <Arduino.h>
#include "USBHost_t36.h"

namespace StegoPhone {
    class USBTmp {
    public:
        void test() {
            USBHost myusb;
            USBHub hub1(myusb);
            USBHub hub2(myusb);
            KeyboardController keyboard1(myusb);
            KeyboardController keyboard2(myusb);
            USBHIDParser hid1(myusb);
            USBHIDParser hid2(myusb);
            USBHIDParser hid3(myusb);
            USBHIDParser hid4(myusb);
            USBHIDParser hid5(myusb);
            MouseController mouse1(myusb);
            JoystickController joystick1(myusb);
            //BluetoothController bluet(myusb, true, "0000");   // Version does pairing to device
            BluetoothController bluet(myusb);   // version assumes it already was paired
            int user_axis[64];
            uint32_t buttons_prev = 0;
            RawHIDController rawhid1(myusb);
            RawHIDController rawhid2(myusb, 0xffc90004);

            USBDriver *drivers[] = {&hub1, &hub2, &keyboard1, &keyboard2, &joystick1, &bluet, &hid1, &hid2, &hid3,
                                    &hid4, &hid5};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
            const char *driver_names[CNT_DEVICES] = {"Hub1", "Hub2", "KB1", "KB2", "JOY1D", "Bluet", "HID1", "HID2",
                                                     "HID3", "HID4", "HID5"};
            bool driver_active[CNT_DEVICES] = {false, false, false, false};

            // Lets also look at HID Input devices
            USBHIDInput *hiddrivers[] = {&mouse1, &joystick1, &rawhid1, &rawhid2};
#define CNT_HIDDEVICES (sizeof(hiddrivers)/sizeof(hiddrivers[0]))
            const char *hid_driver_names[CNT_DEVICES] = {"Mouse1", "Joystick1", "RawHid1", "RawHid2"};
            bool hid_driver_active[CNT_DEVICES] = {false, false};
            bool show_changed_only = false;

            uint8_t joystick_left_trigger_value = 0;
            uint8_t joystick_right_trigger_value = 0;
            uint64_t joystick_full_notify_mask = (uint64_t) - 1;
            uint8_t buttons = 0;
        }
    };
}